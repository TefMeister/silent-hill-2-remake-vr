--------------------------------------------------------------------------
-- Rifle manual reload (MCP-confirmed, shotgun pattern):
-- 1) Block native RangedReload when clip empty.
-- 2) Hip + left grip → sniper shell on hand.
-- 3) Release near FX_Chamber / magwell → +1 @0xBB4, spend 1 RifleAmmo.
--
-- Confirmed live:
--   WeaponAmmoItem.RowName = RifleAmmo
--   WeaponClipSize = 4
--   Clip counter @ weapon+0xBB4
--   Shell mesh = SniperRifle_Shell_01_A
--   Insert SFX = Play_Wpn_James_Rifle_Rattle_Hard
--------------------------------------------------------------------------

local uevrUtils = require("libs/uevr_utils")
local controllers = require("libs/controllers")
local configui = require("libs/configui")

local M = {}

local C = {
	CLIP_AMMO_OFF = 0xBB4,
	SHELL_MESH = "StaticMesh /Game/Game/Art/Items/SniperRifle/SniperRifle_Shell_01_A.SniperRifle_Shell_01_A",
	MUZZLE_SOCKET = "FX_muzzle",
	CHAMBER_SOCKET = "FX_Chamber",
	SHELL_SOCKET = "bone_shell",
	ROOT_SOCKET = "bone_root",
	HOLSTER_RIGHT = -20.0,
	HOLSTER_UP = -35.0,
	HOLSTER_FORWARD = 10.0,
	HOLSTER_DIST = 18.0,
	HOLSTER_COOLDOWN = 0.45,
	DOCK_DIST = 18.0,
	DOCK_DOWN = 4.0,
	DOCK_BACK = 2.0,
	SHELL_SCALE = 1.0,
	HAND_X = 4.0,
	HAND_Y = 1.5,
	HAND_Z = -2.0,
	HAND_PITCH = -15.0,
	HAND_YAW = 90.0,
	HAND_ROLL = 0.0,
	DEFAULT_AMMO_ROW = "RifleAmmo",
	DEFAULT_CLIP_SIZE = 4,
}

local AK_STOP_RELOAD =
	"AkAudioEvent /Game/WWiseAudio/Events/Modulators/Stop_AnimNotifies_Reloads.Stop_AnimNotifies_Reloads"
local AK_SHELL_IN =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Movement/Weapons/Play_Wpn_James_Rifle_Rattle_Hard.Play_Wpn_James_Rifle_Rattle_Hard"

local state = {
	initialized = false,
	clipEmpty = false,
	cachedReloadPlayer = nil,
	statusText = "idle",
	lastAbortPrint = 0.0,
	handShell = nil,
	inHolsterZone = false,
	holsterCooldownUntil = 0.0,
	wasLeftGrip = false,
}

local function setStatus(text)
	state.statusText = text or "idle"
	pcall(function()
		configui.setLabel("rifle_reload_status", "Rifle reload: " .. state.statusText)
	end)
end

local function enabled()
	return configui.getValue("rifle_manual_reload") ~= false
end

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
			error("no write")
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

local function getLocalPawn()
	return uevrUtils.get_local_pawn and uevrUtils.get_local_pawn() or uevr.api:get_local_pawn(0)
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

local function getEquippedWeapon()
	return uevrUtils.getValid(getLocalPawn(), {
		"Mesh",
		"AnimScriptInstance",
		"WeaponManageCmbSubcomp",
		"EquippedWeapon",
	})
end

local function getEquippedRifle()
	local w = getEquippedWeapon()
	if isRifleWeapon(w) then
		return w
	end
	return nil
end

local function getWeaponMesh(weapon)
	return uevrUtils.getValid(weapon, { "Mesh" })
end

local function getRifleBolt()
	local ok, mod = pcall(require, "rifle_bolt")
	if ok then
		return mod
	end
	return nil
end

