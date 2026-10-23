--------------------------------------------------------------------------
-- Shotgun manual reload (incremental):
-- Step 1: block native full-tube RangedReload when empty (pump-safe).
-- Step 2: hip + left grip → shell on hand.
-- Step 3: release near loading port → +1 @0xBB4, spend 1 ShotgunAmmo.
--------------------------------------------------------------------------

local uevrUtils = require("libs/uevr_utils")
local controllers = require("libs/controllers")
local configui = require("libs/configui")

local M = {}

local C = {
	CLIP_AMMO_OFF = 0xBB4,
	SHELL_MESH = "StaticMesh /Game/Game/Art/Items/Shotgun/ShotgunShell_01_A.ShotgunShell_01_A",
	MUZZLE_SOCKET = "FX_muzzle",
	FOREARM_SOCKET = "bone_forearm",
	HOLSTER_RIGHT = -20.0,
	HOLSTER_UP = -35.0,
	HOLSTER_FORWARD = 10.0,
	HOLSTER_DIST = 18.0,
	HOLSTER_COOLDOWN = 0.45,
	DOCK_DIST = 16.0,
	DOCK_DOWN = 8.0,
	DOCK_BACK = 4.0,
	SHELL_SCALE = 1.0,
	HAND_X = 4.0,
	HAND_Y = 1.5,
	HAND_Z = -2.0,
	HAND_PITCH = -15.0,
	HAND_YAW = 90.0,
	HAND_ROLL = 0.0,
}

local AK_STOP_RELOAD =
	"AkAudioEvent /Game/WWiseAudio/Events/Modulators/Stop_AnimNotifies_Reloads.Stop_AnimNotifies_Reloads"
local AK_SHELL_IN =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Movement/Weapons/Play_Wpn_James_Shotgun_Rattle_Hard.Play_Wpn_James_Shotgun_Rattle_Hard"

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
		configui.setLabel("shotgun_reload_status", "Shotgun reload: " .. state.statusText)
	end)
end

local function enabled()
	return configui.getValue("shotgun_manual_reload") ~= false
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

local function isShotgunWeapon(weapon)
	if weapon == nil then
		return false
	end
	local ok, name = pcall(function()
		return weapon:get_full_name()
	end)
	return ok and name ~= nil and string.find(name, "WeaponShotgun", 1, true) ~= nil
end

local function getEquippedWeapon()
	return uevrUtils.getValid(getLocalPawn(), {
		"Mesh",
		"AnimScriptInstance",
		"WeaponManageCmbSubcomp",
		"EquippedWeapon",
	})
end

local function getEquippedShotgun()
	local w = getEquippedWeapon()
	if isShotgunWeapon(w) then
		return w
	end
	return nil
end

local function getWeaponMesh(weapon)
	return uevrUtils.getValid(weapon, { "Mesh" })
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
	local rowStr = "ShotgunAmmo"
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
	local size = 6
	pcall(function()
		if weapon.WeaponClipSize ~= nil then
			size = tonumber(weapon.WeaponClipSize) or size
		end
	end)
	if size < 1 then
		size = 6
	end
	return size
end

local function tubeHasRoom(weapon)
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
		print("[shotgun_reload] stopped native RangedReload")
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
	local r = configui.getValue("shotgun_reload_hip_right") or C.HOLSTER_RIGHT
	local u = configui.getValue("shotgun_reload_hip_up") or C.HOLSTER_UP
	local f = configui.getValue("shotgun_reload_hip_forward") or C.HOLSTER_FORWARD
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

local function getLoadingPortWorldLoc(weapon)
	local mesh = getWeaponMesh(weapon)
	if mesh == nil then
		return nil
	end
	local forearm = trySocketLoc(mesh, C.FOREARM_SOCKET)
	local muzzle = trySocketLoc(mesh, C.MUZZLE_SOCKET)
	local base = forearm or muzzle
	if base == nil then
		pcall(function()
			base = mesh:K2_GetComponentLocation()
		end)
	end
	if base == nil then
		return nil
	end

	local down = configui.getValue("shotgun_reload_dock_down") or C.DOCK_DOWN
	local back = configui.getValue("shotgun_reload_dock_back") or C.DOCK_BACK
	local ux, uy, uz = 0.0, 0.0, -1.0
	local bx, by, bz = 0.0, 0.0, 0.0
	if muzzle ~= nil and forearm ~= nil then
		local fx = forearm.X - muzzle.X
		local fy = forearm.Y - muzzle.Y
		local fz = forearm.Z - muzzle.Z
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

