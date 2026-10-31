--------------------------------------------------------------------------
-- Rifle manual bolt (MCP-confirmed).
-- Separate module (Lua 200-local limit on main.lua).
--
-- Confirmed fire chain:
--   WeaponRifle_BP_C.OnFirePerformed
--     → OnShellEjected
--     → UpperBodyAnim_WeaponPrepare
--     → AnimNotify_SingleActionPrepareReady (James_Rifle_Recoil_Separate_Overload)
--     → OnRangedSingleActionPrepareEnd
--
-- Step 1: lock auto prepare after shot (strip soft refs + block prepare).
-- Step 2: left grip near bone_lock + pull rearward (muzzle→lock) → unlock.
--
-- Mesh bones (MCP): Holder, bone_root, bone_shell, bone_bullet, bone_lock,
--   bone_safety, bone_trigger, FX_muzzle, Holder_L, FX_Chamber.
-- Accessory: SHItemBase_Sniper_Rifle_01_Skeletal / a7c2e91f-… / bone_lock.
--
-- Shared PrepareEnd / IsFireReady hooks live in shotgun_pump.lua (consults us).
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
	BP_CLASS = "BlueprintGeneratedClass /Game/Game/Gameplay/Weapons/Rifle/WeaponRifle_BP.WeaponRifle_BP_C",
	ATTACHMENT_ID = "SHItemBase_Sniper_Rifle_01_Skeletal",
	ACCESSORY_ID = "a7c2e91f-4b18-4d55-9e01-boltlock0001",
	ACCESSORY_PRIORITY = 2,
	DEFAULT_DIST = 16.0,
	BOLT_TRAVEL = 10.0,
	PREPARE_WEAPON_ANIM_OFF = 0xC38,
	PREPARE_ANIM_DATA_OFF = 0xC68,
	SOFT_BYTES = 48,
	PREPARE_SLOT = "UpperBodyWeaponPrepare",
	PREPARE_BODY_ANIM = "AnimSequence /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Rifle/RecoilAndReload/James_Rifle_Recoil_Separate_Overload.James_Rifle_Recoil_Separate_Overload",
	PREPARE_READY_NOTIFY = "AnimNotify_SingleActionPrepareReady /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Rifle/RecoilAndReload/James_Rifle_Recoil_Separate_Overload.James_Rifle_Recoil_Separate_Overload.AnimNotify_SingleActionPrepareReady_0",
	BOLT_SOCKET = "bone_lock",
	MUZZLE_SOCKET = "FX_muzzle",
}

local state = {
	initialized = false,
	needsBolt = false,
	awaitRtRelease = false,
	unlockPlaying = false,
	leftGripDown = false,
	stuck = false,
	wasStuck = false,
	boltGripping = false,
	boltProgress = 0.0,
	boltGrabAlong = 0.0,
	boltOriginWs = nil,
	boltAxisWs = nil,
	fireHooked = false,
	shellHooked = false,
	prepareAnimHooked = false,
	strippedWeapon = nil,
	savedPrepareWeaponAnimBytes = nil,
	savedPrepareAnimDataBytes = nil,
	statusText = "idle",
	prepareBodyAnim = nil,
	prepareReadyNotify = nil,
	cachedRanged = nil,
	lastPullLog = 0.0,
	wasRightThumbrest = false,
	-- After chamber/fire: ignore bolt stick/pull until left grip is released.
	-- Prevents recoil while still gripping from auto-unlocking the next shot.
	awaitLeftGripRelease = false,
	thumbrestUnlockAfter = 0.0,
	-- After shell insert near FX_Chamber (overlaps bone_lock): block unlock briefly.
	suppressUnlockUntil = 0.0,
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
		configui.setLabel("rifle_bolt_status", "Rifle bolt: " .. state.statusText)
	end)
end

local function enabled()
	return configui.getValue("rifle_manual_bolt") ~= false
end