local function getItemsComponent()
	local items = uevrUtils.getValid(getLocalPawn(), { "Items" })
	if items ~= nil then
		return items
	end
	local ok, comps = pcall(function()
		return uevrUtils.find_all_of("Class /Script/SHProto.SHCharacterPlayItemsComponent", false) or {}
	end)
	if not ok then
		return nil
	end
	for _, c in ipairs(comps) do
		local okN, nm = pcall(function()
			return c:get_full_name()
		end)
		if okN and nm ~= nil and string.find(nm, "SHCharacterPlay", 1, true) and not string.find(nm, "Default__", 1, true) then
			return c
		end
	end
	return nil
end

local function getAmmoRowName(weapon)
	local rowStr = C.DEFAULT_AMMO_ROW
	pcall(function()
		local row = weapon.WeaponAmmoItem and weapon.WeaponAmmoItem.RowName
		if row == nil then
			return
		end
		if type(row) == "string" and row ~= "" then
			rowStr = row
			return
		end
		if row.to_string ~= nil then
			local s = row:to_string()
			if s ~= nil and s ~= "" and s ~= "None" then
				rowStr = s
			end
		end
	end)
	return rowStr
end

local function getInventoryAmmoCount(weapon)
	local items = getItemsComponent()
	if items == nil or items.GetItemCount == nil then
		return nil
	end
	local count = nil
	pcall(function()
		count = items:GetItemCount(uevrUtils.fname_from_string(getAmmoRowName(weapon)))
	end)
	if type(count) ~= "number" or count < 0 or count > 100000 then
		return nil
	end
	return math.floor(count)
end

local function removeInventoryAmmo(weapon, amount)
	amount = math.floor(tonumber(amount) or 0)
	if amount <= 0 then
		return 0
	end
	local items = getItemsComponent()
	if items == nil or items.RemoveItem == nil then
		return 0
	end
	local before = getInventoryAmmoCount(weapon)
	local ok = pcall(function()
		items:RemoveItem(uevrUtils.fname_from_string(getAmmoRowName(weapon)), amount, false)
	end)
	if not ok then
		return 0
	end
	local after = getInventoryAmmoCount(weapon)
	if before ~= nil and after ~= nil then
		return math.max(0, before - after)
	end
	return amount
end

local function getClipCount(weapon)
	return readI32(weapon, C.CLIP_AMMO_OFF) or 0
end

local function getClipSize(weapon)
	local size = C.DEFAULT_CLIP_SIZE
	pcall(function()
		if weapon.WeaponClipSize ~= nil then
			size = tonumber(weapon.WeaponClipSize) or size
		end
	end)
	if size < 1 then
		size = C.DEFAULT_CLIP_SIZE
	end
	return size
end

local function magHasRoom(weapon)
	return getClipCount(weapon) < getClipSize(weapon)
end

local function hasReserveAmmo(weapon)
	local inv = getInventoryAmmoCount(weapon)
	if inv ~= nil then
		return inv > 0
	end
	local ok, has = pcall(function()
		return weapon:HasWeaponAmmoInInventory()
	end)
	return ok and has == true
end

local function getRangedReloadPlayer()
	if state.cachedReloadPlayer ~= nil and uevrUtils.getValid(state.cachedReloadPlayer) ~= nil then
		return state.cachedReloadPlayer
	end
	state.cachedReloadPlayer = nil
	local ok, found = pcall(function()
		return uevrUtils.find_all_of("Class /Script/SHProto.SHAnimMontagePlayer", false) or {}
	end)
	if not ok or found == nil then
		return nil
	end
	for _, player in ipairs(found) do
		if player ~= nil then
			local okName, name = pcall(function()
				return player:get_full_name()
			end)
			if okName and name ~= nil and string.find(name, "RangedReload") and string.find(name, "SHCharacterPlay") then
				state.cachedReloadPlayer = player
				return player
			end
		end
	end
	return nil
end

