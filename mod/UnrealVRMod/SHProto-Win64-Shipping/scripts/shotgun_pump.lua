--------------------------------------------------------------------------
-- Shotgun manual pump (MCP-confirmed).
-- Kept in a separate module so main.lua stays under Lua's 200-local limit.
--
-- Flow (like pistol slide rack):
-- 1) Offhand stick: left grip near bone_forearm (NOT proximity-only).
-- 2) After fire: strip SingleActionPrepare soft refs + block
--    OnRangedSingleActionPrepareEnd so IsFireReady stays false and pump
--    mesh does not auto-animate. Eat RT until chambered.
-- 3) Grip pump, pull rearward along barrel axis (~85%) → unlock, eat RT until
--    release, force IsFireReady via locals.ReturnValue (UEVR: result is void*).
--    Soft refs stay stripped so native prepare cannot auto-chamber.
-- Soft refs are instance fields on WeaponShotgun_Alt_D_BP_C.
-- Do NOT touch shared AnimSequence RateScale (global asset).
--------------------------------------------------------------------------

local uevrUtils = require("libs/uevr_utils")
local controllers = require("libs/controllers")
local configui = require("libs/configui")

-- sh2vr (2026-10-31 evening): the REAL left grip. The plugin's button layer keeps the grips from the pad (the game would
-- sprint), so the pad's LB here is the run toggle, not the grip; sh2vr_bridge.lua keeps SH2VR_GRIP_L from the plugin.
local function sh2vrLeftGrip(stateIn)
	if SH2VR_GRIP_L ~= nil then
		return SH2VR_GRIP_L == true
	end
	return uevrUtils.isButtonPressed(stateIn, XINPUT_GAMEPAD_LEFT_SHOULDER)
end
local attachments = require("libs/attachments")
local accessories = require("libs/accessories")

local M = {}

local C = {
	ATTACHMENT_ID = "WeaponShotgun_Alt_D_BP_C_shotgun_05_A_rig",
	ACCESSORY_ID = "39a84ebe-48be-41eb-a830-38913571dc5e",
	ACCESSORY_PRIORITY = 2,
	DEFAULT_DIST = 13.3,
	PUMP_TRAVEL = 8.0,
	BP_CLASS = "BlueprintGeneratedClass /Game/Game/Gameplay/Weapons/Shotgun/WeaponShotgun_Alt_D_BP.WeaponShotgun_Alt_D_BP_C",
	PREPARE_WEAPON_ANIM_OFF = 0xC38, -- 3128
	PREPARE_ANIM_DATA_OFF = 0xC68, -- 3176
	SOFT_BYTES = 48,
	PREPARE_SLOT = "UpperBodyWeaponPrepare",
	PREPARE_MONTAGE = "AnimMontage /Game/Game/Art/Items/Shotgun/WeaponAnimation/Shotgun_Recoil_SingleActionPrepare_Montage.Shotgun_Recoil_SingleActionPrepare_Montage",
	PREPARE_BODY_ANIM = "AnimSequence /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Shotgun/RecoilAndReload/James_Shotgun_Recoil_Single_Action_Prepare.James_Shotgun_Recoil_Single_Action_Prepare",
	PREPARE_WEAPON_MESH_ANIM = "AnimSequence /Game/Game/Art/Items/Shotgun/WeaponAnimation/James_Shotgun_Recoil_Single_Action_Prepare_SkeletalShotgun.James_Shotgun_Recoil_Single_Action_Prepare_SkeletalShotgun",
	PREPARE_READY_NOTIFY = "AnimNotify_SingleActionPrepareReady /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Shotgun/RecoilAndReload/James_Shotgun_Recoil_Single_Action_Prepare.James_Shotgun_Recoil_Single_Action_Prepare.AnimNotify_SingleActionPrepareReady_0",
	MUZZLE_SOCKET = "FX_muzzle",
	PUMP_SOCKET = "bone_forearm",
}

local state = {
	leftGripDown = false,
	stuck = false,
	wasStuck = false,
	needsPump = false,
	pumpGripping = false,
	pumpProgress = 0.0,
	pumpProgressAtGrab = 0.0,
	pumpGrabAlong = 0.0,
	pumpOriginWs = nil,
	pumpAxisWs = nil,
	prepareHooked = false,
	fireHooked = false,
	shellHooked = false,
	readyHooked = false,
	strippedWeapon = nil,
	savedPrepareWeaponAnimBytes = nil,
	savedPrepareAnimDataBytes = nil,
	unlockPlaying = false,
	suppressLockUntil = 0.0,
	-- After unlock, keep eating RT until the player releases — held trigger was
	-- auto-firing and re-locking (log: unlocked → locked OnFirePerformed).
	awaitRtRelease = false,
	statusText = "idle",
	cachedRanged = nil,
	prepareMontage = nil,
	prepareBodyAnim = nil,
	prepareReadyNotify = nil,
	initialized = false,
	-- Diagnostics for "pump stuck after second tube" sessions.
	diagEvents = {},
	diagShotCount = 0,
	diagUnlockCount = 0,
	needsPumpSince = 0.0,
	lastPullLog = 0.0,
}