-- One short shell click — never Play_Wpn_Shotgun_Reload (full multi-shell bed).
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
	local x = configui.getValue("shotgun_shell_hand_x") or C.HAND_X
	local y = configui.getValue("shotgun_shell_hand_y") or C.HAND_Y
	local z = configui.getValue("shotgun_shell_hand_z") or C.HAND_Z
	local pitch = configui.getValue("shotgun_shell_hand_pitch") or C.HAND_PITCH
	local yaw = configui.getValue("shotgun_shell_hand_yaw") or C.HAND_YAW
	local roll = configui.getValue("shotgun_shell_hand_roll") or C.HAND_ROLL
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
		print("[shotgun_reload] failed to spawn shell mesh")
		return nil
	end
	local scale = configui.getValue("shotgun_shell_scale") or C.SHELL_SCALE
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
	print("[shotgun_reload] shell grabbed from hip")
	setStatus("shell in hand — dock under receiver")
	return comp
end

local function insertShell(weapon)
	if weapon == nil then
		return false
	end
	if not tubeHasRoom(weapon) then
		setStatus("tube full")
		return false
	end
	if not hasReserveAmmo(weapon) then
		setStatus("no ShotgunAmmo")
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
		print("[shotgun_reload] clip +1 write failed")
		return false
	end

	local removed = removeInventoryAmmo(weapon, 1)
	if removed < 1 then
		writeI32(weapon, C.CLIP_AMMO_OFF, before)
		print("[shotgun_reload] RemoveItem failed — rolled back")
		return false
	end

	pcall(function()
		local anim = getWeaponMesh(weapon) and getWeaponMesh(weapon).AnimScriptInstance
		if anim ~= nil and anim.SetOutOfAmmo ~= nil then
			anim:SetOutOfAmmo(false)
		end
	end)

	haptic(0, 0.12, 1.0, 0.7)
	playShellInsertSound(weapon)
	local left = clipSize - newCount
	local inv = getInventoryAmmoCount(weapon)
	print(string.format("[shotgun_reload] inserted shell — tube %d/%d (inv %s)", newCount, clipSize, tostring(inv)))
	if left <= 0 then
		setStatus(string.format("tube full %d/%d", newCount, clipSize))
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
	local weapon = getEquippedShotgun()
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

	local weapon = getEquippedShotgun()
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

	local leftGrip = uevrUtils.isButtonPressed(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
	local leftLoc = controllers.getControllerLocation(0)
	local hipLoc = getHolsterWorldPos()
	local dockLoc = getLoadingPortWorldLoc(weapon)
	local hipDistLimit = configui.getValue("shotgun_reload_hip_dist") or C.HOLSTER_DIST
	local dockDistLimit = configui.getValue("shotgun_reload_dock_dist") or C.DOCK_DIST
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

	if state.handShell == nil and gripPressed and state.inHolsterZone then
		if not tubeHasRoom(weapon) then
			setStatus("tube full")
		elseif not hasReserveAmmo(weapon) then
			setStatus("no ShotgunAmmo")
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
			print("[shotgun_reload] shell dropped (not near loading port)")
		end
	end

	if state.handShell ~= nil then
		if dockLoc ~= nil then
			setStatus(string.format("shell in hand — dock %.1f / %.1f", dockDist, dockDistLimit))
		else
			setStatus("shell in hand — dock under receiver")
		end
	elseif state.inHolsterZone and tubeHasRoom(weapon) and hasReserveAmmo(weapon) then
		setStatus("grip hip for shell")
	elseif state.clipEmpty then
		if hasReserveAmmo(weapon) then
			setStatus("tube empty — hip grab shell")
		else
			setStatus("tube empty — no ShotgunAmmo")
		end
	elseif state.statusText == "shell dropped" then
		-- keep briefly
	elseif state.statusText == "idle" or string.find(state.statusText, "loaded", 1, true) then
		local clip = getClipCount(weapon)
		local size = getClipSize(weapon)
		if clip < size and hasReserveAmmo(weapon) then
			setStatus(string.format("tube %d/%d — hip grab shell", clip, size))
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

	configui.onCreateOrUpdate("shotgun_manual_reload", function(value)
		if value == false then
			M.reset("disabled")
		end
	end)

	print("[shotgun_reload] step 3: hip → loading port +1 @0xBB4 / -1 ShotgunAmmo")
end

return M