local function abortRangedReloadIfPlaying(weapon)
	local player = getRangedReloadPlayer()
	if player == nil or player.StopRequest == nil then
		return false
	end
	local ok, playing = pcall(function()
		return player.IsPlaying ~= nil and player:IsPlaying()
	end)
	if not (ok and playing) then
		return false
	end
	pcall(function()
		player:StopRequest(0.0)
	end)
	pcall(function()
		local event = uevr.api:find_uobject(AK_STOP_RELOAD)
		if event ~= nil and event.PostOnActor ~= nil and weapon ~= nil then
			event:PostOnActor(weapon, nil, 0, true)
		end
	end)
	local now = os.clock()
	if now - (state.lastAbortPrint or 0) > 1.0 then
		state.lastAbortPrint = now
		print("[rifle_reload] stopped native RangedReload")
	end
	return true
end

local function vecDist(a, b)
	if a == nil or b == nil then
		return 9999.0
	end
	local dx = (a.X or 0) - (b.X or 0)
	local dy = (a.Y or 0) - (b.Y or 0)
	local dz = (a.Z or 0) - (b.Z or 0)
	return math.sqrt(dx * dx + dy * dy + dz * dz)
end

local function getHolsterWorldPos()
	local hmd = controllers.getControllerLocation(2)
	if hmd == nil then
		return nil
	end
	local forward = controllers.getControllerDirection(2)
	local right = controllers.getControllerRightVector(2)
	local up = controllers.getControllerUpVector(2)
	if forward == nil or right == nil or up == nil then
		return nil
	end
	local r = configui.getValue("rifle_reload_hip_right") or C.HOLSTER_RIGHT
	local u = configui.getValue("rifle_reload_hip_up") or C.HOLSTER_UP
	local f = configui.getValue("rifle_reload_hip_forward") or C.HOLSTER_FORWARD
	return {
		X = hmd.X + right.X * r + up.X * u + forward.X * f,
		Y = hmd.Y + right.Y * r + up.Y * u + forward.Y * f,
		Z = hmd.Z + right.Z * r + up.Z * u + forward.Z * f,
	}
end

local function trySocketLoc(mesh, name)
	if mesh == nil or name == nil or mesh.GetSocketLocation == nil then
		return nil
	end
	local ok, loc = pcall(function()
		return mesh:GetSocketLocation(uevrUtils.fname_from_string(name))
	end)
	if ok and loc ~= nil and loc.X ~= nil then
		return loc
	end
	ok, loc = pcall(function()
		return mesh:GetSocketLocation(name)
	end)
	if ok and loc ~= nil and loc.X ~= nil then
		return loc
	end
	return nil
end

-- Magwell / chamber dock: prefer FX_Chamber, then bone_shell, then muzzle+offsets.
local function getLoadingPortWorldLoc(weapon)
	local mesh = getWeaponMesh(weapon)
	if mesh == nil then
		return nil
	end
	local chamber = trySocketLoc(mesh, C.CHAMBER_SOCKET)
	local shellBone = trySocketLoc(mesh, C.SHELL_SOCKET)
	local root = trySocketLoc(mesh, C.ROOT_SOCKET)
	local muzzle = trySocketLoc(mesh, C.MUZZLE_SOCKET)
	local base = chamber or shellBone or root or muzzle
	if base == nil then
		pcall(function()
			base = mesh:K2_GetComponentLocation()
		end)
	end
	if base == nil then
		return nil
	end

	local down = configui.getValue("rifle_reload_dock_down") or C.DOCK_DOWN
	local back = configui.getValue("rifle_reload_dock_back") or C.DOCK_BACK
	local ux, uy, uz = 0.0, 0.0, -1.0
	local bx, by, bz = 0.0, 0.0, 0.0
	if muzzle ~= nil and base ~= nil then
		local fx = base.X - muzzle.X
		local fy = base.Y - muzzle.Y
		local fz = base.Z - muzzle.Z
		local flen = math.sqrt(fx * fx + fy * fy + fz * fz)
		if flen > 0.001 then
			bx, by, bz = fx / flen, fy / flen, fz / flen
		end
	end
	pcall(function()
		local up = mesh:GetUpVector()
		if up ~= nil then
			ux, uy, uz = up.X or 0, up.Y or 0, up.Z or -1
		end
	end)

	return {
		X = base.X - ux * down + bx * back,
		Y = base.Y - uy * down + by * back,
		Z = base.Z - uz * down + bz * back,
	}
end