local function isRifleWeapon(weapon)
	if weapon == nil then
		return false
	end
	local ok, name = pcall(function()
		return weapon:get_full_name()
	end)
	return ok and name ~= nil and string.find(name, "WeaponRifle", 1, true) ~= nil
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

local function getEquippedRifle()
	local weapon = getEquippedWeapon()
	if isRifleWeapon(weapon) then
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

local function stopPrepareVisuals(weapon)
	local pawn = getLocalPawn()
	local charMesh = pawn and pawn.Mesh or nil
	pcall(function()
		local anim = charMesh and charMesh.AnimScriptInstance
		if anim ~= nil and anim.StopSlotAnimation ~= nil then
			anim:StopSlotAnimation(0.0, uevrUtils.fname_from_string(C.PREPARE_SLOT))
		end
	end)
	local mesh = getWeaponMesh(weapon)
	pcall(function()
		local anim = mesh and mesh.AnimScriptInstance
		if anim ~= nil and anim.StopSlotAnimation ~= nil then
			anim:StopSlotAnimation(0.0, uevrUtils.fname_from_string(C.PREPARE_SLOT))
		end
	end)
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
		return false
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

	local bodyAnim = getPrepareBodyAnim()
	pcall(function()
		local data = weapon.SingleActionPrepareAnimData
		if data ~= nil and bodyAnim ~= nil then
			data.Animation = bodyAnim
			data.SlotName = uevrUtils.fname_from_string(C.PREPARE_SLOT)
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

local function callPrepareEnd()
	local ranged = getRangedSubcomp()
	if ranged == nil or ranged.OnRangedSingleActionPrepareEnd == nil then
		return false
	end
	local ok = pcall(function()
		ranged:OnRangedSingleActionPrepareEnd(nil, 0)
	end)
	if not ok then
		ok = pcall(function()
			ranged:ProcessEvent(ranged.OnRangedSingleActionPrepareEnd, {
				Montage = nil,
				AnimEndType = 0,
			})
		end)
	end
	return ok == true
end

local function resetBoltPull()
	state.boltGripping = false
	state.boltProgress = 0.0
	state.boltGrabAlong = 0.0
	state.boltOriginWs = nil
	state.boltAxisWs = nil
end

local function isRightThumbrestTouched()
	local ok, touched = pcall(function()
		return uevrUtils.isThumbpadTouched(nil, Handed.Right)
	end)
	return ok and touched == true
end

local function isHoldingShellNow()
	local ok, holding = pcall(function()
		local reload = package.loaded["rifle_reload"] or require("rifle_reload")
		return reload ~= nil and reload.isHoldingShell ~= nil and reload.isHoldingShell()
	end)
	return ok and holding == true
end

local function isUnlockSuppressed()
	if os.clock() < (state.suppressUnlockUntil or 0.0) then
		return true
	end
	if isHoldingShellNow() then
		return true
	end
	return false
end

local function lockAfterShot(weapon, reason)
	if weapon == nil then
		return
	end
	if state.awaitRtRelease then
		print(string.format("[rifle_bolt] ignored lock (%s) — awaiting RT release", tostring(reason)))
		return
	end
	state.needsBolt = true
	state.unlockPlaying = false
	state.awaitRtRelease = false
	resetBoltPull()
	state.stuck = false
	state.wasStuck = false
	-- Must release left grip before the next bolt grab (stops recoil auto-chamber).
	state.awaitLeftGripRelease = state.leftGripDown == true
	state.wasRightThumbrest = isRightThumbrestTouched()
	state.thumbrestUnlockAfter = os.clock() + 0.85
	saveAndStrip(weapon)
	stopPrepareVisuals(weapon)
	setStatus("needs bolt — release grip, then pull bone_lock")
	print(string.format("[rifle_bolt] LOCKED (%s)", tostring(reason or "shot")))
end

local function unlock(reason)
	if not state.needsBolt or state.unlockPlaying then
		return
	end
	if isUnlockSuppressed() then
		print(string.format("[rifle_bolt] unlock suppressed (%s) — shell dock / holding shell", tostring(reason)))
		resetBoltPull()
		state.stuck = false
		state.wasStuck = false
		state.boltGripping = false
		return
	end
	state.needsBolt = false
	state.unlockPlaying = true
	state.awaitRtRelease = true
	resetBoltPull()
	-- Keep stuck while grip held so the hand stays on bone_lock (no detach flicker).
	-- Do NOT set awaitLeftGripRelease here — that would clear stuck and hide the arm.
	local weapon = state.strippedWeapon or getEquippedRifle()

	-- Clear leftovers once, restore soft refs, then help native ready path.
	stopPrepareVisuals(weapon)
	restorePrepare(weapon)
	firePrepareReadyNotify()
	callPrepareEnd()

	setStatus("chambered — release trigger, then fire")
	print(string.format("[rifle_bolt] UNLOCKED (%s) — release RT then fire", tostring(reason or "bolt pull")))
	pcall(function()
		delay(1500, function()
			state.unlockPlaying = false
			if not state.needsBolt and not state.awaitRtRelease then
				setStatus("idle")
			elseif not state.needsBolt then
				setStatus("chambered — release trigger, then fire")
			end
		end)
	end)
end

-- Called by rifle_reload after docking a shell near FX_Chamber (overlaps bone_lock).
function M.suppressUnlock(seconds)
	seconds = tonumber(seconds) or 0.55
	if seconds < 0.1 then
		seconds = 0.1
	end
	state.suppressUnlockUntil = os.clock() + seconds
	resetBoltPull()
	state.stuck = false
	state.wasStuck = false
	state.boltGripping = false
	print(string.format("[rifle_bolt] suppress unlock %.2fs (shell insert)", seconds))
end

-- Empty mag → first shell: force bolt cycle even if player unlocked while empty.
function M.lockForChamber(weapon, reason)
	if not enabled() then
		return
	end
	weapon = weapon or getEquippedRifle()
	if weapon == nil or not isRifleWeapon(weapon) then
		return
	end
	-- Always lock (ignore awaitRtRelease) — chambering after empty load is required.
	state.needsBolt = true
	state.unlockPlaying = false
	state.awaitRtRelease = false
	resetBoltPull()
	state.stuck = false
	state.wasStuck = false
	state.awaitLeftGripRelease = state.leftGripDown == true
	state.wasRightThumbrest = isRightThumbrestTouched()
	state.thumbrestUnlockAfter = os.clock() + 0.85
	saveAndStrip(weapon)
	stopPrepareVisuals(weapon)
	setStatus("needs bolt — chamber first shell")
	print(string.format("[rifle_bolt] LOCKED for chamber (%s)", tostring(reason or "empty→1")))
end

function M.shouldBlockPrepareEnd()
	return enabled() and state.needsBolt and getEquippedRifle() ~= nil
end

function M.shouldForceNotReady(weapon)
	if not enabled() then
		return false
	end
	if weapon ~= nil and not isRifleWeapon(weapon) then
		return false
	end
	if not (state.needsBolt or state.awaitRtRelease) then
		return false
	end
	return weapon ~= nil or getEquippedRifle() ~= nil
end

function M.isBlockingFire()
	return enabled() and (state.needsBolt or state.awaitRtRelease) and getEquippedRifle() ~= nil
end

function M.isHoldingBolt()
	return state.stuck == true or state.boltGripping == true
end

function M.reset(reason)
	local weapon = state.strippedWeapon or getEquippedRifle()
	state.needsBolt = false
	state.awaitRtRelease = false
	state.unlockPlaying = false
	state.stuck = false
	state.wasStuck = false
	state.wasRightThumbrest = false
	state.awaitLeftGripRelease = false
	state.thumbrestUnlockAfter = 0.0
	state.suppressUnlockUntil = 0.0
	resetBoltPull()
	if weapon ~= nil then
		restorePrepare(weapon)
		stopPrepareVisuals(weapon)
	end
	state.strippedWeapon = nil
	state.savedPrepareWeaponAnimBytes = nil
	state.savedPrepareAnimDataBytes = nil
	setStatus(reason or "idle")
	print("[rifle_bolt] reset (" .. tostring(reason or "idle") .. ")")
end

local function firePost(fn, obj, locals, result)
	if not enabled() then
		return
	end
	if not isRifleWeapon(obj) then
		return
	end
	lockAfterShot(obj, "OnFirePerformed")
end

local function shellPost(fn, obj, locals, result)
	if not enabled() then
		return
	end
	if not isRifleWeapon(obj) then
		return
	end
	if state.needsBolt and not state.unlockPlaying then
		stopPrepareVisuals(obj)
	end
end

local function weaponPreparePre(fn, obj, locals, result)
	if not enabled() or not state.needsBolt then
		return true
	end
	if getEquippedRifle() == nil then
		return true
	end
	return false
end

local function ensureHooks()
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
			print("[rifle_bolt] OnFirePerformed hooked (WeaponRifle_BP)")
		else
			print("[rifle_bolt] WARNING: failed to hook OnFirePerformed")
		end
	end

	if not state.shellHooked then
		local ok = hook_function(C.BP_CLASS, "OnShellEjected", false, nil, shellPost, false)
		state.shellHooked = ok == true
		if ok then
			print("[rifle_bolt] OnShellEjected hooked (visual stop only)")
		end
	end

	if not state.prepareAnimHooked then
		local ok = hook_function(
			"BlueprintGeneratedClass /Game/Game/Characters/Humans/JamesSunderland/Animation/AnimationBlueprints/CH_JamesAnimBP.CH_JamesAnimBP_C",
			"UpperBodyAnim_WeaponPrepare",
			false,
			weaponPreparePre,
			nil,
			false
		)
		state.prepareAnimHooked = ok == true
		if ok then
			print("[rifle_bolt] UpperBodyAnim_WeaponPrepare blocked while needsBolt")
		else
			print("[rifle_bolt] WARNING: failed to hook UpperBodyAnim_WeaponPrepare")
		end
	end
end

local function meshHasBoltBone(mesh)
	if mesh == nil then
		return false
	end
	local ok, idx = pcall(function()
		if mesh.GetBoneIndex ~= nil then
			return mesh:GetBoneIndex(C.BOLT_SOCKET)
		end
		return nil
	end)
	if ok and idx ~= nil and idx >= 0 then
		return true
	end
	local ok2, loc = pcall(function()
		return mesh:GetSocketLocation(uevrUtils.fname_from_string(C.BOLT_SOCKET))
	end)
	return ok2 and loc ~= nil and loc.X ~= nil
end

-- Prefer weapon Mesh (MCP: 10-bone sniper with bone_lock). Gripped attachment
-- may be James poseable — do NOT use that for bolt sockets / pull math.
local function getBoltMesh()
	local weapon = getEquippedRifle()
	local wMesh = getWeaponMesh(weapon)
	if meshHasBoltBone(wMesh) then
		return wMesh, weapon
	end
	local attachment = attachments.getCurrentGrippedAttachment(Handed.Right)
	if meshHasBoltBone(attachment) then
		return attachment, weapon
	end
	return wMesh, weapon
end

-- Hand must stick to the VR-visible mesh (usually the gripped sniper attachment).
-- Attaching to the actor Mesh can park the arm on a hidden/world copy → "hand disappears".
local function getBoltHandAttachMesh()
	local attachment = attachments.getCurrentGrippedAttachment(Handed.Right)
	if meshHasBoltBone(attachment) then
		return attachment
	end
	return getBoltMesh()
end

local function isRifleAttachmentID(attachmentID)
	if attachmentID == nil or attachmentID == "" then
		return false
	end
	if attachmentID == C.ATTACHMENT_ID then
		return true
	end
	return string.find(attachmentID, "Sniper_Rifle", 1, true) ~= nil
		or string.find(attachmentID, "WeaponRifle", 1, true) ~= nil
end

local function getBoltHandLocFromUI()
	return {
		tonumber(configui.getValue("rifle_bolt_hand_x")) or 0.0,
		tonumber(configui.getValue("rifle_bolt_hand_y")) or 0.0,
		tonumber(configui.getValue("rifle_bolt_hand_z")) or 0.0,
	}
end

local function getBoltHandRotFromUI()
	return {
		tonumber(configui.getValue("rifle_bolt_hand_pitch")) or 0.0,
		tonumber(configui.getValue("rifle_bolt_hand_yaw")) or 0.0,
		tonumber(configui.getValue("rifle_bolt_hand_roll")) or 90.0,
	}
end

local function applyBoltHandPoseToMarker(marker)
	if marker == nil then
		return
	end
	marker.location = getBoltHandLocFromUI()
	marker.rotation = getBoltHandRotFromUI()
end

local function ensureBoltMarker()
	local list = accessories.getAccessoriesForAttachment(C.ATTACHMENT_ID)
	local acc = list and list[C.ACCESSORY_ID] or nil
	local marker = acc and (accessories.getPrimaryMarkerParams(acc) or (acc.markers and acc.markers[1])) or nil
	if marker == nil then
		print("[rifle_bolt] no bolt accessory marker under " .. C.ATTACHMENT_ID)
		return nil
	end
	if marker.activation_hand ~= 1 then
		marker.activation_hand = 1
		print("[rifle_bolt] forced activation_hand=None")
	end
	local dist = tonumber(marker.activation_distance) or 0
	if dist <= 0 then
		marker.activation_distance = C.DEFAULT_DIST
		print(string.format("[rifle_bolt] set activation_distance=%.1f", C.DEFAULT_DIST))
	end
	if marker.socket_name ~= C.BOLT_SOCKET then
		marker.socket_name = C.BOLT_SOCKET
	end
	applyBoltHandPoseToMarker(marker)
	return marker
end

-- Attach hand once when sticking / on slider change — NOT every frame.
-- Per-frame K2_AttachTo makes the arm flicker/disappear (~1s).
local function refreshBoltHandPose(mesh, marker)
	if not state.stuck then
		return
	end
	mesh = mesh or getBoltHandAttachMesh()
	if mesh == nil or not meshHasBoltBone(mesh) then
		return
	end
	marker = marker or ensureBoltMarker()
	local socket = (marker and marker.socket_name) or C.BOLT_SOCKET
	local attachType = (marker and marker.attach_type) or 0
	local gripAnim = (marker and marker.grip_animation) or "rifle_offhand"
	local loc = getBoltHandLocFromUI()
	local rot = getBoltHandRotFromUI()
	pcall(function()
		uevrUtils.executeUEVRCallbacks(
			"on_accessory_attach",
			Handed.Left,
			mesh,
			socket,
			attachType,
			loc,
			rot
		)
	end)
	pcall(function()
		uevrUtils.executeUEVRCallbacks("on_accessory_animation", Handed.Left, gripAnim)
	end)
end

local function socketWorldLoc(mesh, socketName)
	if mesh == nil or mesh.GetSocketLocation == nil then
		return nil
	end
	local ok, loc = pcall(function()
		return mesh:GetSocketLocation(uevrUtils.fname_from_string(socketName))
	end)
	if ok and loc ~= nil and loc.X ~= nil then
		return loc
	end
	ok, loc = pcall(function()
		return mesh:GetSocketLocation(socketName)
	end)
	if ok and loc ~= nil and loc.X ~= nil then
		return loc
	end
	return nil
end

local function isLeftNearBolt(mesh, marker)
	if mesh == nil then
		return false
	end
	local leftLoc = controllers.getControllerLocation(Handed.Left)
	if leftLoc == nil then
		return false
	end
	local activationDistance = (marker and marker.activation_distance) or C.DEFAULT_DIST
	if activationDistance == nil or activationDistance <= 0 then
		activationDistance = C.DEFAULT_DIST
	end
	local uiDist = configui.getValue("rifle_reload_bolt_dist")
	if uiDist ~= nil and tonumber(uiDist) ~= nil and tonumber(uiDist) > 0 then
		activationDistance = tonumber(uiDist)
	end
	local socketName = (marker and marker.socket_name) or C.BOLT_SOCKET
	local targetLoc = socketWorldLoc(mesh, socketName)
	local targetRot = nil
	pcall(function()
		targetRot = mesh:GetSocketRotation(uevrUtils.fname_from_string(socketName))
	end)
	if targetLoc == nil then
		return false
	end
	if marker ~= nil and marker.location ~= nil and targetRot ~= nil then
		local loc = uevrUtils.vector(marker.location or { 0, 0, 0 })
		targetLoc = targetLoc + uevrUtils.rotateVector(loc, targetRot)
	end
	local d = uevrUtils.distanceBetween(leftLoc, targetLoc)
	return d ~= nil and d <= activationDistance
end

local function captureBoltBasis(mesh)
	local bolt = socketWorldLoc(mesh, C.BOLT_SOCKET)
	if bolt == nil then
		return false
	end
	-- Axis REARWARD along barrel (muzzle → bone_lock). MCP-confirmed.
	local ax, ay, az = -1.0, 0.0, 0.0
	local muzzle = socketWorldLoc(mesh, C.MUZZLE_SOCKET)
	if muzzle ~= nil then
		ax = bolt.X - muzzle.X
		ay = bolt.Y - muzzle.Y
		az = bolt.Z - muzzle.Z
	end
	local len = math.sqrt(ax * ax + ay * ay + az * az)
	if len < 0.001 then
		ax, ay, az = -1.0, 0.0, 0.0
		len = 1.0
	end
	ax, ay, az = ax / len, ay / len, az / len
	state.boltAxisWs = { X = ax, Y = ay, Z = az }
	state.boltOriginWs = { X = bolt.X, Y = bolt.Y, Z = bolt.Z }
	return true
end

local function projectHandOnBolt(leftLoc)
	if leftLoc == nil or state.boltOriginWs == nil or state.boltAxisWs == nil then
		return 0.0
	end
	local o = state.boltOriginWs
	local a = state.boltAxisWs
	return (leftLoc.X - o.X) * a.X + (leftLoc.Y - o.Y) * a.Y + (leftLoc.Z - o.Z) * a.Z
end

local function getBoltTravel()
	local t = configui.getValue("rifle_reload_bolt_travel")
	t = tonumber(t)
	if t == nil or t <= 0 then
		t = C.BOLT_TRAVEL
	end
	return t
end

local function updateBoltPull(mesh)
	if not state.needsBolt or mesh == nil then
		return
	end
	local leftLoc = controllers.getControllerLocation(Handed.Left)
	if leftLoc == nil or not state.leftGripDown then
		state.boltGripping = false
		if state.stuck then
			setStatus("stuck — pull back only")
		end
		return
	end

	if not state.boltGripping then
		if not captureBoltBasis(mesh) then
			return
		end
		state.boltGripping = true
		state.boltGrabAlong = projectHandOnBolt(leftLoc)
		state.boltProgress = 0.0
	end

	local along = projectHandOnBolt(leftLoc)
	local deltaAlong = along - (state.boltGrabAlong or 0.0)
	if deltaAlong < 0.0 then
		deltaAlong = 0.0
	end
	local travel = getBoltTravel()
	local progress = deltaAlong / travel
	if progress > 1.0 then
		progress = 1.0
	end
	state.boltProgress = progress
	setStatus(string.format("pull back %.0f%%", progress * 100.0))
	local now = os.clock()
	if progress >= 0.25 and (now - (state.lastPullLog or 0) > 0.35) then
		state.lastPullLog = now
		print(string.format("[rifle_bolt] pull %.0f%% stuck=%s", progress * 100.0, tostring(state.stuck)))
	end
	if progress >= 0.85 then
		if state.needsBolt and not state.unlockPlaying then
			unlock(string.format("pull back %.0f%%", progress * 100.0))
		end
	end
end

function M.init()
	if state.initialized then
		return
	end
	state.initialized = true
	ensureHooks()
	ensureBoltMarker()
	setStatus("idle")

	uevrUtils.registerUEVRCallback("active_left_accessory", function()
		if not enabled() then
			return
		end
		-- Don't steal left hand while a shell is being carried / just docked.
		if isUnlockSuppressed() then
			state.stuck = false
			state.wasStuck = false
			state.boltGripping = false
			return
		end
		local attachment = attachments.getCurrentGrippedAttachment(Handed.Right)
		local attachmentID = attachment and attachments.getAttachmentIDFromAttachment(attachment) or nil
		if not isRifleAttachmentID(attachmentID) then
			state.stuck = false
			state.wasStuck = false
			state.boltGripping = false
			return
		end
		if not state.leftGripDown then
			state.awaitLeftGripRelease = false
			state.stuck = false
			state.wasStuck = false
			state.boltGripping = false
			return
		end
		-- After a new shot: must release grip before grabbing bolt again (anti auto-chamber).
		-- Only applies when we still need to bolt — not while hand is resting after chamber.
		if state.awaitLeftGripRelease and state.needsBolt then
			state.stuck = false
			state.wasStuck = false
			state.boltGripping = false
			return
		end
		local boltMesh = getBoltMesh()
		local marker = ensureBoltMarker()
		local nowStuck = state.stuck or (state.needsBolt and isLeftNearBolt(boltMesh, marker))
		if nowStuck then
			local justStuck = not state.stuck
			state.stuck = true
			state.wasStuck = true
			if state.needsBolt then
				updateBoltPull(boltMesh)
			end
			-- Attach ONCE when becoming stuck (+ sliders). Per-frame K2_AttachTo hides the arm.
			if justStuck then
				local attachMesh = getBoltHandAttachMesh()
				refreshBoltHandPose(attachMesh, marker)
				-- Re-apply shortly after so we win over accessories' same-frame attach.
				pcall(function()
					delay(33, function()
						if state.stuck then
							refreshBoltHandPose(getBoltHandAttachMesh(), ensureBoltMarker())
						end
					end)
				end)
			end
			return C.ACCESSORY_ID, C.ACCESSORY_PRIORITY
		end
		state.wasStuck = false
		state.boltGripping = false
	end)

	uevrUtils.registerPreEngineTickCallback(function(engine, delta)
		if not enabled() then
			return
		end
		if not state.needsBolt then
			return
		end
		local weapon = getEquippedRifle()
		if weapon == nil then
			M.reset("unequipped")
			return
		end
		stopPrepareVisuals(weapon)
	end)

	uevrUtils.registerLevelChangeCallback(function()
		M.reset("level change")
		ensureBoltMarker()
	end)

	-- prio 12: read left grip + bolt pull (before aim remap).
	uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, stateIn)
		local grip = sh2vrLeftGrip(stateIn)
		state.leftGripDown = grip

		if not enabled() then
			if not grip then
				state.wasStuck = false
				state.boltGripping = false
			end
			return
		end

		local rt = 0
		if stateIn ~= nil and stateIn.Gamepad ~= nil then
			rt = stateIn.Gamepad.bRightTrigger or 0
		end

		if state.needsBolt and not state.awaitRtRelease then
			if not grip then
				state.awaitLeftGripRelease = false
				state.wasStuck = false
				state.boltGripping = false
			end

			-- Shell in hand / just docked near chamber: never stick or unlock.
			if isUnlockSuppressed() then
				state.stuck = false
				state.wasStuck = false
				state.boltGripping = false
				state.wasRightThumbrest = isRightThumbrestTouched()
			else
				-- Test shortcut: rising edge of right thumbrest = full bolt unlock.
				-- Debounced after lock so resting the thumb doesn't auto-chamber.
				local thumbrestDown = isRightThumbrestTouched()
				local thumbrestOk = configui.getValue("rifle_bolt_thumbrest") == true
					and os.clock() >= (state.thumbrestUnlockAfter or 0)
				if thumbrestOk
					and thumbrestDown
					and not state.wasRightThumbrest then
					state.wasRightThumbrest = true
					unlock("right thumbrest")
				else
					state.wasRightThumbrest = thumbrestDown
				end

				local boltMesh = getBoltMesh()
				local marker = ensureBoltMarker()
				local canStick = grip and not state.awaitLeftGripRelease
				-- Only pull if already stuck — do NOT set stuck here (active_left owns stick
				-- edge so justStuck attach fires once).
				if state.needsBolt and boltMesh ~= nil and canStick and state.stuck then
					updateBoltPull(boltMesh)
					if XINPUT_GAMEPAD_LEFT_SHOULDER ~= nil then
						uevrUtils.unpressButton(stateIn, XINPUT_GAMEPAD_LEFT_SHOULDER)
					end
				elseif state.needsBolt and boltMesh ~= nil and canStick and isLeftNearBolt(boltMesh, marker) then
					-- Near bolt: eat LB so sprint remap doesn't fight; stick happens in accessory cb.
					if XINPUT_GAMEPAD_LEFT_SHOULDER ~= nil then
						uevrUtils.unpressButton(stateIn, XINPUT_GAMEPAD_LEFT_SHOULDER)
					end
				elseif not grip then
					state.wasStuck = false
					state.boltGripping = false
				end
			end
		else
			state.wasRightThumbrest = isRightThumbrestTouched()
			if not grip then
				state.awaitLeftGripRelease = false
				state.stuck = false
				state.wasStuck = false
				state.boltGripping = false
			end
		end

		if state.awaitRtRelease and rt < 64 then
			state.awaitRtRelease = false
			state.unlockPlaying = false
			print("[rifle_bolt] RT released — next trigger pull can fire")
			if not state.needsBolt then
				setStatus("chambered")
			end
		end
	end, 12)

	-- AFTER main auto-aim: kill LT+RT while locked / awaiting RT release.
	uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, stateIn)
		if not M.isBlockingFire() then
			return
		end
		if stateIn ~= nil and stateIn.Gamepad ~= nil then
			stateIn.Gamepad.bRightTrigger = 0
			stateIn.Gamepad.bLeftTrigger = 0
		end
	end, -5)

	configui.onCreateOrUpdate("rifle_manual_bolt", function(value)
		if value == false then
			M.reset("disabled")
		end
	end)

	local function onBoltHandSliderChanged()
		if not state.stuck then
			ensureBoltMarker()
			return
		end
		refreshBoltHandPose(getBoltHandAttachMesh(), ensureBoltMarker())
	end
	for _, id in ipairs({
		"rifle_bolt_hand_x",
		"rifle_bolt_hand_y",
		"rifle_bolt_hand_z",
		"rifle_bolt_hand_pitch",
		"rifle_bolt_hand_yaw",
		"rifle_bolt_hand_roll",
	}) do
		configui.onCreateOrUpdate(id, onBoltHandSliderChanged)
	end

	print("[rifle_bolt] step 2 initialized — lock + grip bone_lock + pull rearward")
end

return M