local function writeI32(obj, offset, value)
	if obj == nil or offset == nil then
		return false
	end
	value = math.floor(tonumber(value) or 0)
	if value < 0 then
		value = 0
	end
	local ok = pcall(function()
		if obj.write_dword ~= nil then
			obj:write_dword(offset, value)
		elseif obj.write_byte ~= nil then
			obj:write_byte(offset, value % 256)
			obj:write_byte(offset + 1, math.floor(value / 256) % 256)
			obj:write_byte(offset + 2, math.floor(value / 65536) % 256)
			obj:write_byte(offset + 3, math.floor(value / 16777216) % 256)
		else
			error("no write_dword")
		end
	end)
	return ok == true
end

local function readI32(obj, offset)
	if obj == nil or offset == nil then
		return nil
	end
	local ok, val = pcall(function()
		if obj.read_dword ~= nil then
			return obj:read_dword(offset)
		end
		if obj.read_byte ~= nil then
			local b0 = obj:read_byte(offset) or 0
			local b1 = obj:read_byte(offset + 1) or 0
			local b2 = obj:read_byte(offset + 2) or 0
			local b3 = obj:read_byte(offset + 3) or 0
			return b0 + b1 * 256 + b2 * 65536 + b3 * 16777216
		end
		return nil
	end)
	if ok then
		return val
	end
	return nil
end

local function setStatus(text)
	state.statusText = text or "idle"
	pcall(function()
		configui.setLabel("shotgun_pump_status", "Shotgun pump: " .. state.statusText)
	end)
end