local function haptic(hand, duration, frequency, amplitude)
	pcall(function()
		uevrUtils.triggerHapticVibration(hand, 0, duration or 0.08, frequency or 1.0, amplitude or 0.45)
	end)
end

local function playShellInsertSound(weapon)
	if weapon == nil then
		return false
	end
	local event = nil
	pcall(function()
		event = uevr.api:find_uobject(AK_SHELL_IN)
	end)
	if event == nil then
		return false
	end
	pcall(function()
		if event.LoadSynchronous ~= nil then
			local loaded = event:LoadSynchronous()
			if loaded ~= nil then
				event = loaded
			end
		end
	end)
	local played = false
	pcall(function()
		if event.PostOnActor ~= nil then
			event:PostOnActor(weapon, nil, 0, true)
			played = true
		end
	end)
	if not played then
		pcall(function()
			local audio = uevrUtils.getValid(weapon, { "WeaponItemAudioComponent" })
				or uevrUtils.getValid(weapon, { "WeaponAudioComponent" })
			if audio ~= nil and audio.SimplePostAkEvent ~= nil then
				audio:SimplePostAkEvent(event)
				played = true
			end
		end)
	end
	return played
end

local function destroyHandShell()
	if state.handShell == nil then
		return
	end
	local comp = state.handShell
	state.handShell = nil
	pcall(function()
		if comp.K2_DestroyComponent ~= nil then
			comp:K2_DestroyComponent(comp)
		end
	end)
end

local function applyHandAttachTransform(comp)
	if comp == nil then
		return
	end
	local x = configui.getValue("rifle_shell_hand_x") or C.HAND_X
	local y = configui.getValue("rifle_shell_hand_y") or C.HAND_Y
	local z = configui.getValue("rifle_shell_hand_z") or C.HAND_Z
	local pitch = configui.getValue("rifle_shell_hand_pitch") or C.HAND_PITCH
	local yaw = configui.getValue("rifle_shell_hand_yaw") or C.HAND_YAW
	local roll = configui.getValue("rifle_shell_hand_roll") or C.HAND_ROLL
	pcall(function()
		if comp.K2_SetRelativeLocation ~= nil then
			comp:K2_SetRelativeLocation(uevrUtils.vector(x, y, z), false, nil, false)
		end
		if comp.K2_SetRelativeRotation ~= nil then
			comp:K2_SetRelativeRotation(uevrUtils.rotator(pitch, yaw, roll), false, nil, false)
		end
	end)
end

local function spawnHandShell()
	destroyHandShell()
	local comp = nil
	pcall(function()
		comp = uevrUtils.createStaticMeshComponent(C.SHELL_MESH, {
			collisionEnabled = false,
			visible = true,
		})
	end)
	if comp == nil then
		print("[rifle_reload] failed to spawn shell mesh")
		return nil
	end
	local scale = configui.getValue("rifle_shell_scale") or C.SHELL_SCALE
	pcall(function()
		if comp.SetRelativeScale3D ~= nil then
			comp:SetRelativeScale3D(uevrUtils.vector(scale, scale, scale))
		end
		controllers.attachComponentToController(0, comp, "", 0, false)
	end)
	applyHandAttachTransform(comp)
	state.handShell = comp
	state.holsterCooldownUntil = os.clock() + C.HOLSTER_COOLDOWN
	haptic(0, 0.08, 1.0, 0.5)
	print("[rifle_reload] shell grabbed from hip")
	setStatus("shell in hand — dock at chamber")
	return comp
end