local function diag(msg)
	local line = string.format("[%.3f] %s", os.clock(), tostring(msg))
	local ev = state.diagEvents
	ev[#ev + 1] = line
	while #ev > 60 do
		table.remove(ev, 1)
	end
	print("[shotgun_pump] " .. tostring(msg))
end

local function enabled()
	return configui.getValue("shotgun_manual_pump") ~= false
end

local function isShotgunWeapon(weapon)
	if weapon == nil then
		return false
	end
	local ok, name = pcall(function()
		return weapon:get_full_name()
	end)
	return ok and name ~= nil and string.find(name, "WeaponShotgun", 1, true) ~= nil
end

local function getLocalPawn()
	return uevrUtils.get_local_pawn and uevrUtils.get_local_pawn() or uevr.api:get_local_pawn(0)
end

local function getEquippedWeapon(pawn)
	return uevrUtils.getValid(pawn or getLocalPawn(), {
		"Mesh",
		"AnimScriptInstance",
		"WeaponManageCmbSubcomp",
		"EquippedWeapon",
	})
end

local function getEquippedShotgun()
	local weapon = getEquippedWeapon()
	if isShotgunWeapon(weapon) then
		return weapon
	end
	return nil
end

local function getWeaponMesh(weapon)
	return uevrUtils.getValid(weapon, { "Mesh" })
end

local function findAsset(path)
	local obj = nil
	pcall(function()
		obj = uevr.api:find_uobject(path)
	end)
	return obj
end

local function getPrepareMontage()
	if state.prepareMontage ~= nil and uevrUtils.getValid(state.prepareMontage) ~= nil then
		return state.prepareMontage
	end
	state.prepareMontage = findAsset(C.PREPARE_MONTAGE)
	return state.prepareMontage
end

local function getPrepareBodyAnim()
	if state.prepareBodyAnim ~= nil and uevrUtils.getValid(state.prepareBodyAnim) ~= nil then
		return state.prepareBodyAnim
	end
	state.prepareBodyAnim = findAsset(C.PREPARE_BODY_ANIM)
	return state.prepareBodyAnim
end

local function getPrepareReadyNotify()
	if state.prepareReadyNotify ~= nil and uevrUtils.getValid(state.prepareReadyNotify) ~= nil then
		return state.prepareReadyNotify
	end
	state.prepareReadyNotify = findAsset(C.PREPARE_READY_NOTIFY)
	return state.prepareReadyNotify
end

local function readSoft(weapon, offset)
	if weapon == nil then
		return nil
	end
	local bytes = {}
	for i = 0, C.SOFT_BYTES - 4, 4 do
		local v = readI32(weapon, offset + i)
		if v == nil then
			return nil
		end
		bytes[#bytes + 1] = v
	end
	return bytes
end

local function writeSoft(weapon, offset, dwords)
	if weapon == nil or dwords == nil then
		return false
	end
	for i, v in ipairs(dwords) do
		if not writeI32(weapon, offset + (i - 1) * 4, v) then
			return false
		end
	end
	return true
end

local function zeroSoft(weapon, offset)
	for i = 0, C.SOFT_BYTES - 4, 4 do
		writeI32(weapon, offset + i, 0)
	end
end

local function stopPrepareVisuals(weapon)
	local mesh = getWeaponMesh(weapon)
	local prepareMontage = getPrepareMontage()
	pcall(function()
		local anim = mesh and mesh.AnimScriptInstance
		if anim == nil then
			return
		end
		if prepareMontage ~= nil and anim.Montage_Stop ~= nil then
			if anim.Montage_IsPlaying == nil or anim:Montage_IsPlaying(prepareMontage) then
				anim:Montage_Stop(0.0, prepareMontage)
			end
		end
	end)
	local pawn = getLocalPawn()
	local charMesh = pawn and pawn.Mesh or nil
	pcall(function()
		local anim = charMesh and charMesh.AnimScriptInstance
		if anim ~= nil and anim.StopSlotAnimation ~= nil then
			anim:StopSlotAnimation(0.0, uevrUtils.fname_from_string(C.PREPARE_SLOT))
		end
	end)
end

local function softBytesLookValid(dwords)
	if dwords == nil then
		return false
	end
	for _, v in ipairs(dwords) do
		if v ~= nil and v ~= 0 then
			return true
		end
	end
	return false
end

local function saveAndStrip(weapon)
	if weapon == nil or state.strippedWeapon == weapon then
		return
	end
	state.savedPrepareWeaponAnimBytes = readSoft(weapon, C.PREPARE_WEAPON_ANIM_OFF)
	state.savedPrepareAnimDataBytes = readSoft(weapon, C.PREPARE_ANIM_DATA_OFF)
	pcall(function()
		weapon.SingleActionPrepareWeaponAnim = nil
	end)
	pcall(function()
		local data = weapon.SingleActionPrepareAnimData
		if data ~= nil then
			data.Animation = nil
		end
	end)
	zeroSoft(weapon, C.PREPARE_WEAPON_ANIM_OFF)
	zeroSoft(weapon, C.PREPARE_ANIM_DATA_OFF)
	state.strippedWeapon = weapon
end

local function restorePrepare(weapon)
	weapon = weapon or state.strippedWeapon
	if weapon == nil then
		state.strippedWeapon = nil
		return
	end

	local restored = false
	if softBytesLookValid(state.savedPrepareWeaponAnimBytes) then
		writeSoft(weapon, C.PREPARE_WEAPON_ANIM_OFF, state.savedPrepareWeaponAnimBytes)
		restored = true
	end
	if softBytesLookValid(state.savedPrepareAnimDataBytes) then
		writeSoft(weapon, C.PREPARE_ANIM_DATA_OFF, state.savedPrepareAnimDataBytes)
		restored = true
	end

	-- Fallback: assign live assets (PrepareEnd(nil) alone does not unlock — notify path needs assets).
	local montage = getPrepareMontage()
	local bodyAnim = getPrepareBodyAnim()
	local meshAnim = findAsset(C.PREPARE_WEAPON_MESH_ANIM)
	pcall(function()
		if montage ~= nil then
			weapon.SingleActionPrepareWeaponAnim = montage
			restored = true
		elseif meshAnim ~= nil then
			weapon.SingleActionPrepareWeaponAnim = meshAnim
			restored = true
		end
	end)
	pcall(function()
		local data = weapon.SingleActionPrepareAnimData
		if data ~= nil and bodyAnim ~= nil then
			data.Animation = bodyAnim
			data.SlotName = uevrUtils.fname_from_string(C.PREPARE_SLOT)
			-- Body prepare has locomotion/root that spins James in VR if replayed.
			if data.UseRootMotion ~= nil then
				data.UseRootMotion = false
			end
			weapon.SingleActionPrepareAnimData = data
			restored = true
		end
	end)

	state.strippedWeapon = nil
	state.savedPrepareWeaponAnimBytes = nil
	state.savedPrepareAnimDataBytes = nil
	return restored
end

local function getRangedSubcomp()
	if state.cachedRanged ~= nil and uevrUtils.getValid(state.cachedRanged) ~= nil then
		return state.cachedRanged
	end
	state.cachedRanged = nil
	local ok, comps = pcall(function()
		return uevrUtils.find_all_of("Class /Script/SHProto.SHRangedCmbSubcomp", false) or {}
	end)
	if not ok then
		return nil
	end
	for _, c in ipairs(comps) do
		local okN, name = pcall(function()
			return c:get_full_name()
		end)
		if okN and name ~= nil and string.find(name, "SHCharacterPlay", 1, true) ~= nil then
			state.cachedRanged = c
			return c
		end
	end
	return nil
end

-- Weapon-mesh pump only. Do NOT PlaySlot the body prepare — it restarts while
-- the offhand stays pulled and spins James / spam-reloads in VR.
local function playWeaponPumpFeedback(weapon)
	local montage = getPrepareMontage()
	local mesh = getWeaponMesh(weapon)
	local wAnim = mesh and mesh.AnimScriptInstance or nil
	if wAnim == nil or montage == nil or wAnim.Montage_Play == nil then
		return
	end
	pcall(function()
		if wAnim.Montage_IsPlaying ~= nil and wAnim:Montage_IsPlaying(montage) then
			return
		end
		wAnim:Montage_Play(montage, 1.0)
	end)
end

local function stopBodyPrepareSlot()
	local pawn = getLocalPawn()
	local charAnim = uevrUtils.getValid(pawn, { "Mesh", "AnimScriptInstance" })
	pcall(function()
		if charAnim ~= nil and charAnim.StopSlotAnimation ~= nil then
			charAnim:StopSlotAnimation(0.05, uevrUtils.fname_from_string(C.PREPARE_SLOT))
		end
	end)
end

local function firePrepareReadyNotify()
	local notify = getPrepareReadyNotify()
	local pawn = getLocalPawn()
	local mesh = pawn and pawn.Mesh or nil
	local bodyAnim = getPrepareBodyAnim()
	if notify == nil or mesh == nil or bodyAnim == nil then
		return false
	end
	local ok = pcall(function()
		notify:Received_Notify(mesh, bodyAnim)
	end)
	if not ok then
		ok = pcall(function()
			notify:ProcessEvent(notify.Received_Notify, {
				MeshComp = mesh,
				Animation = bodyAnim,
			})
		end)
	end
	return ok == true
end

local function callPrepareEnd(montage)
	local ranged = getRangedSubcomp()
	if ranged == nil or ranged.OnRangedSingleActionPrepareEnd == nil then
		return false
	end
	local ok = pcall(function()
		ranged:OnRangedSingleActionPrepareEnd(montage, 0)
	end)
	if not ok then
		ok = pcall(function()
			ranged:ProcessEvent(ranged.OnRangedSingleActionPrepareEnd, {
				Montage = montage,
				AnimEndType = 0,
			})
		end)
	end
	return ok == true
end

local function isWeaponFireReady(weapon)
	if weapon == nil or weapon.IsFireReady == nil then
		return false
	end
	local ok, ready = pcall(function()
		return weapon:IsFireReady()
	end)
	return ok and ready == true
end

local function resetPumpPull()
	state.pumpGripping = false
	state.pumpProgress = 0.0
	state.pumpProgressAtGrab = 0.0
	state.pumpGrabAlong = 0.0
	state.pumpOriginWs = nil
	state.pumpAxisWs = nil
end

local function lockAfterShot(weapon, reason)
	if not enabled() then
		return
	end
	if not isShotgunWeapon(weapon) then
		return
	end
	-- Ignore only while RT is still held after pump (would auto-fire on unlock).
	-- Once RT is released, the next OnFirePerformed is a real shot — must re-lock.
	if state.awaitRtRelease then
		diag(string.format("ignored lock (%s) — still awaiting RT release", reason or "?"))
		return
	end
	state.needsPump = true
	state.unlockPlaying = false
	state.suppressLockUntil = 0.0
	state.awaitRtRelease = false
	state.needsPumpSince = os.clock()
	state.diagShotCount = (state.diagShotCount or 0) + 1
	resetPumpPull()
	saveAndStrip(weapon)
	stopPrepareVisuals(weapon)
	setStatus("needs pump — grip + pull back only")
	diag(string.format("LOCKED shot=#%d (%s)", state.diagShotCount, reason or "shot"))
end

local function unlock(reason)
	if not state.needsPump or state.unlockPlaying then
		diag(string.format("unlock skipped needsPump=%s unlockPlaying=%s (%s)",
			tostring(state.needsPump), tostring(state.unlockPlaying), tostring(reason)))
		return
	end
	-- Latch immediately so pull-held frames / shell notifies cannot re-enter.
	state.needsPump = false
	state.unlockPlaying = true
	state.awaitRtRelease = true
	state.suppressLockUntil = 0.0
	state.needsPumpSince = 0.0
	state.diagUnlockCount = (state.diagUnlockCount or 0) + 1
	resetPumpPull()
	local weapon = state.strippedWeapon or getEquippedShotgun()

	-- Clear locked leftovers once, then restore — do NOT keep stopping body prepare.
	stopPrepareVisuals(weapon)
	playWeaponPumpFeedback(weapon)
	restorePrepare(weapon)
	firePrepareReadyNotify()
	callPrepareEnd(getPrepareMontage())

	setStatus("chambered — release trigger, then fire")
	diag(string.format("UNLOCKED #%d (%s) — release RT then fire", state.diagUnlockCount, reason or "pump pull"))
	pcall(function()
		delay(1500, function()
			state.unlockPlaying = false
			diag("unlockPlaying cleared after delay")
			if not state.needsPump and not state.awaitRtRelease then
				setStatus("idle")
			elseif not state.needsPump then
				setStatus("chambered — release trigger, then fire")
			end
		end)
	end)
end

function M.isBlockingFire()
	return enabled() and (state.needsPump or state.awaitRtRelease) and getEquippedShotgun() ~= nil
end

function M.isHoldingPump()
	return state.stuck == true or state.pumpGripping == true
end

function M.dumpDiag(tag)
	local weapon = getEquippedShotgun()
	local info = {
		tag = tag or "manual",
		t = os.clock(),
		enabled = enabled(),
		needsPump = state.needsPump,
		awaitRtRelease = state.awaitRtRelease,
		unlockPlaying = state.unlockPlaying,
		stuck = state.stuck,
		pumpGripping = state.pumpGripping,
		leftGripDown = state.leftGripDown,
		pumpProgress = state.pumpProgress,
		statusText = state.statusText,
		shotCount = state.diagShotCount,
		unlockCount = state.diagUnlockCount,
		needsPumpFor = (state.needsPump and state.needsPumpSince > 0) and (os.clock() - state.needsPumpSince) or 0,
		weapon = nil,
		hasAmmo = nil,
		fireReady = nil,
		clip = nil,
		holdingShell = nil,
		attachment = nil,
		nearPump = nil,
		events = {},
	}
	pcall(function()
		if weapon ~= nil then
			info.weapon = weapon:get_full_name()
			if weapon.HasAnyAmmoInClip ~= nil then
				info.hasAmmo = weapon:HasAnyAmmoInClip()
			end
			if weapon.IsFireReady ~= nil then
				info.fireReady = weapon:IsFireReady()
			end
			if weapon.read_dword ~= nil then
				info.clip = weapon:read_dword(0xBB4)
			end
		end
	end)
	pcall(function()
		local reload = package.loaded["shotgun_reload"]
		if reload ~= nil and reload.isHoldingShell ~= nil then
			info.holdingShell = reload.isHoldingShell()
		end
	end)
	pcall(function()
		local attachment = attachments.getCurrentGrippedAttachment(Handed.Right)
		info.attachment = attachment ~= nil
	end)
	for i = 1, #state.diagEvents do
		info.events[i] = state.diagEvents[i]
	end
	print("[shotgun_pump] ===== DUMP " .. tostring(tag) .. " =====")
	print(string.format(
		"[shotgun_pump] needs=%s awaitRT=%s unlockPlay=%s stuck=%s grip=%s prog=%.2f shots=%d unlocks=%d shell=%s near=%s ready=%s ammo=%s clip=%s",
		tostring(info.needsPump),
		tostring(info.awaitRtRelease),
		tostring(info.unlockPlaying),
		tostring(info.stuck),
		tostring(info.leftGripDown),
		tonumber(info.pumpProgress) or 0,
		tonumber(info.shotCount) or 0,
		tonumber(info.unlockCount) or 0,
		tostring(info.holdingShell),
		tostring(info.nearPump),
		tostring(info.fireReady),
		tostring(info.hasAmmo),
		tostring(info.clip)
	))
	for i = 1, #info.events do
		print("[shotgun_pump] hist " .. info.events[i])
	end
	pcall(function()
		local path = (os.getenv("APPDATA") or ".") .. "/UnrealVRMod/SHProto-Win64-Shipping/shotgun_pump_diag.txt"
		local f = io.open(path, "w")
		if f ~= nil then
			f:write(string.format("tag=%s t=%.3f\n", tostring(info.tag), info.t))
			f:write(string.format("needsPump=%s awaitRtRelease=%s unlockPlaying=%s\n",
				tostring(info.needsPump), tostring(info.awaitRtRelease), tostring(info.unlockPlaying)))
			f:write(string.format("stuck=%s pumpGripping=%s leftGrip=%s progress=%.3f status=%s\n",
				tostring(info.stuck), tostring(info.pumpGripping), tostring(info.leftGripDown),
				tonumber(info.pumpProgress) or 0, tostring(info.statusText)))
			f:write(string.format("shots=%d unlocks=%d needsFor=%.2fs holdingShell=%s nearPump=%s\n",
				tonumber(info.shotCount) or 0, tonumber(info.unlockCount) or 0,
				tonumber(info.needsPumpFor) or 0, tostring(info.holdingShell), tostring(info.nearPump)))
			f:write(string.format("weapon=%s hasAmmo=%s fireReady=%s clip=%s\n",
				tostring(info.weapon), tostring(info.hasAmmo), tostring(info.fireReady), tostring(info.clip)))
			f:write("--- history ---\n")
			for i = 1, #info.events do
				f:write(info.events[i] .. "\n")
			end
			f:close()
			print("[shotgun_pump] wrote shotgun_pump_diag.txt")
		end
	end)
	return info
end

function M.reset(reason)
	local weapon = state.strippedWeapon or getEquippedShotgun()
	state.needsPump = false
	state.unlockPlaying = false
	state.suppressLockUntil = 0.0
	state.awaitRtRelease = false
	state.stuck = false
	state.wasStuck = false
	resetPumpPull()
	restorePrepare(weapon)
	setStatus(reason or "idle")
end

-- Lazy require: rifle_bolt also needs PrepareEnd/IsFireReady, but UEVR hook_ptr
-- replaces — shotgun owns the shared hooks and consults rifle here.
local function getRifleBolt()
	local ok, mod = pcall(require, "rifle_bolt")
	if ok then
		return mod
	end
	return nil
end

local function prepareEndPre(fn, obj, locals, result)
	local rifle = getRifleBolt()
	if rifle ~= nil and rifle.shouldBlockPrepareEnd ~= nil then
		local okR, blockR = pcall(function()
			return rifle.shouldBlockPrepareEnd()
		end)
		if okR and blockR then
			return false
		end
	end
	if not enabled() then
		return true
	end
	if not state.needsPump then
		return true
	end
	if getEquippedShotgun() == nil then
		return true
	end
	return false
end

local function firePost(fn, obj, locals, result)
	if not enabled() then
		return
	end
	if not isShotgunWeapon(obj) then
		return
	end
	lockAfterShot(obj, "OnFirePerformed")
end

-- Never lock on shell eject — unlock feedback montage was re-locking every pump
-- (log: unlocked → locked OnShellEjected).
local function shellPost(fn, obj, locals, result)
	if not enabled() then
		return
	end
	if not isShotgunWeapon(obj) then
		return
	end
	if state.needsPump and not state.unlockPlaying then
		stopPrepareVisuals(obj)
	end
end

-- UEVR docs: locals = StructObject*, result = void*.
-- Bool returns must be written to locals.ReturnValue (result is not a Lua table).
local function setHookBoolReturn(locals, result, value)
	local ready = value and true or false
	local okLocals = pcall(function()
		locals.ReturnValue = ready
	end)
	pcall(function()
		if locals ~= nil and locals.set_property ~= nil then
			locals:set_property("ReturnValue", ready)
		end
	end)
	-- Best-effort for native thunks that read the void* result pointer.
	pcall(function()
		if result ~= nil and result.write_byte ~= nil then
			result:write_byte(0, ready and 1 or 0)
		elseif result ~= nil and result.write_bool ~= nil then
			result:write_bool(0, ready)
		end
	end)
	return okLocals
end

local function shotgunFireAllowed()
	return (not state.needsPump) and (not state.awaitRtRelease)
end

local function isFireReadyPre(fn, obj, locals, result)
	local rifle = getRifleBolt()
	if rifle ~= nil and rifle.shouldForceNotReady ~= nil then
		local okR, forceNot = pcall(function()
			return rifle.shouldForceNotReady(obj)
		end)
		if okR and forceNot then
			setHookBoolReturn(locals, result, false)
			return false
		end
	end
	if not enabled() or not isShotgunWeapon(obj) then
		return true
	end
	setHookBoolReturn(locals, result, shotgunFireAllowed())
	-- Skip native — gate is fully ours while manual pump is on.
	return false
end

local function isFireReadyPost(fn, obj, locals, result)
	local rifle = getRifleBolt()
	if rifle ~= nil and rifle.shouldForceNotReady ~= nil then
		local okR, forceNot = pcall(function()
			return rifle.shouldForceNotReady(obj)
		end)
		if okR and forceNot then
			setHookBoolReturn(locals, result, false)
			return
		end
	end
	if not enabled() or not isShotgunWeapon(obj) then
		return
	end
	setHookBoolReturn(locals, result, shotgunFireAllowed())
end

local function ensureHooks()
	if not state.prepareHooked then
		local ok = hook_function(
			"Class /Script/SHProto.SHRangedCmbSubcomp",
			"OnRangedSingleActionPrepareEnd",
			true,
			prepareEndPre,
			nil,
			false
		)
		state.prepareHooked = ok == true
		if ok then
			print("[shotgun_pump] OnRangedSingleActionPrepareEnd blocked while needsPump")
		else
			print("[shotgun_pump] WARNING: failed to hook OnRangedSingleActionPrepareEnd")
		end
	end
	if not state.readyHooked then
		-- Prefer non-native first so locals.ReturnValue is a real StructObject field.
		local ok = hook_function(
			"Class /Script/SHProto.SHItemWeaponRanged",
			"IsFireReady",
			false,
			isFireReadyPre,
			isFireReadyPost,
			false
		)
		if not ok then
			ok = hook_function(
				"Class /Script/SHProto.SHItemWeaponRanged",
				"IsFireReady",
				true,
				isFireReadyPre,
				isFireReadyPost,
				false
			)
		end
		if not ok then
			ok = hook_function(C.BP_CLASS, "IsFireReady", false, isFireReadyPre, isFireReadyPost, false)
		end
		state.readyHooked = ok == true
		if ok then
			print("[shotgun_pump] IsFireReady hooked via locals.ReturnValue (pump gate)")
		else
			print("[shotgun_pump] WARNING: failed to hook IsFireReady")
		end
	end
	if not state.fireHooked then
		local ok = hook_function(C.BP_CLASS, "OnFirePerformed", false, nil, firePost, false)
		if not ok then
			ok = hook_function(
				"Class /Script/SHProto.SHItemWeaponRanged",
				"OnFirePerformed",
				false,
				nil,
				firePost,
				false
			)
		end
		state.fireHooked = ok == true
		if ok then
			print("[shotgun_pump] OnFirePerformed hooked")
		else
			print("[shotgun_pump] WARNING: failed to hook OnFirePerformed")
		end
	end
	if not state.shellHooked then
		local ok = hook_function(C.BP_CLASS, "OnShellEjected", false, nil, shellPost, false)
		state.shellHooked = ok == true
		if ok then
			print("[shotgun_pump] OnShellEjected hooked (visual stop only — never locks)")
		end
	end
end

local function disableProximityAutoStick()
	local list = accessories.getAccessoriesForAttachment(C.ATTACHMENT_ID)
	local acc = list and list[C.ACCESSORY_ID] or nil
	local marker = acc and acc.markers and acc.markers[1] or nil
	if marker == nil then
		print("[shotgun_pump] no pump marker found — cannot disable proximity")
		return
	end
	if marker.activation_hand ~= 1 then
		marker.activation_hand = 1
		print("[shotgun_pump] proximity auto-stick disabled (activation_hand=None)")
	end
end

local function getMarkerAndAttachment()
	local attachment = attachments.getCurrentGrippedAttachment(Handed.Right)
	if attachment == nil then
		return nil, nil, nil
	end
	local attachmentID = attachments.getAttachmentIDFromAttachment(attachment)
	if attachmentID == nil or attachmentID == "" then
		return nil, nil, nil
	end
	if string.find(attachmentID, "WeaponShotgun", 1, true) == nil then
		return nil, nil, nil
	end
	local list = accessories.getAccessoriesForAttachment(attachmentID)
	local acc = list and list[C.ACCESSORY_ID] or nil
	local marker = acc and (accessories.getPrimaryMarkerParams(acc) or (acc.markers and acc.markers[1])) or nil
	return attachment, attachmentID, marker
end

local function socketWorldLoc(attachment, socketName)
	if attachment == nil or attachment.GetSocketLocation == nil then
		return nil
	end
	local ok, loc = pcall(function()
		return attachment:GetSocketLocation(uevrUtils.fname_from_string(socketName))
	end)
	if ok then
		return loc
	end
	return nil
end

local function isLeftNearPump(attachment, marker)
	if attachment == nil or marker == nil then
		return false
	end
	local leftLoc = controllers.getControllerLocation(Handed.Left)
	if leftLoc == nil then
		return false
	end
	local activationDistance = marker.activation_distance or C.DEFAULT_DIST
	if activationDistance == nil or activationDistance <= 0 then
		activationDistance = C.DEFAULT_DIST
	end
	local uiDist = configui.getValue("shotgun_pump_grab_dist")
	if uiDist ~= nil and tonumber(uiDist) ~= nil and tonumber(uiDist) > 0 then
		activationDistance = tonumber(uiDist)
	end
	local socketName = marker.socket_name or C.PUMP_SOCKET
	local targetLoc = nil
	local targetRot = nil
	if socketName ~= "" and attachment.GetSocketLocation ~= nil then
		targetLoc = attachment:GetSocketLocation(uevrUtils.fname_from_string(socketName))
		targetRot = attachment:GetSocketRotation(uevrUtils.fname_from_string(socketName))
	elseif attachment.K2_GetComponentLocation ~= nil then
		targetLoc = attachment:K2_GetComponentLocation()
		targetRot = attachment:K2_GetComponentRotation()
	end
	if targetLoc == nil then
		return false
	end
	local loc = uevrUtils.vector(marker.location or { 0, 0, 0 })
	targetLoc = targetLoc + uevrUtils.rotateVector(loc, targetRot)
	local d = uevrUtils.distanceBetween(leftLoc, targetLoc)
	return d ~= nil and d <= activationDistance
end

local function capturePumpBasis(attachment)
	local pump = socketWorldLoc(attachment, C.PUMP_SOCKET)
	if pump == nil and attachment.K2_GetComponentLocation ~= nil then
		pump = attachment:K2_GetComponentLocation()
	end
	if pump == nil then
		return false
	end
	-- Axis points REARWARD along the barrel (muzzle → pump). Pulling toward the
	-- stock increases progress — no forward stroke required.
	local ax, ay, az = -1.0, 0.0, 0.0
	local muzzle = socketWorldLoc(attachment, C.MUZZLE_SOCKET)
	if muzzle ~= nil then
		ax = pump.X - muzzle.X
		ay = pump.Y - muzzle.Y
		az = pump.Z - muzzle.Z
	else
		local ok, rot = pcall(function()
			return attachment:K2_GetComponentRotation()
		end)
		if ok and rot ~= nil then
			local yaw = math.rad(rot.Yaw or 0)
			ax, ay, az = -math.cos(yaw), -math.sin(yaw), 0.0
		end
	end
	local len = math.sqrt(ax * ax + ay * ay + az * az)
	if len < 0.001 then
		ax, ay, az = -1.0, 0.0, 0.0
		len = 1.0
	end
	ax, ay, az = ax / len, ay / len, az / len
	state.pumpAxisWs = { X = ax, Y = ay, Z = az }
	state.pumpOriginWs = { X = pump.X, Y = pump.Y, Z = pump.Z }
	return true
end

local function projectHandOnPump(leftLoc)
	if leftLoc == nil or state.pumpOriginWs == nil or state.pumpAxisWs == nil then
		return 0.0
	end
	local o = state.pumpOriginWs
	local a = state.pumpAxisWs
	return (leftLoc.X - o.X) * a.X + (leftLoc.Y - o.Y) * a.Y + (leftLoc.Z - o.Z) * a.Z
end

local function updatePumpPull(attachment)
	if not state.needsPump or attachment == nil then
		return
	end
	local leftLoc = controllers.getControllerLocation(Handed.Left)
	if leftLoc == nil or not state.leftGripDown then
		state.pumpGripping = false
		if state.stuck then
			setStatus("stuck — pull back only")
		end
		return
	end

	if not state.pumpGripping then
		if not capturePumpBasis(attachment) then
			return
		end
		state.pumpGripping = true
		state.pumpGrabAlong = projectHandOnPump(leftLoc)
		state.pumpProgressAtGrab = 0.0
		state.pumpProgress = 0.0
	end

	-- Keep grab origin/axis from grip start so hand can slide rearward relative to the gun.
	local along = projectHandOnPump(leftLoc)
	-- Axis is rearward: only motion toward the stock counts (forward is ignored).
	local deltaAlong = along - (state.pumpGrabAlong or 0.0)
	if deltaAlong < 0.0 then
		deltaAlong = 0.0
	end
	local progress = deltaAlong / C.PUMP_TRAVEL
	if progress > 1.0 then
		progress = 1.0
	end
	state.pumpProgress = progress
	setStatus(string.format("pull back %.0f%%", progress * 100.0))
	local now = os.clock()
	if progress >= 0.25 and (now - (state.lastPullLog or 0) > 0.35) then
		state.lastPullLog = now
		diag(string.format("pull %.0f%% stuck=%s grip=%s", progress * 100.0, tostring(state.stuck), tostring(state.leftGripDown)))
	end
	if progress >= 0.85 then
		if state.needsPump and not state.unlockPlaying then
			unlock(string.format("pull back %.0f%%", progress * 100.0))
		end
	end
end

function M.init()
	if state.initialized then
		return
	end
	state.initialized = true

	disableProximityAutoStick()
	ensureHooks()
	setStatus("idle")

	uevrUtils.registerUEVRCallback("active_left_accessory", function()
		local attachment, _, marker = getMarkerAndAttachment()
		if attachment == nil then
			state.stuck = false
			state.wasStuck = false
			state.pumpGripping = false
			return
		end
		if not state.leftGripDown then
			state.stuck = false
			state.wasStuck = false
			state.pumpGripping = false
			return
		end
		local nowStuck = state.stuck or isLeftNearPump(attachment, marker)
		if nowStuck then
			state.stuck = true
			state.wasStuck = true
			-- Stick only — unlock requires rearward pull while stuck.
			if enabled() and state.needsPump then
				updatePumpPull(attachment)
			end
			return C.ACCESSORY_ID, C.ACCESSORY_PRIORITY
		end
		state.wasStuck = false
		state.pumpGripping = false
	end)

	uevrUtils.registerPreEngineTickCallback(function(engine, delta)
		if not enabled() then
			return
		end
		-- Only suppress auto-prepare while locked. After unlock, allow native prepare
		-- to complete so IsFireReady can go true (hook is backup).
		if not state.needsPump then
			return
		end
		local weapon = getEquippedShotgun()
		if weapon == nil then
			M.reset("unequipped")
			return
		end
		stopPrepareVisuals(weapon)
	end)

	uevrUtils.registerLevelChangeCallback(function()
		M.reset("level change")
		disableProximityAutoStick()
	end)

	uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, stateIn)
		local grip = sh2vrLeftGrip(stateIn)
		state.leftGripDown = grip

		if not enabled() then
			if not grip then
				state.wasStuck = false
				state.pumpGripping = false
			end
			return
		end

		local rt = 0
		if stateIn ~= nil and stateIn.Gamepad ~= nil then
			rt = stateIn.Gamepad.bRightTrigger or 0
		end

		-- Pump pull while locked (read raw RT before we eat it later at prio -5).
		if state.needsPump and not state.awaitRtRelease then
			local attachment, _, marker = getMarkerAndAttachment()
			if attachment ~= nil and grip and (state.stuck or isLeftNearPump(attachment, marker)) then
				state.stuck = true
				state.wasStuck = true
				updatePumpPull(attachment)
				if XINPUT_GAMEPAD_LEFT_SHOULDER ~= nil then
					uevrUtils.unpressButton(stateIn, XINPUT_GAMEPAD_LEFT_SHOULDER)
				end
			elseif not grip then
				state.wasStuck = false
				state.pumpGripping = false
			end
		elseif not grip then
			state.wasStuck = false
			state.pumpGripping = false
		end

		-- Track RT release for awaitRtRelease (actual eat happens at prio -5 after aim remap).
		if state.awaitRtRelease and rt < 64 then
			state.awaitRtRelease = false
			state.unlockPlaying = false
			diag("RT released — next trigger pull can fire")
			if not state.needsPump then
				setStatus("chambered")
			end
		end
	end, 12)

	-- AFTER main auto-aim: kill LT+RT so right-grip cannot show aim / fire while pumping.
	uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, stateIn)
		if not M.isBlockingFire() then
			return
		end
		if stateIn ~= nil and stateIn.Gamepad ~= nil then
			stateIn.Gamepad.bRightTrigger = 0
			stateIn.Gamepad.bLeftTrigger = 0
		end
	end, -5)

	-- Auto-dump if locked for too long without unlock (stuck pump session).
	uevrUtils.registerPreEngineTickCallback(function(engine, delta)
		if not enabled() or not state.needsPump then
			return
		end
		if state.needsPumpSince <= 0 then
			state.needsPumpSince = os.clock()
		end
		local held = os.clock() - state.needsPumpSince
		if held > 4.0 and (os.clock() - (state.lastPullLog or 0) > 2.0) then
			state.lastPullLog = os.clock()
			M.dumpDiag(string.format("auto-stuck-%.0fs", held))
		end
	end)

	configui.onCreateOrUpdate("shotgun_manual_pump", function(value)
		if value == false then
			M.reset("disabled")
		end
	end)

	print("[shotgun_pump] initialized (diag ON — dump when stuck)")
end

return M