local function insertShell(weapon)
	if weapon == nil then
		return false
	end
	if not magHasRoom(weapon) then
		setStatus("mag full")
		return false
	end
	if not hasReserveAmmo(weapon) then
		setStatus("no RifleAmmo")
		return false
	end

	local before = getClipCount(weapon)
	if before < 0 then
		before = 0
	end
	local clipSize = getClipSize(weapon)
	local newCount = math.min(before + 1, clipSize)
	local wrote = writeI32(weapon, C.CLIP_AMMO_OFF, newCount)
	local okAmmo, hasAmmo = pcall(function()
		return weapon:HasAnyAmmoInClip()
	end)
	if not (wrote and okAmmo and (hasAmmo == true or newCount > 0)) then
		writeI32(weapon, C.CLIP_AMMO_OFF, before)
		print("[rifle_reload] clip +1 write failed")
		return false
	end

	local removed = removeInventoryAmmo(weapon, 1)
	if removed < 1 then
		writeI32(weapon, C.CLIP_AMMO_OFF, before)
		print("[rifle_reload] RemoveItem failed — rolled back")
		return false
	end

	pcall(function()
		local anim = getWeaponMesh(weapon) and getWeaponMesh(weapon).AnimScriptInstance
		if anim ~= nil and anim.SetOutOfAmmo ~= nil then
			anim:SetOutOfAmmo(false)
		end
	end)

	-- FX_Chamber overlaps bone_lock: block accidental bolt unlock on this dock.
	local bolt = getRifleBolt()
	if bolt ~= nil then
		pcall(function()
			if bolt.suppressUnlock ~= nil then
				bolt.suppressUnlock(0.55)
			end
			-- Empty → first shell: must cycle bolt to chamber (even if unlocked while empty).
			if before <= 0 and bolt.lockForChamber ~= nil then
				bolt.lockForChamber(weapon, "empty→first shell")
			end
		end)
	end

	haptic(0, 0.12, 1.0, 0.7)
	playShellInsertSound(weapon)
	local left = clipSize - newCount
	local inv = getInventoryAmmoCount(weapon)
	print(string.format("[rifle_reload] inserted shell — mag %d/%d (inv %s)", newCount, clipSize, tostring(inv)))
	if left <= 0 then
		setStatus(string.format("mag full %d/%d", newCount, clipSize))
	else
		setStatus(string.format("loaded %d/%d — grab another", newCount, clipSize))
	end
	return true
end

function M.isHoldingShell()
	return state.handShell ~= nil
end

function M.isBusy()
	return enabled() and state.handShell ~= nil
end

function M.isClipEmpty()
	return state.clipEmpty == true
end

function M.shouldBlockNativeReload()
	if not enabled() then
		return false
	end
	local weapon = getEquippedRifle()
	if weapon == nil then
		return false
	end
	if weapon.HasAnyAmmoInClip ~= nil then
		local okAmmo, hasAmmo = pcall(function()
			return weapon:HasAnyAmmoInClip()
		end)
		if okAmmo then
			return hasAmmo == false
		end
	end
	return state.clipEmpty == true
end

function M.reset(reason)
	destroyHandShell()
	state.inHolsterZone = false
	state.holsterCooldownUntil = 0.0
	state.wasLeftGrip = false
	state.clipEmpty = false
	setStatus(reason or "idle")
end

local function handleReload(inputState)
	if not enabled() then
		if state.handShell ~= nil then
			M.reset("disabled")
		end
		state.clipEmpty = false
		return
	end

	local weapon = getEquippedRifle()
	if weapon == nil then
		if state.handShell ~= nil then
			M.reset("unequipped")
		else
			state.clipEmpty = false
		end
		return
	end

	state.clipEmpty = false
	if weapon.HasAnyAmmoInClip ~= nil then
		local okAmmo, hasAmmo = pcall(function()
			return weapon:HasAnyAmmoInClip()
		end)
		state.clipEmpty = okAmmo and hasAmmo == false
	elseif getClipCount(weapon) <= 0 then
		state.clipEmpty = true
	end

	if state.clipEmpty then
		abortRangedReloadIfPlaying(weapon)
		if inputState ~= nil and inputState.Gamepad ~= nil then
			inputState.Gamepad.bLeftTrigger = 0
			inputState.Gamepad.bRightTrigger = 0
		end
	end

	-- Don't fight bolt grip / pull.
	local bolt = getRifleBolt()
	local boltBusy = false
	if bolt ~= nil then
		pcall(function()
			if bolt.isHoldingBolt ~= nil and bolt.isHoldingBolt() then
				boltBusy = true
			elseif bolt.isBlockingFire ~= nil and bolt.isBlockingFire() and bolt.isHoldingBolt == nil then
				-- still allow hip grab while locked but not gripping bolt
			end
		end)
	end

	local leftGrip = uevrUtils.isButtonPressed(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
	local leftLoc = controllers.getControllerLocation(0)
	local hipLoc = getHolsterWorldPos()
	local dockLoc = getLoadingPortWorldLoc(weapon)
	local hipDistLimit = configui.getValue("rifle_reload_hip_dist") or C.HOLSTER_DIST
	local dockDistLimit = configui.getValue("rifle_reload_dock_dist") or C.DOCK_DIST
	local hipDist = vecDist(leftLoc, hipLoc)
	local dockDist = vecDist(leftLoc, dockLoc)
	local now = os.clock()

	if state.handShell ~= nil then
		applyHandAttachTransform(state.handShell)
	end

	if hipLoc ~= nil and leftLoc ~= nil then
		if state.inHolsterZone then
			if hipDist > hipDistLimit + 10.0 then
				state.inHolsterZone = false
			end
		elseif hipDist < hipDistLimit and now >= state.holsterCooldownUntil then
			state.inHolsterZone = true
		end
	else
		state.inHolsterZone = false
	end

	local gripPressed = leftGrip and not state.wasLeftGrip
	local gripReleased = (not leftGrip) and state.wasLeftGrip
	state.wasLeftGrip = leftGrip

	if state.handShell == nil and gripPressed and state.inHolsterZone and not boltBusy then
		if not magHasRoom(weapon) then
			setStatus("mag full")
		elseif not hasReserveAmmo(weapon) then
			setStatus("no RifleAmmo")
		else
			spawnHandShell()
		end
	end

	if state.handShell ~= nil and gripReleased then
		local nearDock = dockLoc ~= nil and dockDist < dockDistLimit
		if nearDock then
			if insertShell(weapon) then
				destroyHandShell()
				state.holsterCooldownUntil = now + C.HOLSTER_COOLDOWN
				state.clipEmpty = false
				if weapon.HasAnyAmmoInClip ~= nil then
					local okAmmo, hasAmmo = pcall(function()
						return weapon:HasAnyAmmoInClip()
					end)
					state.clipEmpty = okAmmo and hasAmmo == false
				end
			else
				destroyHandShell()
				setStatus("insert failed")
			end
		else
			destroyHandShell()
			state.holsterCooldownUntil = now + C.HOLSTER_COOLDOWN
			setStatus("shell dropped")
			print("[rifle_reload] shell dropped (not near chamber)")
		end
	end

	if state.handShell ~= nil then
		if dockLoc ~= nil then
			setStatus(string.format("shell in hand — dock %.1f / %.1f", dockDist, dockDistLimit))
		else
			setStatus("shell in hand — dock at chamber")
		end
	elseif boltBusy then
		-- bolt module owns status while gripping
	elseif state.inHolsterZone and magHasRoom(weapon) and hasReserveAmmo(weapon) then
		setStatus("grip hip for shell")
	elseif state.clipEmpty then
		if hasReserveAmmo(weapon) then
			setStatus("mag empty — hip grab shell")
		else
			setStatus("mag empty — no RifleAmmo")
		end
	elseif state.statusText == "shell dropped" then
		-- keep briefly
	elseif state.statusText == "idle" or string.find(state.statusText, "loaded", 1, true) then
		local clip = getClipCount(weapon)
		local size = getClipSize(weapon)
		if clip < size and hasReserveAmmo(weapon) then
			setStatus(string.format("mag %d/%d — hip grab shell", clip, size))
		end
	end
end

function M.init()
	if state.initialized then
		return
	end
	state.initialized = true
	setStatus("idle")

	uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, stateIn)
		pcall(function()
			handleReload(stateIn)
		end)
	end, 11)

	uevrUtils.registerLevelChangeCallback(function()
		M.reset("level change")
	end)

	configui.onCreateOrUpdate("rifle_manual_reload", function(value)
		if value == false then
			M.reset("disabled")
		end
	end)

	print("[rifle_reload] hip → FX_Chamber +1 @0xBB4 / -1 RifleAmmo")
end

return M
