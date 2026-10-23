local uevrUtils = require('libs/uevr_utils')
local controllers = require('libs/controllers')
local configui = require("libs/configui")
local reticule = require("libs/reticule")
local hands = require('libs/hands')
local attachments = require('libs/attachments')
local input = require('libs/input')
local pawnModule = require('libs/pawn')
local montage = require('libs/montage')
local interaction = require('libs/interaction')
local ui = require('libs/ui')
local remap = require('libs/remap')
local gestures = require('libs/gestures')
local gunstock = require('libs/gunstock')
local ik = require('libs/ik')
local accessories = require('libs/accessories')
local mathLib = require('libs/core/math_lib')
local melee = require('melee')
local collision = require('libs/collision')
local shotgunPump = require('shotgun_pump')
local shotgunReload = require('shotgun_reload')
local rifleBolt = require('rifle_bolt')
local rifleReload = require('rifle_reload')

--uevrUtils.setLogLevel(LogLevel.Debug)
--uevrUtils.setDeveloperMode(true)
--hands.enableConfigurationTool()
--uevrUtils.profiler:toggle(true)

ui.init()
--ui.setRequireWidgetOpenState(true)
ui.setRequireWidgetVisibility(true)
montage.init()
interaction.init()
attachments.init()
reticule.init()
reticule.setHiddenWhenScopeActive(true)
pawnModule.init()
remap.init()
input.init()
gunstock.showConfiguration()
ik.init()
collision.init()

attachments.setGunstockOffsetsEnabled(false)
hands.setGunstockOffsetsEnabled(true)
ik.setGunstockOffsetsEnabled(true)

-- hands.setAutoCreateHands(false)
-- ik.setAutoCreateArms(false)

local settings = {}
local status = {}
local desiredManualSaveSlots = 200
local RaytracingMode = {
	Off = 0,
	On = 1,
	ForceOn = 2,
}
local HandsType = {
	None = 1,
	Forearms = 2,
	IKArms = 3,
}

local TraversalType = {
	Undefined = 0,
	Crawl = 1,
	Squeeze = 2,
	Vault = 3,
	Climb = 4,
}


local versionTxt = "v1.0.2"
local title = "Silent Hill 2 First Person Mod " .. versionTxt
local configDefinition = {
	{
		panelLabel = "Silent Hill 2 Config",
		saveFile = "sh2_config",
		layout = spliceableInlineArray
		{
			{ widgetType = "text", id = "title", label = title },
			{ widgetType = "indent", width = 20 }, { widgetType = "text", label = "Control" }, { widgetType = "begin_rect", },
                {
                    widgetType = "combo",
                    id = "movement_type",
                    label = "Forward Movement Direction",
                    selections = {"Game (no roomscale)", "Follows Head"},
                    initialValue = 2,
                },
                {
                    widgetType = "combo",
                    id = "handedness_type",
                    label = "Handedness",
                    selections = {"Left", "Right"},
                    initialValue = 2,
					isHidden = true,
                },
                {
                    widgetType = "combo",
                    id = "hands_type",
                    label = "Hands Type",
                    selections = {"None", "Forearms", "IK Arms"},
                    initialValue = 1,
                },
                {
                    widgetType = "combo",
                    id = "light_type",
                    label = "Light Location",
                    selections = {"Head", "OffHand", "Weapon"},
                    initialValue = 1,
                },
                {
					widgetType = "checkbox",
					id = "use_snap_turn",
					label = "Use Snap Turn",
					initialValue = false
				},
				{ widgetType = "indent", width = 20 }, 
					{
						widgetType = "drag_int",
						id = "snap_turn_angle",
						label = "Angle",
						speed = 1,
						range = {1, 360},
						initialValue = 30,
					},
				{ widgetType = "unindent", width = 20 },
                {
					widgetType = "checkbox",
					id = "hide_head",
					label = "Hide Head",
					initialValue = false
				},
                {
					widgetType = "checkbox",
					id = "offhand_grip_weapon",
					label = "Offhand Can Grip Weapon",
					initialValue = true
				},
                {
					widgetType = "checkbox",
					id = "enable_raytracing",
					label = "Enable Raytracing",
					initialValue = false
				},
				{ widgetType = "indent", width = 20 }, { widgetType = "begin_group", id = "fix_no_raytracing_visual_glitches_group" },
					{
						widgetType = "checkbox",
						id = "fix_no_raytracing_visual_glitches",
						label = "Fix Visual Glitches",
						initialValue = true
					},
					{ widgetType = "same_line" },
					{
						widgetType = "drag_int",
						id = "fix_visual_glitch_seconds",
						label = "Interval (secs)",
						speed = 1,
						range = {1, 100},
						initialValue = 20,
						width = 50
					},
				{ widgetType = "end_group" }, { widgetType = "unindent", width = 20 },
                {
					widgetType = "checkbox",
					id = "fix_orbit_camera_offset",
					label = "Fix Orbit Camera Offset",
					initialValue = false,
					isHidden = true,
				},
                {
                    widgetType = "drag_float",
                    id = "camera_relative_rotation",
                    label = "Camera Relative Rotation",
                    speed = 1,
                    range = {-180, 180},
                    initialValue = 0,
                    isHidden = true,
                },
			{ widgetType = "end_rect", additionalSize = 12, rounding = 5 }, { widgetType = "unindent", width = 20 },
			{ widgetType = "new_line" },
			{ widgetType = "indent", width = 20 }, { widgetType = "text", label = "Interaction" }, { widgetType = "begin_rect", },
				{
					widgetType = "combo",
					id = "interaction_control_mode",
					label = "Interaction Controls",
					selections = {"Vanilla", "Mixed"}, --, "Full Immersion"},
					initialValue = 1,
				},
				{
					widgetType = "combo",
					id = "interaction_sprint_mode",
					label = "Sprint",
					selections = {"Left Thumbstick Press", "Left Thumbstick Press Toggled", "Left Thumbstick Double Tap Forward"},
					initialValue = 2,
				},
				{ widgetType = "begin_group", id = "interaction_desc_advanced_group" },
					{
						widgetType = "text",
						label = "    Turn around - double tap left stick down"
					},
					{
						widgetType = "text",
						label = "    Dodge - right stick down + left stick direction"
					},
					{
						widgetType = "text",
						label = "    Toggle Flashlight - offhand trigger"
					},
					{
						widgetType = "text",
						label = "    Put Flashlight on Head/Retrieve Flashlight - grip head just above HMD using left or right hand"
					},
					{
						widgetType = "text",
						label = "    Shoot - weapon trigger (auto-aims only while firing)"
					},
					{
						widgetType = "checkbox",
						id = "always_aim",
						label = "    Always aim while a gun is out (no grip needed, like Visceral RE2)",
						initialValue = true,
					},
				{ widgetType = "end_group"},
			{ widgetType = "end_rect", additionalSize = 12, rounding = 5 }, { widgetType = "unindent", width = 20 },
			{ widgetType = "new_line" },
			{ widgetType = "indent", width = 20 }, { widgetType = "text", label = "Manual Reload" }, { widgetType = "begin_rect", },
				{
					widgetType = "checkbox",
					id = "pistol_manual_reload",
					label = "Manual pistol reload (mag → slide)",
					initialValue = true,
				},
				{
					widgetType = "text",
					id = "pistol_reload_status",
					label = "Pistol reload: idle",
				},
				{
					widgetType = "checkbox",
					id = "shotgun_manual_pump",
					label = "Manual shotgun pump (lock after shot)",
					initialValue = true,
				},
				{
					widgetType = "text",
					id = "shotgun_pump_status",
					label = "Shotgun pump: idle",
				},
				{
					widgetType = "text",
					label = "    After shot: left grip near pump + pull back to chamber",
				},
				{
					widgetType = "drag_float",
					id = "shotgun_pump_grab_dist",
					label = "Shotgun pump grab distance",
					speed = 0.5,
					range = { 4.0, 30.0 },
					initialValue = 10.0,
				},
				{
					widgetType = "checkbox",
					id = "shotgun_manual_reload",
					label = "Manual shotgun reload (hip shell → loading port)",
					initialValue = true,
				},
				{
					widgetType = "text",
					id = "shotgun_reload_status",
					label = "Shotgun reload: idle",
				},
				{
					widgetType = "text",
					label = "    Hip grab shell → dock under receiver → +1 tube",
				},
				{
					widgetType = "drag_float",
					id = "shotgun_reload_hip_dist",
					label = "Shotgun hip grab distance",
					speed = 0.5,
					range = { 6.0, 50.0 },
					initialValue = 18.0,
				},
				{
					widgetType = "drag_float",
					id = "shotgun_reload_dock_dist",
					label = "Shotgun loading-port dock distance",
					speed = 0.5,
					range = { 5.0, 40.0 },
					initialValue = 16.0,
				},
				{
					widgetType = "checkbox",
					id = "rifle_manual_bolt",
					label = "Manual rifle bolt (lock auto prepare after shot)",
					initialValue = true,
				},
				{
					widgetType = "text",
					id = "rifle_bolt_status",
					label = "Rifle bolt: idle",
				},
				{
					widgetType = "text",
					label = "    After shot: left grip near bone_lock + pull back to chamber",
				},
				{
					widgetType = "checkbox",
					id = "rifle_bolt_thumbrest",
					label = "Right thumbrest = open bolt (test)",
					initialValue = false,
				},
				{
					widgetType = "drag_float",
					id = "rifle_reload_bolt_dist",
					label = "Rifle bolt grab distance",
					speed = 0.5,
					range = { 5.0, 50.0 },
					initialValue = 16.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_reload_bolt_travel",
					label = "Rifle bolt pull-back travel",
					speed = 0.5,
					range = { 4.0, 40.0 },
					initialValue = 10.0,
				},
				{
					widgetType = "text",
					label = "    Hand on bolt (while gripping) — tune until palm sits on bone_lock",
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_x",
					label = "Bolt hand X",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_y",
					label = "Bolt hand Y",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_z",
					label = "Bolt hand Z",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_pitch",
					label = "Bolt hand Pitch",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_yaw",
					label = "Bolt hand Yaw",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_bolt_hand_roll",
					label = "Bolt hand Roll",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 90.0,
				},
				{
					widgetType = "checkbox",
					id = "rifle_manual_reload",
					label = "Manual rifle reload (hip shell → chamber)",
					initialValue = true,
				},
				{
					widgetType = "text",
					id = "rifle_reload_status",
					label = "Rifle reload: idle",
				},
				{
					widgetType = "text",
					label = "    Hip grab shell → dock at FX_Chamber → +1 mag",
				},
				{
					widgetType = "drag_float",
					id = "rifle_reload_hip_dist",
					label = "Rifle hip grab distance",
					speed = 0.5,
					range = { 6.0, 50.0 },
					initialValue = 18.0,
				},
				{
					widgetType = "drag_float",
					id = "rifle_reload_dock_dist",
					label = "Rifle chamber dock distance",
					speed = 0.5,
					range = { 5.0, 40.0 },
					initialValue = 18.0,
				},
				{
					widgetType = "text",
					label = "    X = eject mag  |  Hip grab  |  Dock  | Grip slide (hand sticks) + pull",
				},
				{
					widgetType = "text",
					label = "    (or right thumbrest once to finish rack after mag seated)",
				},
				{
					widgetType = "checkbox",
					id = "pistol_rack_thumbrest",
					label = "Right thumbrest = finish rack",
					initialValue = true,
				},
				{
					widgetType = "drag_float",
					id = "pistol_reload_mag_dist",
					label = "Mag grab/return distance",
					speed = 0.5,
					range = { 5.0, 40.0 },
					initialValue = 18.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_reload_hip_dist",
					label = "Hip grab distance",
					speed = 0.5,
					range = { 6.0, 50.0 },
					initialValue = 18.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_reload_slide_dist",
					label = "Slide grab distance",
					speed = 0.5,
					range = { 5.0, 50.0 },
					initialValue = 22.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_reload_slide_travel",
					label = "Slide pull-back travel",
					speed = 0.5,
					range = { 4.0, 40.0 },
					initialValue = 12.0,
				},
				{
					widgetType = "text",
					label = "    Hand mag pose (while held)",
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_x",
					label = "Hand mag X",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 4.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_y",
					label = "Hand mag Y",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 1.5,
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_z",
					label = "Hand mag Z",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = -2.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_pitch",
					label = "Hand mag Pitch",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = -15.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_yaw",
					label = "Hand mag Yaw",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 90.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_hand_mag_roll",
					label = "Hand mag Roll",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "text",
					label = "    Hand on slide (while racking) — rotate until palm on top",
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_x",
					label = "Slide hand X",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_y",
					label = "Slide hand Y",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_z",
					label = "Slide hand Z",
					speed = 0.1,
					range = { -20.0, 20.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_pitch",
					label = "Slide hand Pitch",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_yaw",
					label = "Slide hand Yaw",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 0.0,
				},
				{
					widgetType = "drag_float",
					id = "pistol_slide_hand_roll",
					label = "Slide hand Roll",
					speed = 1.0,
					range = { -180.0, 180.0 },
					initialValue = 90.0,
				},
			{ widgetType = "end_rect", additionalSize = 12, rounding = 5 }, { widgetType = "unindent", width = 20 },
			{ widgetType = "new_line" },
			{ widgetType = "indent", width = 20 }, { widgetType = "text", label = "UI" }, { widgetType = "begin_rect", },
				expandArray(ui.getConfigurationWidgets),
			{ widgetType = "end_rect", additionalSize = 12, rounding = 5 }, { widgetType = "unindent", width = 20 },
			{ widgetType = "new_line" },
			{ widgetType = "indent", width = 20 }, { widgetType = "text", label = "Reticule" }, { widgetType = "begin_rect", },
				expandArray(reticule.getConfigurationWidgets,{{id="uevr_reticule_update_distance", initialValue=200}, {id="uevr_reticule_eye_dominance_offset", initialValue=63.0, isHidden=false}}),
			{ widgetType = "end_rect", additionalSize = 12, rounding = 5 }, { widgetType = "unindent", width = 20 },
			{ widgetType = "new_line" },
		}
	}
}

--increase the number of save game slots
local function applySaveSlotLimit()
	local saveGameSettings = uevrUtils.find_default_instance("Class /Script/GameBase.SaveGameSettings")
	if saveGameSettings == nil then
		return
	end

	print("Current max save slots", saveGameSettings.MaxSaveSlotsNumber)

	local autoSaveSlots = saveGameSettings.AutoSaveSlotsNumber or 0
	local targetMaxSlots = desiredManualSaveSlots + autoSaveSlots
	if saveGameSettings.MaxSaveSlotsNumber ~= targetMaxSlots then
		saveGameSettings.MaxSaveSlotsNumber = targetMaxSlots
		print("Updated manual save slots to", desiredManualSaveSlots, "total slots", targetMaxSlots)
	end
end

applySaveSlotLimit()

local function getGameUserSettings()
	if status.gameUserSettings ~= nil and uevrUtils.getValid(status.gameUserSettings) ~= nil then
		return status.gameUserSettings
	end

	local gameUserSettingsClass = uevrUtils.find_default_instance("Class /Script/Engine.GameUserSettings")
	if gameUserSettingsClass ~= nil and gameUserSettingsClass.GetGameUserSettings ~= nil then
		status.gameUserSettings = gameUserSettingsClass:GetGameUserSettings()
	end

	if status.gameUserSettings == nil then
		status.gameUserSettings = (uevrUtils.find_all_of("Class /Script/GameBase.BGameUserSettings", false) or {})[1]
	end

	return uevrUtils.getValid(status.gameUserSettings)
end

local function getPlayerProfile()
	if status.playerProfile ~= nil and uevrUtils.getValid(status.playerProfile) ~= nil then
		return status.playerProfile
	end

	local world = uevrUtils.getWorld()
	if world == nil then
		return nil
	end

	status.gameBaseStatics = status.gameBaseStatics or uevrUtils.find_default_instance("Class /Script/GameBase.GameBaseStatics")
	if status.gameBaseStatics ~= nil and status.gameBaseStatics.GetPlayerProfile ~= nil then
		status.playerProfile = status.gameBaseStatics:GetPlayerProfile(world)
	end

	return uevrUtils.getValid(status.playerProfile)
end

local function savePlayerProfile()
	local world = uevrUtils.getWorld()
	if world == nil then
		return false
	end

	status.gameBaseStatics = status.gameBaseStatics or uevrUtils.find_default_instance("Class /Script/GameBase.GameBaseStatics")
	if status.gameBaseStatics == nil or status.gameBaseStatics.SaveUserProfile == nil then
		return false
	end

	status.gameBaseStatics:SaveUserProfile(world)
	return true
end

local function setSprintToggleEnabled(enabled, saveProfile)
	local playerProfile = getPlayerProfile()
	if playerProfile == nil then
		return false
	end

	playerProfile.SprintToggleable = enabled == true

	if saveProfile ~= false then
		savePlayerProfile()
	end

	return true
end

local function applyRaytracingMode(mode, updateSettings)
	local raytracingMode = mode
	if raytracingMode == nil then raytracingMode = RaytracingMode.Off end

	local enabled = raytracingMode ~= RaytracingMode.Off

	if updateSettings == true then
		local playerProfile = getPlayerProfile()
		if playerProfile ~= nil and playerProfile.GFXSettings ~= nil then
			local profileSettings = playerProfile.GFXSettings
			profileSettings.Raytracing = raytracingMode
			playerProfile.GFXSettings = profileSettings
		end

		local gameUserSettings = getGameUserSettings()
		if gameUserSettings ~= nil and gameUserSettings.GFXSettings ~= nil then
			local gfxSettings = gameUserSettings.GFXSettings
			gfxSettings.Raytracing = raytracingMode
			gameUserSettings.GFXSettings = gfxSettings
			if gameUserSettings.ApplyNonResolutionSettings ~= nil then
				gameUserSettings:ApplyNonResolutionSettings()
			end
			if gameUserSettings.SaveSettings ~= nil then
				gameUserSettings:SaveSettings()
			end
		end

		status.rendererSettings = status.rendererSettings or uevrUtils.find_default_instance("Class /Script/Engine.RendererSettings")
		if status.rendererSettings ~= nil then
			status.rendererSettings.bEnableRayTracing = enabled
			status.rendererSettings.bUseHardwareRayTracingForLumen = enabled
		end
	end

	uevrUtils.set_cvar_int("r.RayTracing", enabled and 1 or 0)
	uevrUtils.set_cvar_int("r.Lumen.HardwareRayTracing", enabled and 1 or 0)
	uevrUtils.set_cvar_int("r.Lumen.Reflections.HardwareRayTracing", enabled and 1 or 0)
	uevrUtils.set_cvar_int("r.LumenScene.Radiosity.HardwareRayTracing", enabled and 1 or 0)
	uevrUtils.set_cvar_int("r.Lumen.RadianceCache.HardwareRayTracing", enabled and 1 or 0)

	return raytracingMode
end

local function cleanup()
    status = {}
end

local function regenerateHands(value)
    hands.setAutoCreateHands(value == HandsType.Forearms)
    ik.setAutoCreateArms(value == HandsType.IKArms)

    hands.destroyHands()
    ik.destroyAll()
end

ik.registerOnMeshCreatedCallback(function(meshComponentList, ikInstance)
	local collisionParent = uevrUtils.getValid(pawn)
	for i, meshComponent in ipairs(meshComponentList or {}) do
		--These hide the arms but not the shadows and flashlight is still blocked even with arms hidden
		-- meshComponent:SetRenderInMainPass(false)
		-- meshComponent:SetRenderInDepthPass(false)
		-- meshComponent:SetVisibility(false)

		--uncomment this to have the arms not cast shadows (and not block the flashlight)
		--meshComponent.bCastDynamicShadow = false
		if collisionParent == nil and meshComponent.GetOwner ~= nil then
			collisionParent = uevrUtils.getValid(meshComponent:GetOwner())
		end
	end

	-- local rightCollider = collision.create("right_hand", collisionParent, Handed.Right)
	-- local leftCollider = collision.create("left_hand", collisionParent, Handed.Left)

	-- --Use the colliders as accessories so the ik hands will follow the colliders rather than the vr controller
	-- --This, for example, makes hands not pass through door when opening doors with hands
	-- if rightCollider ~= nil then uevrUtils.executeUEVRCallbacks("on_accessory_attach", Handed.Right, rightCollider, nil, 0, nil, nil) end
	-- if leftCollider ~= nil then uevrUtils.executeUEVRCallbacks("on_accessory_attach", Handed.Left, leftCollider, nil, 0, nil, nil) end
end)

ik.registerOnDestroyCallback(function(ikInstance)
	uevrUtils.executeUEVRCallbacks("on_accessory_detach", Handed.Right)
	uevrUtils.executeUEVRCallbacks("on_accessory_detach", Handed.Left)

	-- collision.destroy("right_hand")
	-- collision.destroy("left_hand")
	--detach attachments first so they dont get "lost" when hands are destroyed
	attachments.detachGripAttachments(Handed.Right)
	attachments.detachGripAttachments(Handed.Left)
end)

hands.registerOnDestroyCallback(function()
	uevrUtils.executeUEVRCallbacks("on_accessory_detach", Handed.Right)
	uevrUtils.executeUEVRCallbacks("on_accessory_detach", Handed.Left)

	-- collision.destroy("right_hand")
	-- collision.destroy("left_hand")
	--detach attachments first so they dont get "lost" when hands are destroyed
	attachments.detachGripAttachments(Handed.Right)
	attachments.detachGripAttachments(Handed.Left)
end)

local function hideHead(val)
	if uevrUtils.getValid(pawn,{"Mesh"}) ~= nil then
		if val and configui.getValue("hide_head") == true then
			pawn.Mesh:HideBoneByName(uevrUtils.fname_from_string("neck_02_bn"), 0)
		else
			pawn.Mesh:UnHideBoneByName(uevrUtils.fname_from_string("neck_02_bn"))
		end
	else
		delay(2000, function() hideHead(val) end)
	end
end
configui.onCreateOrUpdate("hide_head", function(value)
    hideHead(value)
end)

configui.onCreateOrUpdate("hands_type", function(value)
    regenerateHands(value)
end)

function on_cutscene_change(isInCutscene)
    pawnModule.hideArmsBones(not isInCutscene)
    hideHead(not isInCutscene)
	melee.disable(isInCutscene)
end

local function getHandedness()
	local handednessType = configui.getValue("handedness_type")
	if handednessType == 1 then
		return Handed.Left
	elseif handednessType == 2 then
		return Handed.Right
	end
	return Handed.Right
end

local function resetAttachment()
	local attachmentData = attachments.getCurrentGrippedAttachmentData(getHandedness())
	if attachmentData ~= nil and attachmentData.attachment ~= nil and attachmentData.attachment.RelativeLocation ~= nil and attachmentData.attachment.RelativeLocation.X == 0 and attachmentData.attachment.RelativeLocation.Y == 0 and attachmentData.attachment.RelativeLocation.Z == 0 then
		local loc, rot, scale = attachments.getAttachmentOffset(attachmentData.attachment)
		local attachmentID = attachments.getAttachmentIDFromAttachment(attachmentData.attachment)
		attachments.updateAttachmentTransform(loc, rot, scale, attachmentID)
	end
end

local function attachLightToController(light)
	if uevrUtils.getValid(light) ~= nil then
		if configui.getValue("light_type") == 1 then
			local lightState = UEVR_UObjectHook.get_or_add_motion_controller_state(light)
			lightState:set_hand(2) -- head
			lightState:set_permanent(false)
			lightState:set_rotation_offset(temp_vec3f:set(0.0, 0.0, 0.0))
			lightState:set_location_offset(temp_vec3f:set(0.0, 0.0, 0.0))
		elseif configui.getValue("light_type") == 2 then
			UEVR_UObjectHook.remove_motion_controller_state(light)
			local lightState = UEVR_UObjectHook.get_or_add_motion_controller_state(light)
			lightState:set_hand(1-getHandedness()) -- off hand
			lightState:set_permanent(false)
			lightState:set_rotation_offset(temp_vec3f:set(0.38, -0.59, 0.0))
			lightState:set_location_offset(temp_vec3f:set(-2.4, -6.8, -0.8))
			status.lightState = lightState
		elseif configui.getValue("light_type") == 3 then
			UEVR_UObjectHook.remove_motion_controller_state(light)
			local lightState = UEVR_UObjectHook.get_or_add_motion_controller_state(light)
			lightState:set_hand(getHandedness()) -- off hand
			lightState:set_permanent(false)
			lightState:set_rotation_offset(temp_vec3f:set(0.13, 0.265, 0.0))
			lightState:set_location_offset(temp_vec3f:set(-2.4, -8.9, -0.8))
			status.lightState = lightState
		end
	end
end

local function getFlashlightMesh()
    if pawn == nil then return nil end

	if status.flashlight ~= nil and status.flashlight.flashlightMesh ~= nil and uevrUtils.getValid(status.flashlight.flashlightMesh) ~= nil then
		return status.flashlight.flashlightMesh
	end

	local equipmentActors = uevrUtils.getValid(pawn, {"Items", "EquipmentActors"})
	if equipmentActors ~= nil then
		for _, actor in ipairs(equipmentActors) do
			if actor:is_a(uevrUtils.get_class("Class /Script/SHProto.SHFlashlight")) then
				status.flashlight = {}
				status.flashlight.flashlightMesh = actor.Mesh

				local light = actor.LightMain
				if light ~= nil then
					attachLightToController(light)
				end
				status.flashlight.lightMesh = light

				local lightShaft = actor.Lightshaft
				if lightShaft ~= nil then
					lightShaft:SetRenderInMainPass(false)
					-- lightShaft:K2_AttachTo(status.flashlight.flashlightMesh, uevrUtils.fname_from_string(""), 0, false)
					-- uevrUtils.set_component_relative_transform(lightShaft, uevrUtils.vector(configui.getValue("light_shaft_location")), uevrUtils.rotator(configui.getValue("light_shaft_rotation")))
				end
				status.flashlight.lightShaftMesh = lightShaft

				return status.flashlight.flashlightMesh
			end
		end
	end
    return nil
end

local function getWeaponMesh()
    local equippedWeapon = uevrUtils.getValid(pawn, {"Mesh", "AnimScriptInstance", "WeaponManageCmbSubcomp", "EquippedWeapon"})
	if equippedWeapon ~= nil then
		if equippedWeapon.AutoAimMaxRange ~= nil then
			equippedWeapon.AutoAimMaxRange = 1.0
		end
		local weaponName = equippedWeapon:get_full_name()
		if uevrUtils.startsWith(weaponName, "WeaponPistol") then
			status.currentReticuleOffset = 3
		elseif uevrUtils.startsWith(weaponName, "WeaponShotgun") then
			status.currentReticuleOffset = 5
		elseif uevrUtils.startsWith(weaponName, "WeaponRifle") then
			status.currentReticuleOffset = 6
		end
		--print("Equipped weapon", equippedWeapon:get_full_name())
	end

	local weaponMesh = uevrUtils.getValid(equippedWeapon, {"Mesh"})

	status.currentEquippedWeapon = equippedWeapon


    if weaponMesh == nil then
        --try to get investigation items
        weaponMesh = uevrUtils.getValid(pawn, {"Items", "ItemExecutive", "ItemContext", "Mesh"})
		if weaponMesh ~= status.currentWeaponMesh then
			-- if weaponMesh ~= nil then
			-- 	print("New Weapon mesh found", weaponMesh:get_full_name(), weaponMesh.StaticMesh and weaponMesh.StaticMesh:get_full_name() or "no static mesh", weaponMesh.SkeletalMesh and weaponMesh.SkeletalMesh:get_full_name() or "no skeletal mesh")
			-- end
			status.currentWeaponMesh = weaponMesh
			status.updateAttachmentTransform = true
			delay(1000, function()
				status.updateAttachmentTransform = false
			end)
		end
    end
    return weaponMesh
end

local function getEquippedWeapon(currentPawn)
	return uevrUtils.getValid(currentPawn, {"Mesh", "AnimScriptInstance", "WeaponManageCmbSubcomp", "EquippedWeapon"})
end

local defaultAttachOptions = {
	detachFromOriginOnGrip = true,
	maintainWorldPositionOnDetachFromOrigin = false,
	detachFromParentOnRelease = true,
	maintainWorldPositionOnDetachFromParent = false,
	reattachToOriginOnRelease = true,
	restoreTransformToOriginOnReattach = true,
	useZeroTransformOnReattach = false,
	allowChildVisibilityHandling = false,
	allowChildHiddenInGameHandling = false,
	allowRenderInMainPassHandling = false,
}

attachments.registerOnGripUpdateCallback(function()
	if uevrUtils.isInCutscene() then return nil end

    local weaponMesh = getWeaponMesh()
	local flashlightMesh = getFlashlightMesh()
	if configui.getValue("light_type") == 1 or configui.getValue("light_type") == 3 then
		flashlightMesh = nil
	end
	accessories.setDisabled(not configui.getValue("offhand_grip_weapon") or flashlightMesh ~= nil)

    local rightHandComponent = nil
   	local leftHandComponent = nil
    if configui.getValue("hands_type") == HandsType.None then
        rightHandComponent = controllers.getController(Handed.Right)
        leftHandComponent = controllers.getController(Handed.Left)
    elseif configui.getValue("hands_type") == HandsType.Forearms then
        rightHandComponent = hands.getHandComponent(Handed.Right)
        leftHandComponent = hands.getHandComponent(Handed.Left)
    elseif configui.getValue("hands_type") == HandsType.IKArms then
        rightHandComponent = ik.getCurrentMesh()
		leftHandComponent = ik.getCurrentMesh()
    end

    local weaponAttachSocket = "hand_r_bn" 
	local flashlightAttachSocket = "hand_l_bn"
    --print( weaponMesh, rightHandComponent, weaponAttachSocket, flashlightMesh, leftHandComponent, flashlightAttachSocket)
    return rightHandComponent and weaponMesh, rightHandComponent, weaponAttachSocket, leftHandComponent and flashlightMesh, flashlightMesh and leftHandComponent, flashlightAttachSocket, defaultAttachOptions --, controllers.getController(Handed.Right)
end)

local snapTurnState = {
	rxState = 0,
	deadZone = 20000,
}

local smoothTurnState = {
	deadZone = 4000,
	degreesPerSecond = 180,
}

local leftStickDoubleTapState = {
	forward = {
		isPressed = false,
		lastTapTime = nil,
		pressThreshold = 24000,
		releaseThreshold = 18000,
	},
	backward = {
		isPressed = false,
		lastTapTime = nil,
		pressThreshold = -20000,
		releaseThreshold = -12000,
	},
	doubleTapWindow = 0.35,
}

local fixedOrbitCameraPitch = -20.0

local function getLocalPawn()
	local currentPawn = uevrUtils.get_local_pawn()
	if uevrUtils.getValid(currentPawn) == nil then
		return nil
	end
	return currentPawn
end

local function isPawnTraversing(currentPawn)
	if currentPawn == nil then
		return false
	end

	local activeTraversal = uevrUtils.getValid(currentPawn, {"Traversal", "CurrentlyPlayingTraversal"})
	if activeTraversal == nil then
		return false
	end
	print("Active traversal type", activeTraversal.Type)
	return activeTraversal.Type == TraversalType.Vault or activeTraversal.Type == TraversalType.Climb
end

local function applyFaceAwayFromCamera(currentPawn)
	if currentPawn == nil or currentPawn.View == nil or currentPawn.Movement == nil then
		return
	end

	local viewRotation = currentPawn.View:GetViewRotation()
	if viewRotation == nil then
		return
	end

	local yawOffset = configui.getValue("camera_relative_rotation") or 0
	local targetRotation = uevrUtils.rotator(0, viewRotation.Yaw + yawOffset, 0)
	currentPawn.Movement:RotationSnapAbsoluteStatic(targetRotation, 0.0, currentPawn, nil)
end

local function handleSnapTurnInput(state)
	if configui.getValue("use_snap_turn") ~= true then
		snapTurnState.rxState = 0
		return
	end

	if state == nil or state.Gamepad == nil then
		snapTurnState.rxState = 0
		return
	end

	local currentPawn = getLocalPawn()
	if currentPawn == nil or currentPawn.View == nil then
		snapTurnState.rxState = 0
		return
	end

	local thumbRX = state.Gamepad.sThumbRX or 0
	local turnStep = nil
	if thumbRX >= snapTurnState.deadZone then
		state.Gamepad.sThumbRX = 0
		if snapTurnState.rxState == 0 then
			turnStep = configui.getValue("snap_turn_angle") or 30
			snapTurnState.rxState = 1
		end
	elseif thumbRX <= -snapTurnState.deadZone then
		state.Gamepad.sThumbRX = 0
		if snapTurnState.rxState == 0 then
			turnStep = -(configui.getValue("snap_turn_angle") or 30)
			snapTurnState.rxState = -1
		end
	else
		snapTurnState.rxState = 0
	end

	if turnStep == nil or currentPawn.View.GetViewRotation == nil or currentPawn.View.OverrideControlRotation == nil then
		return
	end

	local viewRotation = currentPawn.View:GetViewRotation()
	if viewRotation == nil then
		return
	end

	if status.turnStep == nil then status.turnStep = 0 end
	status.turnStep = status.turnStep + turnStep
end

local function handleSmoothTurnInput(state)
	if configui.getValue("use_snap_turn") == true then
		status.turnAxisX = 0
		return
	end

	if state == nil or state.Gamepad == nil then
		status.turnAxisX = 0
		return
	end

	local thumbRX = state.Gamepad.sThumbRX or 0
	if math.abs(thumbRX) < smoothTurnState.deadZone then
		status.turnAxisX = 0
		return
	end

	status.turnAxisX = thumbRX / 32767.0
	state.Gamepad.sThumbRX = 0
end

local function getLeftStickDoubleTapDirection(state)
	if state == nil or state.Gamepad == nil then
		leftStickDoubleTapState.forward.isPressed = false
		leftStickDoubleTapState.backward.isPressed = false
		return false, false
	end

	local thumbLY = state.Gamepad.sThumbLY or 0
	local currentTime = os.clock()
	local wasTappedForward = false
	local wasTappedBackward = false

	local isPressedForward = thumbLY >= leftStickDoubleTapState.forward.pressThreshold
	local isReleasedForward = thumbLY <= leftStickDoubleTapState.forward.releaseThreshold
	if isPressedForward and not leftStickDoubleTapState.forward.isPressed then
		leftStickDoubleTapState.forward.isPressed = true
		if leftStickDoubleTapState.forward.lastTapTime ~= nil and currentTime - leftStickDoubleTapState.forward.lastTapTime <= leftStickDoubleTapState.doubleTapWindow then
			leftStickDoubleTapState.forward.lastTapTime = nil
			wasTappedForward = true
		else
			leftStickDoubleTapState.forward.lastTapTime = currentTime
		end
	elseif isReleasedForward then
		leftStickDoubleTapState.forward.isPressed = false
	end

	local isPressedBackward = thumbLY <= leftStickDoubleTapState.backward.pressThreshold
	local isReleasedBackward = thumbLY >= leftStickDoubleTapState.backward.releaseThreshold
	if isPressedBackward and not leftStickDoubleTapState.backward.isPressed then
		leftStickDoubleTapState.backward.isPressed = true
		if leftStickDoubleTapState.backward.lastTapTime ~= nil and currentTime - leftStickDoubleTapState.backward.lastTapTime <= leftStickDoubleTapState.doubleTapWindow then
			leftStickDoubleTapState.backward.lastTapTime = nil
			wasTappedBackward = true
		else
			leftStickDoubleTapState.backward.lastTapTime = currentTime
		end
	elseif isReleasedBackward then
		leftStickDoubleTapState.backward.isPressed = false
	end

	return wasTappedForward, wasTappedBackward
end

--wont callback unless an uevrUtils.updateDeferral("is_gripping_light") hasnt been called in the last 300ms
uevrUtils.createDeferral("is_gripping_light", 300, function()
    status["isGrippingLight"] = false
end)


-- =============================================================================
-- Pistol manual reload (inlined). MCP-confirmed on WeaponPistol_BP_C:
-- bones magazine_joint / magazine_socket_joint / slide_joint
-- refill via AnimNotify_ReloadFillWeaponClip (pulse X); empty via SetOutOfAmmo
-- =============================================================================
local PISTOL_RELOAD_MAG_DIST = 18.0
local PISTOL_RELOAD_SLIDE_DIST = 22.0
local PISTOL_RELOAD_SLIDE_TRAVEL = 12.0
local PISTOL_RELOAD_TIMEOUT = 10.0
-- Hip holster (from reload.lua): HMD-relative pocket where dropped mag parks / is grabbed
local PISTOL_HOLSTER_RIGHT = -20.0
local PISTOL_HOLSTER_UP = -35.0
local PISTOL_HOLSTER_FORWARD = 10.0
local PISTOL_HOLSTER_DIST = 18.0
local PISTOL_HOLSTER_COOLDOWN = 0.45
local PISTOL_HAND_MAG_MESH = "StaticMesh /Game/Game/Art/Items/Handgun/HandgunAmmoClosed_01_A.HandgunAmmoClosed_01_A"
-- Native-style mag eject (from reload.lua): ReloadWeaponAnim → PoseableMesh clone → fall → hip
local PISTOL_MAG_BONE = "magazine_joint"
local PISTOL_MAG_SOCKET = "magazine_socket_joint"
local PISTOL_BONE_CS = 1 -- EBoneSpaces::ComponentSpace
local PISTOL_EJECT_ANIM_DUR = 0.42
local PISTOL_EJECT_ANIM_TRAVEL = 4.0
local PISTOL_EJECT_FALL_DUR = 0.45
local PISTOL_EJECT_FALL_DIST = 55.0
local PISTOL_EJECT_SLIDE_DUR = 0.16
local PISTOL_EJECT_SLIDE_DIST = 8.0
local PISTOL_EJECT_BOUNDS = 16.0
local PISTOL_HAND_ATTACH_X = 4.0
local PISTOL_HAND_ATTACH_Y = 1.5
local PISTOL_HAND_ATTACH_Z = -2.0
local PISTOL_HAND_ATTACH_PITCH = -15.0
local PISTOL_HAND_ATTACH_YAW = 90.0
local PISTOL_HAND_ATTACH_ROLL = 0.0
-- Default: roll 90 so palm sits on top of slide (tune with Slide hand sliders).
local PISTOL_SLIDE_HAND_X = 0.0
local PISTOL_SLIDE_HAND_Y = 0.0
local PISTOL_SLIDE_HAND_Z = 0.0
local PISTOL_SLIDE_HAND_PITCH = 0.0
local PISTOL_SLIDE_HAND_YAW = 0.0
local PISTOL_SLIDE_HAND_ROLL = 90.0
local PISTOL_MAG_KEEP = {
	magazine_socket_joint = true,
	magazine_joint = true,
}
local PISTOL_MAG_HIDE_SIBLINGS = {
	slide_joint = true,
	trigger_joint = true,
	safety_joint = true,
	hammer_01_joint = true,
	FX_muzzle = true,
	Holder_L = true,
	bullet_01_socket_joint = true,
	bullet_01_joint = true,
	cartridge_case_01_joint = true,
	bullet_02_socket_joint = true,
	bullet_02_joint = true,
}
local PISTOL_MAG_ANCESTOR = {
	Holder = true,
	offset_joint = true,
}
-- MCP-confirmed: HasAnyAmmoInClip reads int at weapon+0xBB4 (setg al from cmp [rcx+0xBB4],0).
-- WeaponsAmmoCounts map mirrors the same count (shot 10→9) but raw map write alone does NOT flip HasAnyAmmo.
local PISTOL_CLIP_AMMO_OFFSET = 0xBB4
local RANGED_WEAPONS_AMMO_MAP_OFFSET = 120 -- SHRangedCmbSubcomp.WeaponsAmmoCounts
local PISTOL_AMMO_MAP_FNAME_INDEX = 0xE04F -- live FName ComparisonIndex for pistol clip entry
local PISTOL_AMMO_MAP_ELEM_STRIDE = 20
local PISTOL_RELOAD_FILL_NOTIFY =
	"AnimNotify_ReloadFillWeaponClip /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Handgun/RecoilAndReload/James_Combat_Recoils_Combat_Handgun_Reload.James_Combat_Recoils_Combat_Handgun_Reload.AnimNotify_ReloadFillWeaponClip_0"
local PISTOL_RELOAD_ANIM_SEQ =
	"AnimSequence /Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Handgun/RecoilAndReload/James_Combat_Recoils_Combat_Handgun_Reload.James_Combat_Recoils_Combat_Handgun_Reload"
local PISTOL_HIDE_MAG_BONES = {
	"magazine_joint",
	"magazine_socket_joint",
	"bullet_01_joint",
	"bullet_01_socket_joint",
	"bullet_02_joint",
	"bullet_02_socket_joint",
	"cartridge_case_01_joint",
}

local pistolReload = {
	active = false,
	magEjected = false,
	awaitingRack = false,
	magPullLocked = false,
	slideGripping = false,
	handMag = nil,
	timer = 0.0,
	statusText = "idle",
	wasLeftGrip = false,
	wasXDown = false,
	xPressStarted = nil,
	holsterCooldownUntil = 0.0,
	inHolsterZone = false,
	pulseFrames = 0,
	aimPulseFrames = 0,
	nativeGrace = 0.0,
	allowNative = false, -- kept false: body RangedReload is never allowed
	userInitiated = false,
	playHooked = false,
	cachedReloadPlayer = nil,
	cachedFillNotify = nil,
	cachedReloadAnim = nil,
	slideProgress = 0.0,
	slideGrabAlong = 0.0,
	slideProgressAtGrab = 0.0,
	slideOpenWs = nil,
	slideAxisWs = nil,
	holdingEmptyVisual = false,
	clipEmpty = false,
	wasRightTrigger = false,
	wasRightThumbrest = false,
	slideClosedByRack = false, -- after manual rack, do not force open-slide visual
	handStuckToSlide = false, -- left hand attached to slide_joint (same path as shotgun offhand)
	eject = {
		active = false,
		phase = nil, -- "anim"|"slide"|"fall"|"holstered"|"in_hand"
		start_t = 0.0,
		origin = nil,
		down = nil,
		out = nil,
		mag_rot = nil,
		comp = nil,
		source_mesh = nil,
		used_game_anim = false,
		hand_attached = false,
		rest_bone_poses = nil,
		weapon = nil,
	},
}

local function setPistolReloadStatus(text)
	pistolReload.statusText = text or "idle"
	pcall(function()
		configui.setLabel("pistol_reload_status", "Pistol reload: " .. pistolReload.statusText)
	end)
end

local function pistolVecDist(a, b)
	if a == nil or b == nil then
		return 9999.0
	end
	local dx = (a.X or 0) - (b.X or 0)
	local dy = (a.Y or 0) - (b.Y or 0)
	local dz = (a.Z or 0) - (b.Z or 0)
	return math.sqrt(dx * dx + dy * dy + dz * dz)
end

local function isPistolEquipped(currentPawn)
	local weapon = getEquippedWeapon(currentPawn)
	if weapon == nil then
		return false, nil
	end
	local name = weapon:get_full_name()
	if name ~= nil and string.find(name, "WeaponPistol") then
		return true, weapon
	end
	return false, weapon
end

local function getPistolWeaponMesh(weapon)
	return uevrUtils.getValid(weapon, { "Mesh" })
end

local function getPistolAnim(weapon)
	local mesh = getPistolWeaponMesh(weapon)
	return mesh and mesh.AnimScriptInstance or nil
end

local function getRangedReloadPlayer()
	-- Cached: find_all_of every frame is heavy and unsafe during anim ticks.
	if pistolReload.cachedReloadPlayer ~= nil and uevrUtils.getValid(pistolReload.cachedReloadPlayer) ~= nil then
		return pistolReload.cachedReloadPlayer
	end
	pistolReload.cachedReloadPlayer = nil
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
				pistolReload.cachedReloadPlayer = player
				return player
			end
		end
	end
	return nil
end

local function tryPistolSocketLocation(mesh, socketName)
	if mesh == nil or socketName == nil then
		return nil
	end
	if mesh.GetSocketLocation ~= nil then
		local ok, loc = pcall(function()
			return mesh:GetSocketLocation(uevrUtils.fname_from_string(socketName))
		end)
		if ok and loc ~= nil then
			return loc
		end
	end
	if mesh.GetBoneLocationByName ~= nil then
		local ok, loc = pcall(function()
			return mesh:GetBoneLocationByName(uevrUtils.fname_from_string(socketName), 0)
		end)
		if ok and loc ~= nil then
			return loc
		end
	end
	return nil
end

local function getPistolMagWorldLoc(mesh)
	return tryPistolSocketLocation(mesh, "magazine_socket_joint")
		or tryPistolSocketLocation(mesh, "magazine_joint")
		or (mesh and mesh.K2_GetComponentLocation and mesh:K2_GetComponentLocation())
end

local function getPistolSlideWorldLoc(mesh)
	return tryPistolSocketLocation(mesh, "slide_joint")
end

-- Hip pocket relative to HMD (same offsets as reload.lua).
local function getPistolHolsterWorldPos()
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
	return {
		X = hmd.X + right.X * PISTOL_HOLSTER_RIGHT + up.X * PISTOL_HOLSTER_UP + forward.X * PISTOL_HOLSTER_FORWARD,
		Y = hmd.Y + right.Y * PISTOL_HOLSTER_RIGHT + up.Y * PISTOL_HOLSTER_UP + forward.Y * PISTOL_HOLSTER_FORWARD,
		Z = hmd.Z + right.Z * PISTOL_HOLSTER_RIGHT + up.Z * PISTOL_HOLSTER_UP + forward.Z * PISTOL_HOLSTER_FORWARD,
	}
end

local function pistolHaptic(hand, duration, frequency, amplitude)
	pcall(function()
		uevrUtils.triggerHapticVibration(hand, 0, duration or 0.1, frequency or 1.0, amplitude or 0.5)
	end)
end

-- Game dry-fire / reload SFX (paths confirmed in Videos/.../manual_reload.lua + reload.lua).
local AK_PISTOL_RELOAD =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Weapons/Pistol/Play_Wpn_Pistol_Reload.Play_Wpn_Pistol_Reload"
local AK_PISTOL_NO_AMMO =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Weapons/Pistol/Play_Wpn_Pistol_Shot_No_Ammo.Play_Wpn_Pistol_Shot_No_Ammo"
local AK_PISTOL_MAG_OUT =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Movement/Weapons/Play_Wpn_James_Pistol_Rattle_Hard.Play_Wpn_James_Pistol_Rattle_Hard"
local AK_PISTOL_MAG_IN =
	"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Weapons/Pistol/Play_Wpn_Pistol_Equip.Play_Wpn_Pistol_Equip"
local pistolAkCache = {}

local function findPistolAkEvent(path)
	if path == nil then
		return nil
	end
	if pistolAkCache[path] ~= nil then
		return pistolAkCache[path]
	end
	local obj = nil
	pcall(function()
		obj = uevrUtils.find_required_object(path)
	end)
	if obj == nil then
		pcall(function()
			obj = uevr.api:find_uobject(path)
		end)
	end
	if obj ~= nil then
		pistolAkCache[path] = obj
	end
	return obj
end

local function getPistolWeaponAudio(weapon)
	return uevrUtils.getValid(weapon, { "WeaponItemAudioComponent" })
		or uevrUtils.getValid(weapon, { "WeaponAudioComponent" })
end

local function resolvePistolAkEvent(weapon, kind)
	local audio = getPistolWeaponAudio(weapon)
	-- Full reload montage SFX — only if something explicitly asks for "reload".
	if kind == "reload" then
		if audio ~= nil and audio.ReloadAudioEvent ~= nil then
			return audio.ReloadAudioEvent
		end
		return findPistolAkEvent(AK_PISTOL_RELOAD)
	end
	-- Slide rack / chamber close: short equip click, not the long Reload event.
	if kind == "rack" then
		local evt = findPistolAkEvent(AK_PISTOL_MAG_IN)
		if evt ~= nil then
			return evt
		end
		return findPistolAkEvent(AK_PISTOL_MAG_OUT)
	end
	if kind == "no_ammo" or kind == "slide_open" or kind == "dry_fire" then
		if audio ~= nil and audio.OutOfAmmoAudioEvent ~= nil then
			return audio.OutOfAmmoAudioEvent
		end
		return findPistolAkEvent(AK_PISTOL_NO_AMMO)
	end
	if kind == "mag_out" then
		return findPistolAkEvent(AK_PISTOL_MAG_OUT) or (audio and audio.ReloadAudioEvent) or findPistolAkEvent(AK_PISTOL_RELOAD)
	end
	if kind == "mag_in" then
		local evt = findPistolAkEvent(AK_PISTOL_MAG_IN)
		if evt ~= nil then
			return evt
		end
		if audio ~= nil and audio.ReloadAudioEvent ~= nil then
			return audio.ReloadAudioEvent
		end
		return findPistolAkEvent(AK_PISTOL_RELOAD)
	end
	return nil
end

local function playPistolAk(weapon, kind)
	if weapon == nil or kind == nil then
		return false
	end
	local event = resolvePistolAkEvent(weapon, kind)
	if event == nil then
		return false
	end
	-- Soft refs sometimes need LoadSynchronous
	pcall(function()
		if event.LoadSynchronous ~= nil then
			local loaded = event:LoadSynchronous()
			if loaded ~= nil then
				event = loaded
			end
		end
	end)
	local played = false
	local audio = getPistolWeaponAudio(weapon)
	pcall(function()
		if event.PostOnActor ~= nil then
			event:PostOnActor(weapon, nil, 0, true)
			played = true
		end
	end)
	if not played and audio ~= nil and audio.SimplePostAkEvent ~= nil then
		pcall(function()
			audio:SimplePostAkEvent(event)
			played = true
		end)
	end
	if not played and audio ~= nil and audio.PostAkEvent ~= nil then
		pcall(function()
			audio:PostAkEvent(event, 0, nil, "", true)
			played = true
		end)
	end
	return played
end

local function setPistolOutOfAmmoVisual(weapon, empty)
	-- Don't fight the player closing/keeping the slide shut after a manual rack.
	if empty == true then
		if pistolReload.slideClosedByRack then
			return
		end
		if pistolReload.awaitingRack or pistolReload.slideGripping or pistolReload.magPullLocked then
			return
		end
	end
	local anim = getPistolAnim(weapon)
	if anim ~= nil and anim.SetOutOfAmmo ~= nil then
		pcall(function()
			anim:SetOutOfAmmo(empty == true)
		end)
	end
	pistolReload.holdingEmptyVisual = empty == true
end

-- Dry-fire click: prefer Wwise OutOfAmmo event (OnOutOfAmmoPerformed often silent while we eat LT/RT).
local function playPistolEmptyTriggerClick(weapon)
	if weapon == nil then
		return
	end
	local played = playPistolAk(weapon, "dry_fire")
	if not played then
		pcall(function()
			if weapon.OnOutOfAmmoPerformed ~= nil then
				weapon:OnOutOfAmmoPerformed()
				played = true
			end
		end)
	end
	pistolHaptic(1, 0.05, 0.55, 0.4)
end

local function playPistolSlideOpenSfx(weapon)
	playPistolAk(weapon, "slide_open")
	pistolHaptic(1, 0.08, 0.5, 0.55)
end

local function hidePistolMagBones(mesh)
	if mesh == nil or mesh.HideBoneByName == nil then
		return
	end
	for _, bone in ipairs(PISTOL_HIDE_MAG_BONES) do
		pcall(function()
			mesh:HideBoneByName(uevrUtils.fname_from_string(bone), 0)
		end)
	end
end

local function showPistolMagBones(mesh)
	if mesh == nil or mesh.UnHideBoneByName == nil then
		return
	end
	for _, bone in ipairs(PISTOL_HIDE_MAG_BONES) do
		pcall(function()
			mesh:UnHideBoneByName(uevrUtils.fname_from_string(bone))
		end)
	end
end

--------------------------------------------------------------------------
-- Native-style mag eject visual (ported from Videos/.../reload.lua)
--------------------------------------------------------------------------

local function getPistolBoneWorldPos(mesh, boneName)
	if mesh == nil or boneName == nil then
		return nil
	end
	local pos = nil
	pcall(function()
		if mesh.GetSocketLocation ~= nil then
			pos = mesh:GetSocketLocation(uevrUtils.fname_from_string(boneName))
		end
	end)
	if pos == nil then
		pcall(function()
			if mesh.GetBoneLocation ~= nil then
				pos = mesh:GetBoneLocation(uevrUtils.fname_from_string(boneName))
			end
		end)
	end
	return pos
end

local function getPistolBoneWorldRot(mesh, boneName)
	if mesh == nil or boneName == nil then
		return nil
	end
	local rot = nil
	pcall(function()
		if mesh.GetSocketRotation ~= nil then
			rot = mesh:GetSocketRotation(uevrUtils.fname_from_string(boneName))
		end
	end)
	return rot
end

local function refreshPistolMagVisualBounds(comp)
	if comp == nil then
		return
	end
	pcall(function()
		comp.BoundsScale = PISTOL_EJECT_BOUNDS
	end)
	pcall(function()
		comp.bUseAttachParentBound = false
	end)
	pcall(function()
		if comp.UpdateBounds ~= nil then
			comp:UpdateBounds()
		end
	end)
	pcall(function()
		if comp.MarkRenderTransformDirty ~= nil then
			comp:MarkRenderTransformDirty()
		end
	end)
end

local function showOnlyPistolMagazine(poseable)
	if poseable == nil then
		return
	end
	local tiny = uevrUtils.vector(0.001, 0.001, 0.001)
	local one = uevrUtils.vector(1, 1, 1)
	local WS = 0
	local n = 0
	pcall(function()
		n = poseable:GetNumBones()
	end)
	if type(n) ~= "number" then
		return
	end
	for i = 0, n - 1 do
		local name = nil
		pcall(function()
			name = poseable:GetBoneName(i):to_string()
		end)
		if name ~= nil then
			local keep = PISTOL_MAG_KEEP[name] == true
			local hideSibling = PISTOL_MAG_HIDE_SIBLINGS[name] == true
			pcall(function()
				if poseable.SetBoneScaleByName ~= nil then
					poseable:SetBoneScaleByName(uevrUtils.fname_from_string(name), keep and one or tiny, WS)
				end
			end)
			if keep then
				pcall(function()
					if poseable.UnHideBoneByName ~= nil then
						poseable:UnHideBoneByName(uevrUtils.fname_from_string(name))
					end
				end)
			elseif hideSibling or PISTOL_MAG_ANCESTOR[name] ~= true then
				pcall(function()
					if poseable.HideBoneByName ~= nil then
						poseable:HideBoneByName(uevrUtils.fname_from_string(name), 0)
					end
				end)
			end
		end
	end
	refreshPistolMagVisualBounds(poseable)
end

local function drivePistolMagWorldPose(poseable, pos, rot)
	if poseable == nil or pos == nil then
		return
	end
	pcall(function()
		if poseable.SetBoneLocationByName ~= nil then
			poseable:SetBoneLocationByName(
				uevrUtils.fname_from_string(PISTOL_MAG_BONE),
				uevrUtils.vector(pos.X, pos.Y, pos.Z),
				0
			)
		end
	end)
	if rot ~= nil then
		pcall(function()
			if poseable.SetBoneRotationByName ~= nil then
				poseable:SetBoneRotationByName(uevrUtils.fname_from_string(PISTOL_MAG_BONE), rot, 0)
			end
		end)
	end
	pcall(function()
		if poseable.SetBoneScaleByName ~= nil then
			poseable:SetBoneScaleByName(
				uevrUtils.fname_from_string(PISTOL_MAG_BONE),
				uevrUtils.vector(1, 1, 1),
				0
			)
		end
	end)
	refreshPistolMagVisualBounds(poseable)
end

local function copyPistolMagPoseFromSource(comp, sourceMesh, applyMagFilter)
	if comp == nil or sourceMesh == nil then
		return false
	end
	local ok = false
	pcall(function()
		if comp.CopyPoseFromSkeletalComponent ~= nil then
			comp:CopyPoseFromSkeletalComponent(sourceMesh)
			ok = true
		end
	end)
	if ok and applyMagFilter ~= false then
		showOnlyPistolMagazine(comp)
	end
	return ok
end

local function capturePistolMagCloneRestPose(comp)
	local ej = pistolReload.eject
	if comp == nil or ej.rest_bone_poses ~= nil then
		return
	end
	local poses = {}
	local n = 0
	pcall(function()
		n = comp:GetNumBones()
	end)
	if type(n) ~= "number" or n <= 0 then
		return
	end
	for i = 0, n - 1 do
		local name = nil
		pcall(function()
			name = comp:GetBoneName(i):to_string()
		end)
		if name ~= nil then
			local fname = uevrUtils.fname_from_string(name)
			local entry = {}
			pcall(function()
				if comp.GetBoneLocationByName ~= nil then
					entry.loc = comp:GetBoneLocationByName(fname, PISTOL_BONE_CS)
				end
			end)
			pcall(function()
				if comp.GetBoneRotationByName ~= nil then
					entry.rot = comp:GetBoneRotationByName(fname, PISTOL_BONE_CS)
				end
			end)
			pcall(function()
				if comp.GetBoneScaleByName ~= nil then
					entry.scale = comp:GetBoneScaleByName(fname, PISTOL_BONE_CS)
				end
			end)
			if entry.loc ~= nil or entry.rot ~= nil then
				poses[name] = entry
			end
		end
	end
	ej.rest_bone_poses = poses
end

local function restorePistolMagCloneRestPose(comp)
	local poses = pistolReload.eject.rest_bone_poses
	if comp == nil or poses == nil then
		return
	end
	for name, entry in pairs(poses) do
		local fname = uevrUtils.fname_from_string(name)
		pcall(function()
			if entry.loc ~= nil and comp.SetBoneLocationByName ~= nil then
				comp:SetBoneLocationByName(fname, entry.loc, PISTOL_BONE_CS)
			end
		end)
		pcall(function()
			if entry.rot ~= nil and comp.SetBoneRotationByName ~= nil then
				comp:SetBoneRotationByName(fname, entry.rot, PISTOL_BONE_CS)
			end
		end)
		pcall(function()
			if entry.scale ~= nil and comp.SetBoneScaleByName ~= nil then
				comp:SetBoneScaleByName(fname, entry.scale, PISTOL_BONE_CS)
			end
		end)
	end
	showOnlyPistolMagazine(comp)
	refreshPistolMagVisualBounds(comp)
end

local function stopPistolWeaponReloadAnim(weapon)
	if weapon == nil then
		return
	end
	local animInst = getPistolAnim(weapon)
	if animInst == nil then
		return
	end
	pcall(function()
		if animInst.StopSlotAnimation ~= nil then
			animInst:StopSlotAnimation(0.0, uevrUtils.fname_from_string("DefaultSlot"))
		end
	end)
	pcall(function()
		if animInst.GetCurrentActiveMontage == nil or animInst.Montage_Stop == nil then
			return
		end
		local montage = animInst:GetCurrentActiveMontage()
		if montage ~= nil then
			local ok, name = pcall(function()
				return montage:get_full_name()
			end)
			if ok and name ~= nil and string.find(string.lower(name), "reload", 1, true) ~= nil then
				animInst:Montage_Stop(0.0, montage)
			end
		end
	end)
end

local function tryPlayPistolWeaponReloadAnim(weapon)
	local played = false
	pcall(function()
		local soft = weapon.ReloadWeaponAnim
		local animAsset = soft
		if soft ~= nil and soft.LoadSynchronous ~= nil then
			animAsset = soft:LoadSynchronous()
		end
		if animAsset == nil then
			return
		end
		local animInst = getPistolAnim(weapon)
		if animInst == nil or animInst.PlaySlotAnimationAsDynamicMontage == nil then
			return
		end
		animInst:PlaySlotAnimationAsDynamicMontage(
			animAsset,
			uevrUtils.fname_from_string("DefaultSlot"),
			0.05,
			0.05,
			1.0,
			1,
			0.0,
			0.0
		)
		played = true
	end)
	return played
end

local function detachPistolMagVisual(comp)
	if comp == nil then
		return
	end
	pcall(function()
		if comp.K2_DetachFromComponent ~= nil then
			comp:K2_DetachFromComponent(1, 1, 1)
		elseif comp.DetachFromParent ~= nil then
			comp:DetachFromParent(true, true)
		end
	end)
end

local function getPistolHandMagAttach()
	return {
		x = configui.getValue("pistol_hand_mag_x") or PISTOL_HAND_ATTACH_X,
		y = configui.getValue("pistol_hand_mag_y") or PISTOL_HAND_ATTACH_Y,
		z = configui.getValue("pistol_hand_mag_z") or PISTOL_HAND_ATTACH_Z,
		pitch = configui.getValue("pistol_hand_mag_pitch") or PISTOL_HAND_ATTACH_PITCH,
		yaw = configui.getValue("pistol_hand_mag_yaw") or PISTOL_HAND_ATTACH_YAW,
		roll = configui.getValue("pistol_hand_mag_roll") or PISTOL_HAND_ATTACH_ROLL,
	}
end

local function applyPistolHandAttachTransform(comp)
	if comp == nil then
		return
	end
	local a = getPistolHandMagAttach()
	pcall(function()
		comp:K2_SetRelativeLocation(
			uevrUtils.vector(a.x, a.y, a.z),
			false,
			reusable_hit_result,
			false
		)
		comp:K2_SetRelativeRotation(
			uevrUtils.rotator(a.pitch, a.yaw, a.roll),
			false,
			reusable_hit_result,
			false
		)
	end)
end

local function syncPistolMagVisualToHolster(comp)
	if comp == nil then
		return
	end
	local pos = getPistolHolsterWorldPos()
	if pos == nil then
		return
	end
	local rot = controllers.getControllerRotation(0)
	restorePistolMagCloneRestPose(comp)
	drivePistolMagWorldPose(comp, pos, rot)
end

local function destroyPistolEjectMag()
	local ej = pistolReload.eject
	local weapon = ej.weapon
	local comp = ej.comp
	local usedAnim = ej.used_game_anim == true
	ej.active = false
	ej.phase = nil
	ej.origin = nil
	ej.down = nil
	ej.out = nil
	ej.mag_rot = nil
	ej.comp = nil
	ej.source_mesh = nil
	ej.used_game_anim = false
	ej.hand_attached = false
	ej.rest_bone_poses = nil
	ej.weapon = nil
	if pistolReload.handMag ~= nil and pistolReload.handMag == comp then
		pistolReload.handMag = nil
	end
	if usedAnim then
		stopPistolWeaponReloadAnim(weapon)
	end
	if comp == nil then
		return
	end
	pcall(function()
		if comp.K2_DestroyComponent ~= nil then
			comp:K2_DestroyComponent(comp)
		elseif comp.DestroyComponent ~= nil then
			comp:DestroyComponent(true)
		end
	end)
end

local function parkPistolMagAtHolster(comp)
	local ej = pistolReload.eject
	comp = uevrUtils.getValid(comp) or uevrUtils.getValid(ej.comp)
	if comp == nil then
		return
	end
	detachPistolMagVisual(comp)
	ej.hand_attached = false
	syncPistolMagVisualToHolster(comp)
	ej.comp = comp
	ej.active = true
	ej.phase = "holstered"
	ej.origin = nil
	ej.down = nil
	ej.out = nil
	pistolReload.handMag = nil
	pistolReload.magEjected = true
end

local function attachPistolMagToLeftHand(comp)
	local ej = pistolReload.eject
	comp = uevrUtils.getValid(comp) or uevrUtils.getValid(ej.comp)
	if comp == nil then
		return false
	end
	showOnlyPistolMagazine(comp)
	pcall(function()
		if comp.SetVisibility ~= nil then
			comp:SetVisibility(true, true)
		end
		if comp.SetHiddenInGame ~= nil then
			comp:SetHiddenInGame(false, true)
		end
	end)
	restorePistolMagCloneRestPose(comp)
	if not ej.hand_attached then
		detachPistolMagVisual(comp)
		local ok = controllers.attachComponentToController(0, comp, "", 0, false, true)
		if ok ~= true then
			print("[pistol_reload] attach eject mag to hand failed")
			return false
		end
		ej.hand_attached = true
	end
	applyPistolHandAttachTransform(comp)
	ej.comp = comp
	ej.active = true
	ej.phase = "in_hand"
	pistolReload.handMag = comp
	return true
end

local function beginPistolEjectFall(comp, magPos)
	local ej = pistolReload.eject
	capturePistolMagCloneRestPose(comp)
	local now = os.clock()
	local down = uevrUtils.vector(0, 0, -1)
	ej.phase = "fall"
	ej.start_t = now
	ej.origin = { X = magPos.X, Y = magPos.Y, Z = magPos.Z }
	ej.down = { X = down.X, Y = down.Y, Z = down.Z }
	if ej.used_game_anim then
		stopPistolWeaponReloadAnim(ej.weapon)
		ej.used_game_anim = false
	end
end

local function startPistolMagEjectAnim(weapon)
	destroyPistolEjectMag()
	local mesh = getPistolWeaponMesh(weapon)
	if mesh == nil then
		print("[pistol_reload] startMagEjectAnim: no weapon mesh")
		return false
	end

	local magPos = getPistolBoneWorldPos(mesh, PISTOL_MAG_BONE) or getPistolBoneWorldPos(mesh, PISTOL_MAG_SOCKET)
	local magRot = getPistolBoneWorldRot(mesh, PISTOL_MAG_BONE) or getPistolBoneWorldRot(mesh, PISTOL_MAG_SOCKET)
	if magPos == nil then
		pcall(function()
			magPos = mesh:K2_GetComponentLocation()
		end)
	end
	if magPos == nil then
		print("[pistol_reload] startMagEjectAnim: no mag pose — hide only")
		hidePistolMagBones(mesh)
		return false
	end

	local parent = getLocalPawn() or weapon
	local comp = nil
	pcall(function()
		comp = uevrUtils.createPoseableMeshFromSkeletalMesh(mesh, {
			parent = parent,
			manualAttachment = true,
			useDefaultPose = false,
			showDebug = false,
		})
	end)
	if comp == nil then
		print("[pistol_reload] PoseableMesh clone failed — hide only")
		hidePistolMagBones(mesh)
		return false
	end

	pcall(function()
		if comp.SetCollisionEnabled ~= nil then
			comp:SetCollisionEnabled(0)
		end
		if comp.SetRenderInMainPass ~= nil then
			comp:SetRenderInMainPass(true)
		end
		if comp.SetVisibility ~= nil then
			comp:SetVisibility(true, true)
		end
		if comp.SetHiddenInGame ~= nil then
			comp:SetHiddenInGame(false, true)
		end
		if comp.K2_DetachFromComponent ~= nil then
			comp:K2_DetachFromComponent(1, 1, 1)
		elseif comp.DetachFromParent ~= nil then
			comp:DetachFromParent(true, true)
		end
	end)

	pcall(function()
		local meshLoc = mesh:K2_GetComponentLocation()
		local meshRot = mesh:K2_GetComponentRotation()
		if meshLoc ~= nil then
			comp:K2_SetWorldLocation(meshLoc, false, reusable_hit_result, false)
		end
		if meshRot ~= nil then
			comp:K2_SetWorldRotation(meshRot, false, reusable_hit_result, false)
		end
	end)

	showOnlyPistolMagazine(comp)
	copyPistolMagPoseFromSource(comp, mesh)
	hidePistolMagBones(mesh)

	local usedAnim = tryPlayPistolWeaponReloadAnim(weapon)
	local down = uevrUtils.vector(0, 0, -1)
	local out = uevrUtils.vector(0, 0, -1)
	local root = uevrUtils.getValid(weapon, { "RootComponent" })
	if root ~= nil then
		pcall(function()
			if root.GetUpVector ~= nil then
				local up = root:GetUpVector()
				out = uevrUtils.vector(-up.X, -up.Y, -up.Z)
			end
		end)
	end

	local ej = pistolReload.eject
	ej.active = true
	ej.start_t = os.clock()
	ej.origin = { X = magPos.X, Y = magPos.Y, Z = magPos.Z }
	ej.out = { X = out.X, Y = out.Y, Z = out.Z }
	ej.down = { X = down.X, Y = down.Y, Z = down.Z }
	ej.mag_rot = magRot
	ej.comp = comp
	ej.source_mesh = mesh
	ej.used_game_anim = usedAnim
	ej.weapon = weapon
	ej.hand_attached = false
	ej.rest_bone_poses = nil

	if usedAnim then
		ej.phase = "anim"
		print("[pistol_reload] eject: game ReloadWeaponAnim → mag clone")
	else
		ej.phase = "slide"
		copyPistolMagPoseFromSource(comp, mesh)
		capturePistolMagCloneRestPose(comp)
		drivePistolMagWorldPose(comp, magPos, magRot)
		print("[pistol_reload] eject: procedural fallback")
	end
	return true
end

local function tickPistolMagEjectAnim()
	local ej = pistolReload.eject
	if not ej.active or ej.comp == nil then
		return
	end
	local comp = uevrUtils.getValid(ej.comp)
	if comp == nil then
		destroyPistolEjectMag()
		return
	end

	pcall(function()
		if comp.SetVisibility ~= nil then
			comp:SetVisibility(true, true)
		end
		if comp.SetHiddenInGame ~= nil then
			comp:SetHiddenInGame(false, true)
		end
	end)

	local now = os.clock()
	local elapsed = now - (ej.start_t or now)
	local phase = ej.phase

	if phase == "holstered" then
		syncPistolMagVisualToHolster(comp)
		return
	end

	if phase == "in_hand" then
		showOnlyPistolMagazine(comp)
		if ej.hand_attached then
			applyPistolHandAttachTransform(comp)
		end
		return
	end

	local o = ej.origin
	local out = ej.out
	local down = ej.down
	if o == nil or out == nil or down == nil then
		destroyPistolEjectMag()
		return
	end

	if phase == "anim" then
		local sourceMesh = uevrUtils.getValid(ej.source_mesh)
		if sourceMesh ~= nil then
			copyPistolMagPoseFromSource(comp, sourceMesh)
		end
		local magPos = getPistolBoneWorldPos(comp, PISTOL_MAG_BONE) or getPistolBoneWorldPos(comp, PISTOL_MAG_SOCKET)
		if magPos ~= nil then
			local traveled = pistolVecDist(magPos, o)
			if elapsed >= PISTOL_EJECT_ANIM_DUR or traveled >= PISTOL_EJECT_ANIM_TRAVEL then
				beginPistolEjectFall(comp, magPos)
			end
		elseif elapsed >= PISTOL_EJECT_ANIM_DUR then
			beginPistolEjectFall(comp, o)
		end
		return
	end

	showOnlyPistolMagazine(comp)
	local x, y, z = o.X, o.Y, o.Z

	if phase == "slide" then
		local t = math.min(elapsed / PISTOL_EJECT_SLIDE_DUR, 1.0)
		local e = 1.0 - (1.0 - t) * (1.0 - t)
		local d = PISTOL_EJECT_SLIDE_DIST * e
		x = o.X + out.X * d
		y = o.Y + out.Y * d
		z = o.Z + out.Z * d
		if t >= 1.0 then
			beginPistolEjectFall(comp, { X = x, Y = y, Z = z })
			return
		end
		drivePistolMagWorldPose(comp, { X = x, Y = y, Z = z }, ej.mag_rot)
	elseif phase == "fall" then
		local fallElapsed = now - (ej.start_t or now)
		local t = math.min(fallElapsed / PISTOL_EJECT_FALL_DUR, 1.0)
		local e = t * t
		local d = PISTOL_EJECT_FALL_DIST * e
		x = o.X + down.X * d
		y = o.Y + down.Y * d
		z = o.Z + down.Z * d
		if t >= 1.0 then
			parkPistolMagAtHolster(comp)
			setPistolReloadStatus("mag at hip — grip near hip to grab")
			return
		end
		drivePistolMagWorldPose(comp, { X = x, Y = y, Z = z }, ej.mag_rot)
	end
end

local function canGrabPistolEjectMag()
	if not pistolReload.magEjected or pistolReload.handMag ~= nil then
		return false
	end
	local phase = pistolReload.eject.phase
	if phase == "anim" or phase == "slide" or phase == "fall" then
		return false
	end
	return true
end

local function isPistolEjectHandMag(comp)
	return comp ~= nil and pistolReload.eject.comp ~= nil and comp == pistolReload.eject.comp
end

local function destroyPistolHandMag()
	if pistolReload.handMag == nil then
		return
	end
	local comp = pistolReload.handMag
	pistolReload.handMag = nil
	if isPistolEjectHandMag(comp) then
		pistolReload.eject.hand_attached = false
		-- Eject clone is owned by destroyPistolEjectMag / park — do not destroy here.
		return
	end
	pcall(function()
		if comp.K2_DestroyComponent ~= nil then
			comp:K2_DestroyComponent(comp)
		end
	end)
end

local function spawnPistolHandMag()
	destroyPistolHandMag()
	local comp = nil
	pcall(function()
		comp = uevrUtils.createStaticMeshComponent(PISTOL_HAND_MAG_MESH, {
			collisionEnabled = false,
			visible = true,
		})
	end)
	if comp == nil then
		print("[pistol_reload] failed to spawn hand mag mesh")
		return nil
	end
	pcall(function()
		if comp.SetRelativeScale3D ~= nil then
			comp:SetRelativeScale3D(uevrUtils.vector(0.9, 0.9, 0.9))
		end
		controllers.attachComponentToController(0, comp, "", 0, false)
	end)
	applyPistolHandAttachTransform(comp)
	pistolReload.handMag = comp
	print("[pistol_reload] hand mag spawned (static fallback)")
	return comp
end

-- Same path as shotgun two-hand offhand: hands attach via on_accessory_attach to a weapon socket.
local function getPistolSlideHandAttachParams()
	local socket = "slide_joint"
	local gripAnim = "rifle_offhand"
	local attachType = 0
	-- Prefer Manual Reload sliders (live-tunable). Fall back to accessory marker, then defaults.
	local loc = {
		configui.getValue("pistol_slide_hand_x") or PISTOL_SLIDE_HAND_X,
		configui.getValue("pistol_slide_hand_y") or PISTOL_SLIDE_HAND_Y,
		configui.getValue("pistol_slide_hand_z") or PISTOL_SLIDE_HAND_Z,
	}
	local rot = {
		configui.getValue("pistol_slide_hand_pitch") or PISTOL_SLIDE_HAND_PITCH,
		configui.getValue("pistol_slide_hand_yaw") or PISTOL_SLIDE_HAND_YAW,
		configui.getValue("pistol_slide_hand_roll") or PISTOL_SLIDE_HAND_ROLL,
	}
	pcall(function()
		local att = attachments.getCurrentGrippedAttachment(Handed.Right)
		if att == nil then
			return
		end
		local attachmentID = attachments.getAttachmentIDFromAttachment(att)
		if attachmentID == nil or attachmentID == "" then
			return
		end
		local list = accessories.getAccessoriesForAttachment(attachmentID)
		if list == nil then
			return
		end
		for _, accessoryParams in pairs(list) do
			local marker = accessories.getPrimaryMarkerParams(accessoryParams)
			if marker ~= nil then
				if type(marker.socket_name) == "string" and marker.socket_name ~= "" then
					socket = marker.socket_name
				end
				if type(marker.grip_animation) == "string" and marker.grip_animation ~= "" then
					gripAnim = marker.grip_animation
				end
				if marker.attach_type ~= nil then
					attachType = marker.attach_type
				end
				break
			end
		end
	end)
	return socket, loc, rot, gripAnim, attachType
end

local function detachPistolHandFromSlide()
	if not pistolReload.handStuckToSlide then
		return
	end
	pistolReload.handStuckToSlide = false
	pcall(function()
		uevrUtils.executeUEVRCallbacks("on_accessory_detach", Handed.Left)
	end)
	pcall(function()
		uevrUtils.executeUEVRCallbacks("on_accessory_animation", Handed.Left, nil)
	end)
end

local function attachPistolHandToSlide(mesh)
	if mesh == nil then
		return false
	end
	local socket, loc, rot, gripAnim, attachType = getPistolSlideHandAttachParams()
	local ok = pcall(function()
		uevrUtils.executeUEVRCallbacks(
			"on_accessory_attach",
			Handed.Left,
			mesh,
			socket,
			attachType or 0,
			loc,
			rot
		)
	end)
	if not ok then
		return false
	end
	pcall(function()
		uevrUtils.executeUEVRCallbacks("on_accessory_animation", Handed.Left, gripAnim)
	end)
	pistolReload.handStuckToSlide = true
	return true
end

-- Re-apply loc/rot while stuck so Manual Reload sliders update live.
local function refreshPistolSlideHandPose(mesh)
	if not pistolReload.handStuckToSlide or mesh == nil then
		return
	end
	local socket, loc, rot, gripAnim, attachType = getPistolSlideHandAttachParams()
	pcall(function()
		uevrUtils.executeUEVRCallbacks(
			"on_accessory_attach",
			Handed.Left,
			mesh,
			socket,
			attachType or 0,
			loc,
			rot
		)
	end)
	pcall(function()
		uevrUtils.executeUEVRCallbacks("on_accessory_animation", Handed.Left, gripAnim)
	end)
end

local function resetPistolManualReload(statusText)
	detachPistolHandFromSlide()
	destroyPistolHandMag()
	destroyPistolEjectMag()
	local ok, weapon = isPistolEquipped(getLocalPawn())
	if ok then
		showPistolMagBones(getPistolWeaponMesh(weapon))
		stopPistolWeaponReloadAnim(weapon)
	end
	pistolReload.active = false
	pistolReload.magEjected = false
	pistolReload.awaitingRack = false
	pistolReload.magPullLocked = false
	pistolReload.slideGripping = false
	pistolReload.timer = 0.0
	pistolReload.slideProgress = 0.0
	pistolReload.slideGrabAlong = 0.0
	pistolReload.slideProgressAtGrab = 0.0
	pistolReload.slideOpenWs = nil
	pistolReload.slideAxisWs = nil
	pistolReload.pulseFrames = 0
	pistolReload.aimPulseFrames = 0
	pistolReload.nativeGrace = 0.0
	pistolReload.allowNative = false
	pistolReload.userInitiated = false
	pistolReload.clipEmpty = false
	pistolReload.wasRightTrigger = false
	pistolReload.wasRightThumbrest = false
	pistolReload.slideClosedByRack = false
	pistolReload.xPressStarted = nil
	pistolReload.holsterCooldownUntil = 0.0
	pistolReload.inHolsterZone = false
	setPistolReloadStatus(statusText or "idle")
end

-- X drop: native-style eject clone → fall → hip (grab with left grip).
local function dropPistolMagViaX(weapon, mesh)
	if weapon == nil or mesh == nil then
		return false
	end
	if pistolReload.handMag ~= nil or pistolReload.awaitingRack then
		return false
	end
	if pistolReload.magEjected then
		return false
	end
	pistolReload.slideClosedByRack = false
	destroyPistolHandMag()
	local ok = startPistolMagEjectAnim(weapon)
	if not ok then
		hidePistolMagBones(mesh)
	end
	pistolReload.magEjected = true
	pistolReload.active = true
	pistolReload.timer = PISTOL_RELOAD_TIMEOUT
	pistolReload.magPullLocked = false
	pistolReload.slideGripping = false
	pistolReload.holsterCooldownUntil = os.clock() + PISTOL_HOLSTER_COOLDOWN
	pistolHaptic(1, 0.08, 0.45, 0.6)
	playPistolAk(weapon, "mag_out")
	setPistolReloadStatus(ok and "mag ejecting…" or "mag dropped — grip near hip to grab")
	print("[pistol_reload] mag dropped via X (eject visual=" .. tostring(ok) .. ")")
	return true
end

local function grabPistolMagFromHip(mesh)
	if not canGrabPistolEjectMag() then
		return false
	end
	if os.clock() < (pistolReload.holsterCooldownUntil or 0) then
		return false
	end
	pistolReload.slideClosedByRack = false
	hidePistolMagBones(mesh)

	local ej = pistolReload.eject
	local grabbed = false
	if ej.comp ~= nil and (ej.phase == "holstered" or ej.phase == nil) then
		grabbed = attachPistolMagToLeftHand(ej.comp)
	end
	if not grabbed then
		spawnPistolHandMag()
		grabbed = pistolReload.handMag ~= nil
	end
	if not grabbed then
		return false
	end

	pistolReload.active = true
	pistolReload.timer = PISTOL_RELOAD_TIMEOUT
	pistolReload.holsterCooldownUntil = os.clock() + PISTOL_HOLSTER_COOLDOWN
	pistolHaptic(0, 0.1, 0.6, 0.75)
	do
		local okW, w = isPistolEquipped(getLocalPawn())
		if okW then
			playPistolAk(w, "mag_out")
		end
	end
	setPistolReloadStatus("mag grabbed — dock at well and release")
	print("[pistol_reload] mag grabbed from hip")
	return true
end

local function shouldAllowPistolNativeReload()
	-- Body RangedReload montage is never allowed while manual reload is on.
	return false
end

local function isPistolRangedReloadPlayer(obj)
	if obj == nil then
		return false
	end
	local ok, name = pcall(function()
		return obj:get_full_name()
	end)
	return ok and name ~= nil and string.find(name, "RangedReload") ~= nil and string.find(name, "SHCharacterPlay") ~= nil
end

local function abortPistolBodyReloadAnim()
	local player = getRangedReloadPlayer()
	if player == nil or player.StopRequest == nil then
		return
	end
	local ok, playing = pcall(function()
		return player.IsPlaying ~= nil and player:IsPlaying()
	end)
	if ok and playing then
		pcall(function()
			player:StopRequest(0.0)
		end)
	end
end

local function getPistolReloadFillNotify()
	if pistolReload.cachedFillNotify ~= nil and uevrUtils.getValid(pistolReload.cachedFillNotify) ~= nil then
		return pistolReload.cachedFillNotify
	end
	pistolReload.cachedFillNotify = nil
	local ok, obj = pcall(function()
		return uevr.api:find_uobject(PISTOL_RELOAD_FILL_NOTIFY)
	end)
	if ok and obj ~= nil then
		pistolReload.cachedFillNotify = obj
		return obj
	end
	local found = uevrUtils.find_all_of("Class /Script/SHProto.AnimNotify_ReloadFillWeaponClip", false) or {}
	for _, n in ipairs(found) do
		local okName, name = pcall(function()
			return n:get_full_name()
		end)
		if okName and name ~= nil and string.find(name, "Handgun_Reload") then
			pistolReload.cachedFillNotify = n
			return n
		end
	end
	return nil
end

local function getPistolReloadAnimSeq()
	if pistolReload.cachedReloadAnim ~= nil and uevrUtils.getValid(pistolReload.cachedReloadAnim) ~= nil then
		return pistolReload.cachedReloadAnim
	end
	pistolReload.cachedReloadAnim = nil
	local ok, obj = pcall(function()
		return uevr.api:find_uobject(PISTOL_RELOAD_ANIM_SEQ)
	end)
	if ok and obj ~= nil then
		pistolReload.cachedReloadAnim = obj
		return obj
	end
	return nil
end

local function uobjectWriteI32(obj, offset, value)
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
			error("no write_dword/write_byte")
		end
	end)
	return ok == true
end

local function uobjectReadI32(obj, offset)
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

local function uobjectReadU64(obj, offset)
	if obj == nil then
		return nil
	end
	local ok, val = pcall(function()
		if obj.read_qword ~= nil then
			return obj:read_qword(offset)
		end
		return nil
	end)
	if ok then
		return val
	end
	return nil
end

local function getPistolRangedSubcomp()
	local okFind, comps = pcall(function()
		return uevrUtils.find_all_of("Class /Script/SHProto.SHRangedCmbSubcomp", false) or {}
	end)
	if not okFind then
		return nil
	end
	for _, c in ipairs(comps) do
		local okN, nm = pcall(function()
			return c:get_full_name()
		end)
		if okN and nm ~= nil and string.find(nm, "SHCharacterPlay") and not string.find(nm, "Default__") then
			return c
		end
	end
	return nil
end

local function syncPistolAmmoMapCount(ranged, count)
	if ranged == nil then
		return false
	end
	-- TMap pair array pointer at struct start (offset 0 of MapProperty / +120 on subcomp).
	local dataPtr = uobjectReadU64(ranged, RANGED_WEAPONS_AMMO_MAP_OFFSET)
	local num = uobjectReadI32(ranged, RANGED_WEAPONS_AMMO_MAP_OFFSET + 8)
	if dataPtr == nil or num == nil or num <= 0 then
		return false
	end
	-- Write via temporary: use ranged write relative? Data is heap — need absolute.
	-- UObject write_* is relative to object base; heap writes need a different path.
	-- Prefer syncing only weapon+0xBB4 (HasAnyAmmo source). Map mirrors on next native fire/reload.
	return false
end

local function getPistolItemsComponent()
	local pawn = getLocalPawn()
	local items = uevrUtils.getValid(pawn, { "Items" })
	if items ~= nil then
		return items
	end
	local okFind, comps = pcall(function()
		return uevrUtils.find_all_of("Class /Script/SHProto.SHCharacterPlayItemsComponent", false) or {}
	end)
	if not okFind then
		return nil
	end
	for _, c in ipairs(comps) do
		local okN, nm = pcall(function()
			return c:get_full_name()
		end)
		if okN and nm ~= nil and string.find(nm, "SHCharacterPlay") and not string.find(nm, "Default__") then
			return c
		end
	end
	return nil
end

local function getWeaponAmmoRowName(weapon)
	local rowStr = "HandgunAmmo"
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
	local items = getPistolItemsComponent()
	if items == nil or items.GetItemCount == nil then
		return nil
	end
	local rowName = getWeaponAmmoRowName(weapon)
	local count = nil
	pcall(function()
		count = items:GetItemCount(uevrUtils.fname_from_string(rowName))
	end)
	if type(count) ~= "number" then
		return nil
	end
	-- Guard against bad FName marshalling returning garbage.
	if count < 0 or count > 100000 then
		return nil
	end
	return math.floor(count)
end

local function removeInventoryAmmo(weapon, amount)
	amount = math.floor(tonumber(amount) or 0)
	if amount <= 0 then
		return 0
	end
	local items = getPistolItemsComponent()
	if items == nil or items.RemoveItem == nil then
		return 0
	end
	local rowName = getWeaponAmmoRowName(weapon)
	local before = getInventoryAmmoCount(weapon)
	local ok = pcall(function()
		items:RemoveItem(uevrUtils.fname_from_string(rowName), amount, false)
	end)
	if not ok then
		return 0
	end
	local after = getInventoryAmmoCount(weapon)
	if before ~= nil and after ~= nil then
		local removed = before - after
		if removed < 0 then
			removed = 0
		end
		return removed
	end
	-- Couldn't verify count; assume RemoveItem took what we asked.
	return amount
end

-- Fill clip by writing weapon+0xBB4 and consuming inventory HandgunAmmo.
local function forceSilentPistolClipFill(weapon, quiet)
	abortPistolBodyReloadAnim()
	if weapon == nil then
		return false
	end

	local clipSize = 10
	pcall(function()
		if weapon.WeaponClipSize ~= nil then
			clipSize = tonumber(weapon.WeaponClipSize) or clipSize
		end
	end)
	if clipSize < 1 then
		clipSize = 10
	end

	local before = uobjectReadI32(weapon, PISTOL_CLIP_AMMO_OFFSET) or 0
	if before < 0 then
		before = 0
	end
	if before > clipSize then
		before = clipSize
	end

	local needed = clipSize - before
	if needed <= 0 then
		setPistolOutOfAmmoVisual(weapon, false)
		if not quiet then
			print("[pistol_reload] clip already full")
		end
		return true
	end

	local inv = getInventoryAmmoCount(weapon)
	local canTake = needed
	if inv ~= nil then
		if inv <= 0 then
			if not quiet then
				print("[pistol_reload] no HandgunAmmo in inventory")
			end
			return false
		end
		canTake = math.min(needed, inv)
	else
		-- GetItemCount failed; still try RemoveItem if weapon says inventory has ammo.
		local hasInv = true
		pcall(function()
			if weapon.HasWeaponAmmoInInventory ~= nil then
				hasInv = weapon:HasWeaponAmmoInInventory() == true
			end
		end)
		if not hasInv then
			if not quiet then
				print("[pistol_reload] inventory reports no ammo")
			end
			return false
		end
	end

	local newCount = before + canTake
	local wrote = uobjectWriteI32(weapon, PISTOL_CLIP_AMMO_OFFSET, newCount)
	setPistolOutOfAmmoVisual(weapon, false)
	abortPistolBodyReloadAnim()

	local okAmmo, hasAmmo = pcall(function()
		return weapon:HasAnyAmmoInClip()
	end)
	if not (wrote and okAmmo and hasAmmo) then
		-- Roll back clip write if fill didn't take.
		uobjectWriteI32(weapon, PISTOL_CLIP_AMMO_OFFSET, before)
		if not quiet then
			print("[pistol_reload] clip fill write failed")
		end
		return false
	end

	local removed = removeInventoryAmmo(weapon, canTake)
	local finalCount = newCount
	if removed > 0 and removed < canTake then
		-- Inventory had less than expected; shrink clip to match what we actually took.
		finalCount = before + removed
		uobjectWriteI32(weapon, PISTOL_CLIP_AMMO_OFFSET, finalCount)
	elseif removed <= 0 and inv ~= nil then
		-- RemoveItem failed after we already filled — pull clip back to avoid free ammo.
		uobjectWriteI32(weapon, PISTOL_CLIP_AMMO_OFFSET, before)
		setPistolOutOfAmmoVisual(weapon, before <= 0)
		if not quiet then
			print("[pistol_reload] RemoveItem failed — clip rolled back")
		end
		return false
	elseif removed <= 0 and inv == nil then
		-- Couldn't read inventory count; RemoveItem may still have worked. Keep clip fill.
		if not quiet then
			print(string.format("[pistol_reload] clip fill %d->%d (inventory remove unverified)", before, newCount))
		end
		return true
	end

	if not quiet then
		print(string.format(
			"[pistol_reload] clip fill %d->%d, consumed %d %s",
			before,
			finalCount,
			(removed > 0) and removed or canTake,
			getWeaponAmmoRowName(weapon)
		))
	end
	return true
end

local function pistolReloadMontagePre(fn, obj, locals, result)
	if not isPistolRangedReloadPlayer(obj) then
		return true
	end
	-- Shared RangedReload player: block for manual pistol OR shotgun.
	if configui.getValue("pistol_manual_reload") ~= false then
		local okPistol = isPistolEquipped(getLocalPawn())
		if okPistol then
			return false
		end
	end
	if configui.getValue("shotgun_manual_reload") ~= false
		and shotgunReload ~= nil
		and shotgunReload.shouldBlockNativeReload ~= nil then
		local okSg, blockSg = pcall(function()
			return shotgunReload.shouldBlockNativeReload()
		end)
		if okSg and blockSg then
			return false
		end
	end
	if configui.getValue("rifle_manual_reload") ~= false
		and rifleReload ~= nil
		and rifleReload.shouldBlockNativeReload ~= nil then
		local okRf, blockRf = pcall(function()
			return rifleReload.shouldBlockNativeReload()
		end)
		if okRf and blockRf then
			return false
		end
	end
	return true
end

local function ensurePistolReloadPlayHooks()
	if pistolReload.playHooked then
		return
	end
	local ok1 = hook_function(
		"Class /Script/SHProto.SHAnimMontagePlayer",
		"PlayRequest",
		true,
		pistolReloadMontagePre,
		nil,
		false
	)
	local ok2 = hook_function(
		"Class /Script/SHProto.SHAnimMontagePlayer",
		"PlayOrOverwriteRequest",
		true,
		pistolReloadMontagePre,
		nil,
		false
	)
	local ok3 = hook_function(
		"Class /Script/SHProto.SHAnimMontagePlayer",
		"OverwriteRequest",
		true,
		pistolReloadMontagePre,
		nil,
		false
	)
	pistolReload.playHooked = (ok1 == true) or (ok2 == true) or (ok3 == true)
	if pistolReload.playHooked then
		print("[pistol_reload] RangedReload PlayRequest fully blocked (silent fill)")
	else
		print("[pistol_reload] WARNING: failed to hook RangedReload PlayRequest")
	end
end

local function finishPistolNativeFill(weapon)
	pistolReload.nativeGrace = 0.0
	pistolReload.allowNative = false
	pistolReload.userInitiated = false
	pistolReload.pulseFrames = 0
	pistolReload.aimPulseFrames = 0
	setPistolOutOfAmmoVisual(weapon, false)
	abortPistolBodyReloadAnim()
	setPistolReloadStatus("idle")
end

local function isPistolRightThumbrestTouched()
	local ok, touched = pcall(function()
		return uevrUtils.isThumbpadTouched(nil, Handed.Right)
	end)
	return ok and touched == true
end

local function capturePistolSlideBasis(mesh)
	local slide = getPistolSlideWorldLoc(mesh)
	if slide == nil then
		return false
	end
	local ax, ay, az = 1.0, 0.0, 0.0
	local muzzle = tryPistolSocketLocation(mesh, "FX_muzzle")
	if muzzle ~= nil then
		ax = muzzle.X - slide.X
		ay = muzzle.Y - slide.Y
		az = muzzle.Z - slide.Z
	else
		local ok, rot = pcall(function()
			return mesh:K2_GetComponentRotation()
		end)
		if ok and rot ~= nil then
			local yaw = math.rad(rot.Yaw or 0)
			ax, ay, az = math.cos(yaw), math.sin(yaw), 0.0
		end
	end
	local len = math.sqrt(ax * ax + ay * ay + az * az)
	if len < 0.001 then
		ax, ay, az = 1.0, 0.0, 0.0
		len = 1.0
	end
	ax, ay, az = ax / len, ay / len, az / len
	pistolReload.slideAxisWs = { X = ax, Y = ay, Z = az }
	pistolReload.slideOpenWs = { X = slide.X, Y = slide.Y, Z = slide.Z }
	pistolReload.slideProgress = 0.0
	return true
end

local function projectHandOnPistolSlide(leftLoc)
	if leftLoc == nil or pistolReload.slideOpenWs == nil or pistolReload.slideAxisWs == nil then
		return 0.0
	end
	local o = pistolReload.slideOpenWs
	local a = pistolReload.slideAxisWs
	return (leftLoc.X - o.X) * a.X + (leftLoc.Y - o.Y) * a.Y + (leftLoc.Z - o.Z) * a.Z
end

local function suppressPistolAutoReload(weapon, delta)
	abortPistolBodyReloadAnim()
	if pistolReload.nativeGrace > 0.0 then
		pistolReload.nativeGrace = pistolReload.nativeGrace - (delta or 0.016)
		if forceSilentPistolClipFill(weapon, true) then
			pistolReload.slideClosedByRack = false
			finishPistolNativeFill(weapon)
		elseif pistolReload.nativeGrace <= 0.0 then
			pistolReload.nativeGrace = 0.0
			-- Keep slide closed even if fill is still pending.
			local anim = getPistolAnim(weapon)
			if anim ~= nil and anim.SetOutOfAmmo ~= nil then
				pcall(function()
					anim:SetOutOfAmmo(false)
				end)
			end
		end
	end
	if weapon ~= nil and weapon.HasAnyAmmoInClip ~= nil then
		local okAmmo, hasAmmo = pcall(function()
			return weapon:HasAnyAmmoInClip()
		end)
		if okAmmo and hasAmmo == false
			and not pistolReload.magEjected
			and pistolReload.handMag == nil
			and not pistolReload.awaitingRack
			and pistolReload.nativeGrace <= 0.0
			and not pistolReload.slideClosedByRack then
			setPistolOutOfAmmoVisual(weapon, true)
		elseif okAmmo and hasAmmo then
			pistolReload.slideClosedByRack = false
		end
	end
end

local function reseatPistolMag(mesh)
	destroyPistolHandMag()
	destroyPistolEjectMag()
	showPistolMagBones(mesh)
	pistolReload.magEjected = false
	pistolReload.awaitingRack = true
	pistolReload.magPullLocked = true
	pistolReload.slideGripping = false
	pistolReload.slideProgress = 0.0
	pistolReload.timer = PISTOL_RELOAD_TIMEOUT
	pistolReload.active = true
	pistolReload.slideOpenWs = nil
	pistolReload.slideAxisWs = nil
	capturePistolSlideBasis(mesh)
	pistolHaptic(0, 0.12, 0.55, 0.7)
	pistolHaptic(1, 0.06, 0.4, 0.4)
	do
		local okW, w = isPistolEquipped(getLocalPawn())
		if okW then
			playPistolAk(w, "mag_in")
		end
	end
	-- Slide stays visually open until rack completes; don't let empty-logic reopen mid-rack.
	setPistolReloadStatus("mag seated — grip slide to stick, then pull back")
	print("[pistol_reload] mag reseated — await slide rack")
end

local function commitPistolReload(inputState, weapon, mesh)
	detachPistolHandFromSlide()
	pistolHaptic(0, 0.22, 0.85, 1.0)
	pistolHaptic(1, 0.1, 0.5, 0.5)
	playPistolAk(weapon, "rack")
	destroyPistolHandMag()
	destroyPistolEjectMag()
	showPistolMagBones(mesh)
	pistolReload.awaitingRack = false
	pistolReload.magPullLocked = false
	pistolReload.slideGripping = false
	pistolReload.magEjected = false
	pistolReload.slideProgress = 0.0
	pistolReload.slideOpenWs = nil
	pistolReload.slideAxisWs = nil
	pistolReload.active = false
	pistolReload.timer = 0.0
	pistolReload.pulseFrames = 0
	pistolReload.aimPulseFrames = 0
	pistolReload.allowNative = false
	pistolReload.userInitiated = false
	pistolReload.slideClosedByRack = true
	if inputState ~= nil then
		uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_X)
		uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
		inputState.Gamepad.bLeftTrigger = 0
	end
	-- Close slide visual first and keep it closed (slideClosedByRack).
	local anim = getPistolAnim(weapon)
	if anim ~= nil and anim.SetOutOfAmmo ~= nil then
		pcall(function()
			anim:SetOutOfAmmo(false)
		end)
	end
	pistolReload.holdingEmptyVisual = false

	local filled = forceSilentPistolClipFill(weapon)
	if filled then
		pistolReload.slideClosedByRack = false
		finishPistolNativeFill(weapon)
		setPistolReloadStatus("idle")
	else
		setPistolReloadStatus("slide closed — chambering…")
		pistolReload.nativeGrace = 0.5
	end
	print("[pistol_reload] slide closed — silent fill (no body anim)")
end

local function handlePistolManualReload(inputState, delta)
	ensurePistolReloadPlayHooks()
	if configui.getValue("pistol_manual_reload") == false then
		if pistolReload.active or pistolReload.handMag ~= nil or pistolReload.magEjected or pistolReload.awaitingRack then
			resetPistolManualReload("disabled")
		end
		return false
	end

	local currentPawn = getLocalPawn()
	if currentPawn == nil or uevrUtils.isInCutscene() then
		return false
	end

	local isPistol, weapon = isPistolEquipped(currentPawn)
	if not isPistol then
		pistolReload.clipEmpty = false
		pistolReload.wasRightTrigger = false
		if pistolReload.active or pistolReload.handMag ~= nil then
			resetPistolManualReload("idle")
		end
		return false
	end

	-- Track empty clip every frame. Empty + aim/fire is what starts the body reload anim.
	local wasClipEmpty = pistolReload.clipEmpty == true
	pistolReload.clipEmpty = false
	if weapon.HasAnyAmmoInClip ~= nil then
		local okAmmo, hasAmmo = pcall(function()
			return weapon:HasAnyAmmoInClip()
		end)
		pistolReload.clipEmpty = okAmmo and hasAmmo == false
	end
	-- Last round spent → slide opens: play empty/slide-lock SFX once.
	if pistolReload.clipEmpty and not wasClipEmpty
		and not pistolReload.magEjected
		and pistolReload.handMag == nil
		and not pistolReload.awaitingRack then
		playPistolSlideOpenSfx(weapon)
	end

	-- While empty: eat aim + fire (no body reload). Keep slide open ONLY when not racking.
	local rtDown = (inputState.Gamepad.bRightTrigger or 0) > 128
	if pistolReload.clipEmpty and not shouldAllowPistolNativeReload() then
		if rtDown and not pistolReload.wasRightTrigger then
			playPistolEmptyTriggerClick(weapon)
		end
		inputState.Gamepad.bLeftTrigger = 0
		inputState.Gamepad.bRightTrigger = 0
		if not pistolReload.slideClosedByRack
			and not pistolReload.awaitingRack
			and not pistolReload.slideGripping
			and not pistolReload.magPullLocked then
			setPistolOutOfAmmoVisual(weapon, true)
		end
	end
	pistolReload.wasRightTrigger = rtDown

	suppressPistolAutoReload(weapon, delta)

	local magDistLimit = configui.getValue("pistol_reload_mag_dist") or PISTOL_RELOAD_MAG_DIST
	local hipDistLimit = configui.getValue("pistol_reload_hip_dist") or PISTOL_HOLSTER_DIST
	local slideGrabLimit = configui.getValue("pistol_reload_slide_dist") or PISTOL_RELOAD_SLIDE_DIST
	local slideTravel = configui.getValue("pistol_reload_slide_travel") or PISTOL_RELOAD_SLIDE_TRAVEL
	local hipReleaseLimit = hipDistLimit + 10.0

	local xDown = uevrUtils.isButtonPressed(inputState, XINPUT_GAMEPAD_X)
	-- Left grip only (LB). Do not treat LT as grip — aim remaps fight that in VR.
	local leftGripDown = uevrUtils.isButtonPressed(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)

	if pistolReload.pulseFrames > 0 then
		pistolReload.allowNative = true
		pistolReload.userInitiated = true
		uevrUtils.pressButton(inputState, XINPUT_GAMEPAD_X)
		pistolReload.pulseFrames = pistolReload.pulseFrames - 1
		xDown = true
		if pistolReload.pulseFrames <= 0 then
			-- Keep allowNative via nativeGrace until clip actually fills.
			setPistolReloadStatus("filling clip…")
		end
	end

	local mesh = getPistolWeaponMesh(weapon)
	local leftLoc = controllers.getControllerLocation(0)
	local magLoc = getPistolMagWorldLoc(mesh)
	local slideLoc = getPistolSlideWorldLoc(mesh)
	local holsterLoc = getPistolHolsterWorldPos()
	local magDist = pistolVecDist(leftLoc, magLoc)
	local slideDist = pistolVecDist(leftLoc, slideLoc)
	local hipDist = pistolVecDist(leftLoc, holsterLoc)
	local now = os.clock()
	local hipThr = pistolReload.inHolsterZone and hipReleaseLimit or hipDistLimit
	local inHip = hipDist <= hipThr
	if now < (pistolReload.holsterCooldownUntil or 0) then
		inHip = false
	end
	pistolReload.inHolsterZone = inHip

	-- Short tap X = drop mag to hip (native reload always blocked).
	if pistolReload.pulseFrames <= 0 then
		if xDown then
			uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_X)
			if not pistolReload.wasXDown then
				pistolReload.xPressStarted = now
			end
		else
			if pistolReload.wasXDown then
				local held = now - (pistolReload.xPressStarted or now)
				if held < 0.45 and pistolReload.handMag == nil and not pistolReload.awaitingRack then
					local canDrop = pistolReload.clipEmpty == true
					if weapon.CanBeReloaded ~= nil then
						local ok, val = pcall(function()
							return weapon:CanBeReloaded()
						end)
						if ok and val == true then
							canDrop = true
						end
					end
					if not canDrop then
						-- Still allow tactical drop / pull even if clip reports full.
						canDrop = true
					end
					if canDrop then
						dropPistolMagViaX(weapon, mesh)
					end
				end
			end
			pistolReload.xPressStarted = nil
		end
	end
	pistolReload.wasXDown = xDown

	if pistolReload.active or pistolReload.magEjected or pistolReload.awaitingRack or pistolReload.handMag ~= nil or pistolReload.magPullLocked then
		pistolReload.timer = pistolReload.timer - (delta or 0.016)
		if pistolReload.timer <= 0.0 then
			pistolReload.wasLeftGrip = leftGripDown
			resetPistolManualReload("timeout")
			return true
		end
	end

	if pistolReload.handMag ~= nil then
		local holdingEject = isPistolEjectHandMag(pistolReload.handMag)
		-- Live pose from Manual Reload sliders (works for static + poseable).
		applyPistolHandAttachTransform(pistolReload.handMag)
		-- Grip near hip while holding mag → park back at hip (reload.lua release).
		if leftGripDown and inHip and magDist >= magDistLimit then
			if holdingEject then
				local comp = pistolReload.handMag
				pistolReload.handMag = nil
				parkPistolMagAtHolster(comp)
			else
				destroyPistolHandMag()
				pistolReload.magEjected = true
			end
			pistolReload.holsterCooldownUntil = now + PISTOL_HOLSTER_COOLDOWN
			pistolReload.inHolsterZone = false
			setPistolReloadStatus("mag at hip — grip near hip to grab")
			pistolHaptic(0, 0.05, 0.3, 0.4)
			uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
			pistolReload.wasLeftGrip = leftGripDown
			return true
		end
		if not leftGripDown then
			if magDist < magDistLimit then
				destroyPistolHandMag()
				destroyPistolEjectMag()
				reseatPistolMag(mesh)
			elseif holdingEject then
				local comp = pistolReload.handMag
				pistolReload.handMag = nil
				parkPistolMagAtHolster(comp)
				pistolReload.holsterCooldownUntil = now + PISTOL_HOLSTER_COOLDOWN
				setPistolReloadStatus(string.format("mag at hip — grip hip (%.1f) or well (%.1f)", hipDist, magDist))
				pistolHaptic(0, 0.05, 0.3, 0.25)
			else
				destroyPistolHandMag()
				pistolReload.magEjected = true
				pistolReload.holsterCooldownUntil = now + PISTOL_HOLSTER_COOLDOWN
				setPistolReloadStatus(string.format("mag at hip — grip hip (%.1f) or well (%.1f)", hipDist, magDist))
				pistolHaptic(0, 0.05, 0.3, 0.25)
			end
			pistolReload.wasLeftGrip = leftGripDown
			return true
		end
		uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
		if magDist < magDistLimit then
			setPistolReloadStatus(string.format("RETURN OK (%.1f) — release", magDist))
		elseif inHip then
			setPistolReloadStatus(string.format("HIP OK (%.1f) — release to park", hipDist))
		else
			setPistolReloadStatus(string.format("dock well (%.1f) or hip (%.1f)", magDist, hipDist))
		end
		pistolReload.wasLeftGrip = leftGripDown
		return true
	end

	if pistolReload.magEjected then
		local ejectPhase = pistolReload.eject.phase
		if ejectPhase == "anim" or ejectPhase == "slide" or ejectPhase == "fall" then
			setPistolReloadStatus("mag ejecting…")
			pistolReload.wasLeftGrip = leftGripDown
			return true
		end
		-- Direct dock at well without picking up (fast path).
		if leftGripDown and magDist < magDistLimit then
			destroyPistolEjectMag()
			reseatPistolMag(mesh)
			uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
			pistolReload.wasLeftGrip = leftGripDown
			return true
		end
		-- Grip near hip → pick up dropped mag into left hand.
		if leftGripDown and inHip then
			if grabPistolMagFromHip(mesh) then
				uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
				pistolReload.wasLeftGrip = leftGripDown
				return true
			end
		end
		if inHip then
			uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
			setPistolReloadStatus(string.format("HIP OK (%.1f) — grip to grab mag", hipDist))
		else
			setPistolReloadStatus(string.format("mag at hip (%.1f/%.1f) — or dock well (%.1f)", hipDist, hipDistLimit, magDist))
		end
		pistolReload.wasLeftGrip = leftGripDown
		return true
	end

	if pistolReload.awaitingRack or pistolReload.magPullLocked then
		pistolReload.awaitingRack = true
		if pistolReload.slideOpenWs == nil then
			capturePistolSlideBasis(mesh)
		end

		-- Optional: one right-thumbrest press = same as completing pull-back rack
		local thumbrestDown = isPistolRightThumbrestTouched()
		if configui.getValue("pistol_rack_thumbrest") ~= false
			and thumbrestDown
			and not pistolReload.wasRightThumbrest then
			pistolReload.wasRightThumbrest = thumbrestDown
			commitPistolReload(inputState, weapon, mesh)
			setPistolReloadStatus("racked (thumbrest)")
			print("[pistol_reload] rack committed via right thumbrest")
			return true
		end
		pistolReload.wasRightThumbrest = thumbrestDown

		local nearSlide = leftGripDown and leftLoc ~= nil and slideDist < slideGrabLimit
		if nearSlide or pistolReload.slideGripping then
			if leftGripDown and leftLoc ~= nil then
				if not pistolReload.slideGripping then
					if pistolReload.slideOpenWs == nil then
						capturePistolSlideBasis(mesh)
					end
					pistolReload.slideGripping = true
					pistolReload.slideGrabAlong = projectHandOnPistolSlide(leftLoc)
					pistolReload.slideProgressAtGrab = pistolReload.slideProgress or 0.0
					-- Stick left hand to slide_joint (same accessories path as shotgun pump).
					if attachPistolHandToSlide(mesh) then
						print("[pistol_reload] left hand stuck to slide")
					end
					pistolHaptic(0, 0.08, 0.5, 0.55)
				elseif not pistolReload.handStuckToSlide then
					-- Re-stick if something detached us mid-pull.
					attachPistolHandToSlide(mesh)
				else
					refreshPistolSlideHandPose(mesh)
				end
				local along = projectHandOnPistolSlide(leftLoc)
				-- Axis is slide→muzzle (forward). Rack is a rearward pull: invert delta.
				local deltaAlong = (pistolReload.slideGrabAlong or 0.0) - along
				local progress = (pistolReload.slideProgressAtGrab or 0.0) + (deltaAlong / slideTravel)
				if progress < 0.0 then progress = 0.0 end
				if progress > 1.0 then progress = 1.0 end
				pistolReload.slideProgress = progress
				uevrUtils.unpressButton(inputState, XINPUT_GAMEPAD_LEFT_SHOULDER)
				setPistolReloadStatus(string.format("racking slide %.0f%% (hand stuck)", progress * 100.0))
				if pistolReload.slideProgress >= 0.5 then
					-- Close slide visual as the rack progresses (not only on commit).
					local anim = getPistolAnim(weapon)
					if anim ~= nil and anim.SetOutOfAmmo ~= nil then
						pcall(function()
							anim:SetOutOfAmmo(false)
						end)
					end
				end
				if pistolReload.slideProgress >= 0.95 then
					commitPistolReload(inputState, weapon, mesh)
				end
			else
				pistolReload.slideGripping = false
				detachPistolHandFromSlide()
				if configui.getValue("pistol_rack_thumbrest") ~= false then
					setPistolReloadStatus("grip slide to stick — or right thumbrest")
				else
					setPistolReloadStatus("grip slide to stick, then pull back")
				end
			end
			pistolReload.wasLeftGrip = leftGripDown
			return true
		end
		if configui.getValue("pistol_rack_thumbrest") ~= false then
			setPistolReloadStatus(string.format("grip slide (%.1f) to stick — or thumbrest", slideDist))
		else
			setPistolReloadStatus(string.format("grip slide (%.1f/%.1f) to stick + pull", slideDist, slideGrabLimit))
		end
		pistolReload.wasLeftGrip = leftGripDown
		return true
	end

	pistolReload.wasRightThumbrest = isPistolRightThumbrestTouched()
	pistolReload.wasLeftGrip = leftGripDown
	return pistolReload.active
end

setPistolReloadStatus("idle")
ensurePistolReloadPlayHooks()
pcall(function()
	uevrUtils.set_cvar_int("DisableAutoUnequipWeapon", 1)
end)
print("[pistol_reload] initialized (X eject + hip grab, clip fill @weapon+0xBB4, body RangedReload blocked)")

-- Shotgun pump + reload + rifle bolt/reload. Separate modules for locals limit.
shotgunPump.init()
shotgunReload.init()
rifleBolt.init()
rifleReload.init()

uevrUtils.registerLevelChangeCallback(function()
	resetPistolManualReload("level change")
end)

-- Priority 10 > main remaps (5): pistol reload reads raw left grip / pulses X before sprint remap eats LB.
uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, state)
	pcall(function()
		handlePistolManualReload(state, 0.016)
	end)
end, 10)

-- After aim remap (prio 5): force LT so SH2 starts the one allowed empty-aim refill after rack.
uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, state)
	pcall(function()
		if pistolReload.aimPulseFrames <= 0 then
			return
		end
		if configui.getValue("pistol_manual_reload") == false then
			pistolReload.aimPulseFrames = 0
			return
		end
		state.Gamepad.bLeftTrigger = 255
		pistolReload.aimPulseFrames = pistolReload.aimPulseFrames - 1
		if pistolReload.pulseFrames > 0 then
			uevrUtils.pressButton(state, XINPUT_GAMEPAD_X)
		end
	end)
end, 4)

uevrUtils.registerOnPreInputGetStateCallback(function(retval, user_index, state)
    if not ui.isRemapDisabled() then
        if state.Gamepad.sThumbRY < -20000 then
            uevrUtils.pressButton(state, XINPUT_GAMEPAD_B)
        end
        state.Gamepad.sThumbRY = 0 --disable pitching the orbiting camera

	    handleSnapTurnInput(state)
		handleSmoothTurnInput(state)

		if configui.getValue("interaction_control_mode") == 2 then
			local handed = getHandedness()
			local isEating, isGrabbingGlasses, gripHead, isGrabbingEar, triggerMouth, isScratchingEyes, triggerHead, isScratchingEar = gestures.getHeadGestures(state, 1-handed, false)
			local isEatingRight, isGrabbingGlassesRight, gripHeadRight, isGrabbingEarRight, triggerMouthRight, isScratchingEyesRight, triggerHeadRight, isScratchingEarRight = gestures.getHeadGestures(state, handed, false)

			local sprintMode = configui.getValue("interaction_sprint_mode")
			local wasTappedForward, wasTappedBackward = getLeftStickDoubleTapDirection(state)
			local leftGripPressed = uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_LEFT_SHOULDER)
			local rightGripPressed = uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_RIGHT_SHOULDER)
			local leftStickPressed = uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_LEFT_THUMB)
			local rightStickPressed = uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_RIGHT_THUMB)
			local leftTriggerPressed = state.Gamepad.bLeftTrigger >= 128
			local blockShotgunReloadGrip = shotgunReload ~= nil
				and shotgunReload.isHoldingShell ~= nil
				and shotgunReload.isHoldingShell()
			local blockRifleReloadGrip = rifleReload ~= nil
				and rifleReload.isHoldingShell ~= nil
				and rifleReload.isHoldingShell()
			if (sprintMode == 3 and wasTappedForward) or ((sprintMode == 1 or sprintMode == 2) and leftStickPressed) then --leftStickPressed then
				-- Don't turn stick-sprint into LB while hand is using hip/mag/slide reload.
				if pistolReload.inHolsterZone
					or pistolReload.handMag ~= nil
					or pistolReload.slideGripping
					or pistolReload.awaitingRack
					or pistolReload.magEjected
					or blockShotgunReloadGrip
					or blockRifleReloadGrip then
					uevrUtils.unpressButton(state, XINPUT_GAMEPAD_LEFT_SHOULDER)
				else
					uevrUtils.pressButton(state, XINPUT_GAMEPAD_LEFT_SHOULDER)
				end
			else
				uevrUtils.unpressButton(state, XINPUT_GAMEPAD_LEFT_SHOULDER)
			end

			if wasTappedBackward then
				uevrUtils.pressButton(state, XINPUT_GAMEPAD_RIGHT_SHOULDER)
			else
				uevrUtils.unpressButton(state, XINPUT_GAMEPAD_RIGHT_SHOULDER)
    		end

			--flashlight
			if gripHead and status["isGrippingLight"] ~= true then
				configui.setValue("light_type", configui.getValue("light_type") == 1 and 2 or 1)
				status["isGrippingLight"] = true
			elseif gripHeadRight and status["isGrippingLight"] ~= true then
				configui.setValue("light_type", configui.getValue("light_type") == 1 and 3 or 1)
				status["isGrippingLight"] = true
			else
				if leftTriggerPressed then
					uevrUtils.pressButton(state, XINPUT_GAMEPAD_RIGHT_THUMB)
				else
					uevrUtils.unpressButton(state, XINPUT_GAMEPAD_RIGHT_THUMB)
				end
			end
			--prevent flapping between light states by only allowing grip to trigger light state change once every 300ms, the deferral will reset the "isGrippingLight" status after 300ms allowing it to be triggered again
			if gripHead or gripHeadRight then
				uevrUtils.updateDeferral("is_gripping_light")
			end
			-- end flashlight

			-- SH2 gates fire behind aim/LT. Auto-aim only while RT/grip — but NEVER
			-- when the pistol clip is empty (empty+aim starts the body reload anim),
			-- and NEVER while shotgun manual pump is blocking (right grip was re-showing aim).
			local rightTriggerPressed = (state.Gamepad.bRightTrigger or 0) > 128
			local blockEmptyAim = configui.getValue("pistol_manual_reload") ~= false
				and pistolReload.clipEmpty
				and not shouldAllowPistolNativeReload()
			local blockShotgunPump = shotgunPump ~= nil
				and shotgunPump.isBlockingFire ~= nil
				and shotgunPump.isBlockingFire()
			local blockRifleBolt = rifleBolt ~= nil
				and rifleBolt.isBlockingFire ~= nil
				and rifleBolt.isBlockingFire()
			-- Empty tube/mag aims start native reload — but NEVER while pump/bolt owns input.
			local blockShotgunEmpty = (not blockShotgunPump)
				and configui.getValue("shotgun_manual_reload") ~= false
				and shotgunReload ~= nil
				and shotgunReload.isClipEmpty ~= nil
				and shotgunReload.isClipEmpty()
			local blockRifleEmpty = (not blockRifleBolt)
				and configui.getValue("rifle_manual_reload") ~= false
				and rifleReload ~= nil
				and rifleReload.isClipEmpty ~= nil
				and rifleReload.isClipEmpty()
			-- sh2vr (2026-10-24, Tefa): with "always_aim" on, the aim input is held the whole time a gun is out, so no
			-- grip press is ever needed (the Visceral RE2 way); every block above still wins.
			local gunOut = uevrUtils.getValid(status.currentEquippedWeapon) ~= nil and status.currentEquippedWeapon.RegisteredFirePoint ~= nil
			-- Running (Tefa 2026-10-24): the game will not sprint while aiming, so the aim lets go while the sprint
			-- button is held (the remap above turned the stick press into LB), for a few frames after it (toggled
			-- sprint needs a moment to start) and for as long as the game says it is sprinting. The trigger still
			-- forces the aim, which ends the sprint: Tefa accepted dropping back to a walk on a shot.
			local sprintRequested = uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_LEFT_SHOULDER)
			if sprintRequested then status.sprintGraceFrames = 20 end
			if (status.sprintGraceFrames or 0) > 0 then status.sprintGraceFrames = status.sprintGraceFrames - 1 end
			local sprinting = false
			local movement = uevrUtils.getValid(pawn, {"Movement"})
			if movement ~= nil then
				pcall(function() sprinting = movement:IsSprinting() == true end)
			end
			local alwaysAim = configui.getValue("always_aim") ~= false and gunOut and not (sprintRequested or sprinting or (status.sprintGraceFrames or 0) > 0)
			if blockEmptyAim or blockShotgunPump or blockShotgunEmpty or blockRifleBolt or blockRifleEmpty then
				state.Gamepad.bLeftTrigger = 0
				state.Gamepad.bRightTrigger = 0
			elseif rightTriggerPressed or rightGripPressed or alwaysAim then
				local holdingShell = (shotgunReload ~= nil
						and shotgunReload.isHoldingShell ~= nil
						and shotgunReload.isHoldingShell())
					or (rifleReload ~= nil
						and rifleReload.isHoldingShell ~= nil
						and rifleReload.isHoldingShell())
				if holdingShell then
					state.Gamepad.bLeftTrigger = rightTriggerPressed and 255 or 0
				else
					state.Gamepad.bLeftTrigger = 255
				end
			else
				state.Gamepad.bLeftTrigger = 0
			end
			--end aim

			if uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_RIGHT_SHOULDER) then
				status.turnStep = (status.turnStep or 0) + 180
			end

		elseif configui.getValue("interaction_control_mode") == 1 then
			--prevent continuous 180 turn in vanilla mode
			if uevrUtils.isButtonPressed(state, XINPUT_GAMEPAD_RIGHT_SHOULDER) then
				if status.lock180Turn ~= true then
					status.lock180Turn = true
					status.turnStep = (status.turnStep or 0) + 180
				end
			else
				status.lock180Turn = false
			end

		end

	end
end, 5)

configui.onCreateOrUpdate("use_snap_turn", function(enabled)
	snapTurnState.rxState = 0
	status.turnAxisX = 0
	status.lastViewRotationYaw = nil
	if enabled == true then
		uevrUtils.enableSnapTurn(false)
	end
	configui.setHidden("snap_turn_angle", enabled ~= true)
end)

--The normal handler for this is being overridden somehow
ui.registerWidgetChangeCallback("GameplayEndGame_WidgetBP_C", function(active)
	if active then
		delay(200, function()
			uevrUtils.fadeCamera(0.1, true)
		end)
	else
		uevrUtils.stopFadeCamera()
	end
end)

uevr.sdk.callbacks.on_pre_engine_tick(function(engine, delta)
	-- Mag eject clone: CopyPose / fall / holster sync (needs engine tick, not only input)
	pcall(function()
		if configui.getValue("pistol_manual_reload") ~= false then
			tickPistolMagEjectAnim()
			-- Keep gun mag bones hidden while eject clone owns the visual
			if pistolReload.magEjected or pistolReload.eject.active then
				local ok, weapon = isPistolEquipped(getLocalPawn())
				if ok then
					hidePistolMagBones(getPistolWeaponMesh(weapon))
				end
			end
		end
	end)

	local currentPawn = getLocalPawn()
	if currentPawn == nil then
		return
	end

	if not ui.isInputDisabled() then
		applyFaceAwayFromCamera(currentPawn)
	end

	local camera_overlap = currentPawn.CameraOverlapHandler
    if camera_overlap ~= nil then
        if camera_overlap:IsComponentTickEnabled() then
            camera_overlap:SetComponentTickEnabled(false) -- Stops camera from making pawn and other things near it invisible
            --print("Disabled camera overlap handler")
        end

        camera_overlap.OwnerCharacterPlay = nil -- Nulls out the owner actor, this stops some logic from functioning when we start aiming which hides the pawn
    end

	-- This is what the game uses to signify a cutscene is in progress
	if status.characterStatics == nil then
		status.characterStatics = uevrUtils.find_default_instance("Class /Script/SHProto.SHCharacterStatics")
	end
	if status.characterStatics ~= nil then
		local isInCutscene = status.characterStatics:IsCharacterInCutscene(currentPawn)
		if status.isInCutscene ~= isInCutscene then
			local skipNotify = status.isInCutscene == nil and isInCutscene == false --dont trigger cutscene state change on first tick if not in cutscene, in case another system thinks we are in a cutscene
			status.isInCutscene = isInCutscene
			if skipNotify == false then
				--print("Cutscene state changed from characterStatics", status.isInCutscene)
				uevrUtils.setIsInCutsceneOverride(status.isInCutscene)
				if status.isInCutscene == false then
					status.currentGameplayInputMode = 0
					if not ui.isFadeCameraEnabled() then
						uevrUtils.stopFadeCamera()
					end
				end
			end
		end
	else
		print("Could not find character statics for cutscene detection")
	end

	if currentPawn.GetGameplayInputMode ~= nil then
		local currentGameplayInputMode = currentPawn:GetGameplayInputMode()
		if status.currentGameplayInputMode ~= currentGameplayInputMode then
			status.currentGameplayInputMode = currentGameplayInputMode
			if uevrUtils.getValid(currentPawn, {"Items", "ItemExecutive"}) == nil and status.isPushing ~= true and status.isTraversing ~= true then
				--print("Cutscene state changed from GetGameplayInputMode", currentGameplayInputMode == 1, currentGameplayInputMode)
				uevrUtils.setIsInCutsceneOverride(currentGameplayInputMode == 1)
			end
		end
	end

	-- We have to disable roomscale movement during the "Pushing" animations or the game will prevent the task from ever being completed
	if currentPawn.Movement ~= nil then
		if status.isPushing ~= (currentPawn.Movement.PushableComponent ~= nil) then
			status.isPushing = (currentPawn.Movement.PushableComponent ~= nil)
			uevrUtils.setIsInCutsceneOverride(status.isPushing)
			if status.isPushing then
				status.isRoomscale = uevrUtils.getUEVRParam_bool("VR_RoomscaleMovement")
				uevrUtils.setUEVRParam("VR_RoomscaleMovement", "false")
			elseif status.isRoomscale ~= nil then
				uevrUtils.setUEVRParam("VR_RoomscaleMovement", status.isRoomscale == true and "true" or "false")
				status.isRoomscale = nil
			end
		end
	end

	--invincibility
	-- local health = uevrUtils.getValid(currentPawn, {"Health"})
	-- if health ~= nil then
	-- 	health.HealthValue = 100
	-- end

end)

--This gets the source location and rotation for the reticle. The game must internally handle the RegisteredFirePoint yaw offset when firing
--because just using the raw rotation doesn't work for reticule aiming and must be offset manually
reticule.registerCustomTargetCallback(function()
	if uevrUtils.getValid(status.currentEquippedWeapon) ~= nil and status.currentEquippedWeapon.RootComponent ~= nil and status.currentEquippedWeapon.RootComponent.K2_GetComponentLocation ~= nil and status.currentEquippedWeapon.RegisteredFirePoint ~= nil then
		local loc = status.currentEquippedWeapon.RootComponent:K2_GetComponentLocation()
		local rot = status.currentEquippedWeapon.RootComponent:K2_GetComponentRotation()
		local offsetYaw = status.currentEquippedWeapon.RegisteredFirePoint.RelativeRotation
		rot = mathLib.composeRotators(uevrUtils.rotator(offsetYaw.Pitch, offsetYaw.Yaw + 90, offsetYaw.Roll), rot)

		local vector = uevrUtils.rotateVector(uevrUtils.vector(0,0,status.currentReticuleOffset or 0), rot)
		loc = loc + vector

		return loc, rot
	end
	return uevrUtils.vector(0,0,0), uevrUtils.rotator(0,0,0)
end)

uevr.sdk.callbacks.on_post_engine_tick(function(engine, delta)
	local currentPawn = getLocalPawn()
	if currentPawn == nil then
		return
	end

	-- The game tries to change the relative offset of attachments for a short period during activation so this overrides that
	if status.updateAttachmentTransform == true then
		resetAttachment()
	end

	--This is what aims the ranged weapons
	if uevrUtils.getValid(status.currentEquippedWeapon) ~= nil and status.currentEquippedWeapon.RootComponent ~= nil and status.currentEquippedWeapon.RootComponent.K2_SetWorldLocation ~= nil then
		local location, rotation = attachments.getActiveAttachmentTransforms(getHandedness())
		status.currentEquippedWeapon.RootComponent:K2_SetWorldLocation(location, false, reusable_hit_result, false)
		status.currentEquippedWeapon.RootComponent:K2_SetWorldRotation(rotation, false, reusable_hit_result, false)
	end

    if currentPawn.View ~= nil and not ui.isViewLocked() then
		uevr.params.vr.get_pose(uevr.params.vr.get_hmd_index(), temp_vec3f, temp_quatf)
		local poseQuat = uevrUtils.quat(temp_quatf.Z, temp_quatf.X, -temp_quatf.Y, -temp_quatf.W)  --reordered terms to convert UEVR to unreal coord system
		local hmdRotation = kismet_math_library:Quat_Rotator(poseQuat)

		status.currentHMDYaw = hmdRotation.Yaw
		uevr.params.vr.recenter_view()

		if configui.getValue("movement_type") == 1 then
			status.currentHMDYaw = 0
		end

		local turnDelta = 0
		if configui.getValue("use_snap_turn") then
			turnDelta = (status.turnStep or 0)
		else
			if status.turnStep == nil then
				status.turnStep = 0
			end
			status.turnStep = status.turnStep + ((status.turnAxisX or 0) * smoothTurnState.degreesPerSecond * (delta or 0))
			turnDelta = status.turnStep or 0
		end

		local deltaRotation = turnDelta - (status.currentHMDYaw or 0)
		currentPawn.View:OverrideControlRotation(
			uevrUtils.rotator(hmdRotation.Pitch, deltaRotation, hmdRotation.Roll), -- Pitch required for item indicators
			currentPawn
		)
	end

end)

-- SH2 doors are angle-driven via USHDoorMovementComponent:AddPhysicForce, not Chaos impulses.
local DOOR_PHYSIC_FORCE_SCALE = 0.1

local function getDoorMovement(actor)
	if actor == nil then return nil end
	local candidates = { actor.DoorMovement, actor.LeftDoorMovement, actor.RightDoorMovement }
	for i = 1, #candidates do
		local dm = candidates[i]
		if dm ~= nil and dm.AddPhysicForce ~= nil then
			return dm
		end
	end
	return nil
end

collision.registerContactCallback(function(id, handVelocity, hitResult, collider, handed)
	if handVelocity == nil or hitResult == nil then return end
	local hit = uevrUtils.getCleanHitResult(hitResult)
	local hitComp = hit and uevrUtils.getValid(hit.Component) or nil
	if hit == nil or hitComp == nil or hit.ImpactPoint == nil then return end

	local doorMovement = getDoorMovement(hitComp.GetOwner and hitComp:GetOwner() or nil)
	if doorMovement == nil then return end

	local pivot = hitComp:K2_GetComponentLocation()
	local rx = hit.ImpactPoint.X - pivot.X
	local ry = hit.ImpactPoint.Y - pivot.Y
	local rz = hit.ImpactPoint.Z - pivot.Z
	local up = hitComp.GetUpVector and hitComp:GetUpVector() or nil
	if up == nil then return end

	-- Tangential direction around the hinge (up × radius) so force sign matches swing direction.
	local tx = up.Y * rz - up.Z * ry
	local ty = up.Z * rx - up.X * rz
	local tz = up.X * ry - up.Y * rx
	local tLen = math.sqrt(tx * tx + ty * ty + tz * tz)
	if tLen < 1.0 then return end
	tx, ty, tz = tx / tLen, ty / tLen, tz / tLen

	local moveForce = (handVelocity.X * tx + handVelocity.Y * ty + handVelocity.Z * tz) * DOOR_PHYSIC_FORCE_SCALE
	if math.abs(moveForce) > 0.001 then
		doorMovement:AddPhysicForce(moveForce)
	end
end)

setInterval(200, function()
    local springArm = uevrUtils.getValid(pawn, {"SpringArm"})
    if springArm ~= nil then
        springArm.bEnableCameraLag = configui.getValue("enable_spring_arm_lag")
        springArm.bEnableCameraRotationLag = configui.getValue("enable_spring_arm_rotationlag")
    end

	-- currently vaulting and climbing (not ladder)
	local isTraversing = isPawnTraversing(pawn)
	if status.isTraversing ~= isTraversing then
		local skipNotify = status.isTraversing == nil and isTraversing == false
		status.isTraversing = isTraversing
		if skipNotify == false then
			--print("Cutscene state changed from isTraversing", status.isInCutscene)
			uevrUtils.setIsInCutsceneOverride(status.isTraversing)
			if status.isTraversing == false then status.currentGameplayInputMode = 0 end
		end
	end

	--When map is showing use default button mappings
	local map = uevrUtils.getValid(pawn, {"Items", "ItemExecutive"})
	if map ~= nil and string.sub(map:get_full_name(), 1, 3) == "Map"  then
		ui.setCustomState("remapEnabled", false, 2)
	else
		ui.setCustomState("remapEnabled", nil)
	end

end)


local function getBlendDataOverride()
	local blendData = uevrUtils.get_struct_object("ScriptStruct /Script/SHProto.SHBlendData")
	if blendData == nil then
		return nil
	end

	blendData.BlendInTime = 0.0
	blendData.BlendInAlphaCurve = nil
	blendData.BlendOutTime = 0.0
	blendData.BlendOutAlphaCurve = nil
	return blendData
end


local function buildCenteredCameraData(sourceData)
	if sourceData == nil or sourceData.BlendData == nil then
		return nil
	end

	local blendData = getBlendDataOverride()
	if blendData == nil then
		return nil
	end

	local cameraData = uevrUtils.get_struct_object("ScriptStruct /Script/SHProto.SHCameraDataStruct")
	if cameraData == nil then
		return nil
	end

	cameraData.BlendData = blendData
	cameraData.ArmLengthFromPitchCurve = sourceData.ArmLengthFromPitchCurve
	cameraData.SocketOffsetFromPitchCurve = nil
	cameraData.TargetOffset = uevrUtils.vector(0, -45, sourceData.TargetOffset and sourceData.TargetOffset.Z or 0)
	cameraData.TargetOffsetExtraHeightFromPitchScale = sourceData.TargetOffsetExtraHeightFromPitchScale or 1.0
	cameraData.MovementForwardCameraLag = sourceData.MovementForwardCameraLag or 0.0
	cameraData.MovementNonForwardCameraLag = sourceData.MovementNonForwardCameraLag or 0.0
	cameraData.RotationLag = sourceData.RotationLag or 0.0
	return cameraData
end

local function getActiveCameraBlender(currentPawn)
	if currentPawn == nil or currentPawn.SpringArm == nil or currentPawn.View == nil then
		return nil
	end

	local viewRotation = currentPawn.View:GetViewRotation()
	if viewRotation == nil then
		return nil
	end

	local targetArmLength = currentPawn.SpringArm.TargetArmLength
	if targetArmLength == nil then
		return nil
	end

	local candidates = {
		currentPawn.SpringArm.InteriorCameraBlender,
		currentPawn.SpringArm.ExteriorSprintCameraBlender,
		currentPawn.SpringArm.InteriorSprintCameraBlender,
	}

	local bestBlender = nil
	local bestDiff = nil
	for _, blender in ipairs(candidates) do
		if blender ~= nil and blender.CameraData ~= nil and blender.CameraData.ArmLengthFromPitchCurve ~= nil then
			local curveValue = blender.CameraData.ArmLengthFromPitchCurve:GetFloatValue(viewRotation.Pitch)
			--local curveValue = blender.CameraData.ArmLengthFromPitchCurve:GetFloatValue(fixedOrbitCameraPitch)
			if curveValue ~= nil then
				local diff = math.abs(curveValue - targetArmLength)
				if bestDiff == nil or diff < bestDiff then
					bestDiff = diff
					bestBlender = blender
				end
			end
		end
	end

	return bestBlender
end

local function centerOrbitCamera(currentPawn)
	if currentPawn == nil or currentPawn.SpringArm == nil then
		return
	end

	local activeBlender = getActiveCameraBlender(currentPawn)
	if activeBlender == nil or activeBlender.CameraData == nil or activeBlender.CollisionData == nil then
		return
	end

	local centeredCameraData = buildCenteredCameraData(activeBlender.CameraData)
	local centeredCollisionData = buildCenteredCameraData(activeBlender.CollisionData)
	if centeredCameraData == nil or centeredCollisionData == nil then
		return
	end

	currentPawn.SpringArm:SetCustomCameraData(currentPawn, "CopilotCenteredOrbit", centeredCameraData, centeredCollisionData)
	currentPawn.SpringArm:RequestRefreshState(currentPawn)
end

local function fixRaytrace()
	if configui.getValue("enable_raytracing") ~= true and configui.getValue("fix_no_raytracing_visual_glitches") == true then
		applyRaytracingMode(RaytracingMode.On)
		delay(500, function()
			applyRaytracingMode(RaytracingMode.Off)
		end)
	end
end

function on_level_change(level)
    cleanup()
	applySaveSlotLimit()
    regenerateHands(configui.getValue("hands_type") or 1)
    hideHead(true)
	delay(10000, fixRaytrace)
	if configui.getValue("fix_orbit_camera_offset") == true then
		centerOrbitCamera(pawn)
	end
	accessories.setDisabled(configui.getValue("offhand_grip_weapon") == false)
end


-- local currentOverride = false
-- register_key_bind("F1", function()
-- 	currentOverride = not currentOverride
-- 	uevrUtils.setIsInCutsceneOverride(currentOverride)
-- end)
register_key_bind("F2", function()
	print("F2 pressed")
	pawn:K2_AddActorLocalOffset(uevrUtils.vector(100,0,0), false, reusable_hit_result, true)
end)
-- local isPaused = false
-- register_key_bind("F3", function()
-- 	isPaused = not isPaused
-- 	uevrUtils.pauseGame(isPaused)
-- end)
-- register_key_bind("F4", function()
-- 	uevrUtils.profiler:report()
-- end)
-- register_key_bind("F1", function()
-- 	ending_selector.select("inwater") -- "leave" / "inwater" / "maria"
-- 	print("Set Ending to inwater")
-- end)

--When raytracing is off there can be mismatched lighting in both eyes. 
--This forces a renderer reset every 20 seconds to help mitigate that issue. 
--This is only needed when raytracing is disabled, so it will not run if the user has enabled raytracing in the settings.
--setInterval(20000,fixRaytrace)
--configui.getValue("fix_visual_glitch_seconds")
local function onRaytracingVisualGlitchTimer()
	local delaySeconds = configui.getValue("fix_visual_glitch_seconds") or 20
	if configui.getValue("fix_no_raytracing_visual_glitches") == true then
		fixRaytrace()
	else
		delaySeconds = 5
	end
	delay(delaySeconds * 1000, onRaytracingVisualGlitchTimer)
end
onRaytracingVisualGlitchTimer()

configui.onCreateOrUpdate("enable_raytracing", function(value)
	applyRaytracingMode(value == true and RaytracingMode.On or RaytracingMode.Off, true)
	configui.setHidden("fix_no_raytracing_visual_glitches_group", value)
	if value == false and configui.getValue("fix_no_raytracing_visual_glitches") == true then
		fixRaytrace()
	end
end)

configui.onCreateOrUpdate("fix_no_raytracing_visual_glitches", function(value)
	configui.setHidden("fix_visual_glitch_seconds", not value)
	if value == true then
		fixRaytrace()
	end
end)

configui.onCreateOrUpdate("interaction_sprint_mode", function(value)
	setSprintToggleEnabled(value == 2 or value == 3, true)
end)

configui.onCreateOrUpdate("interaction_control_mode", function(value)
	configui.setHidden("interaction_sprint_mode", value == 1)
	configui.setHidden("interaction_desc_advanced_group", value == 1)
end)

configui.onUpdate("light_type", function(value)
	--print("Light type changed, reattaching light")
	local light = uevrUtils.getValid(status.flashlight and status.flashlight.lightMesh)
	attachLightToController(light)
end)

configui.onCreateOrUpdate("offhand_grip_weapon", function(value)
	accessories.setDisabled(value == false)
end)

configui.onCreateOrUpdate("shotgun_manual_pump", function(value)
	if value == false then
		pcall(function()
			shotgunPump.reset("disabled")
		end)
	end
end)

configui.onCreateOrUpdate("shotgun_manual_reload", function(value)
	if value == false then
		pcall(function()
			shotgunReload.reset("disabled")
		end)
	end
end)

configui.onCreateOrUpdate("rifle_manual_bolt", function(value)
	if value == false then
		pcall(function()
			rifleBolt.reset("disabled")
		end)
	end
end)

configui.onCreateOrUpdate("rifle_manual_reload", function(value)
	if value == false then
		pcall(function()
			rifleReload.reset("disabled")
		end)
	end
end)

configui.create(configDefinition)



--The following code courtesy of Praydog
local LegacyCameraShake_c = uevrUtils.find_required_object("Class /Script/GameplayCameras.LegacyCameraShake")

-- Disable camera shake 1
local BlueprintUpdateCameraShake = LegacyCameraShake_c and LegacyCameraShake_c:find_function("BlueprintUpdateCameraShake")

if BlueprintUpdateCameraShake ~= nil then
    BlueprintUpdateCameraShake:set_function_flags(BlueprintUpdateCameraShake:get_function_flags() | 0x400) -- Mark as native
    BlueprintUpdateCameraShake:hook_ptr(function(fn, obj, locals, result)
        obj.ShakeScale = 0.0
        return false
    end)
end

-- Disable camera shake 2
local ReceivePlayShake = LegacyCameraShake_c and LegacyCameraShake_c:find_function("ReceivePlayShake")

if ReceivePlayShake ~= nil then
    ReceivePlayShake:set_function_flags(ReceivePlayShake:get_function_flags() | 0x400) -- Mark as native
    ReceivePlayShake:hook_ptr(function(fn, obj, locals, result)
        obj.ShakeScale = 0.0
        return false
    end)
end

uevr.sdk.callbacks.on_script_reset(function()
    print("Resetting")

    if BlueprintUpdateCameraShake ~= nil then
        BlueprintUpdateCameraShake:set_function_flags(BlueprintUpdateCameraShake:get_function_flags() & ~0x400) -- Unmark as native
    end

    if ReceivePlayShake ~= nil then
        ReceivePlayShake:set_function_flags(ReceivePlayShake:get_function_flags() & ~0x400) -- Unmark as native
    end

	pcall(function()
		resetPistolManualReload("reset")
	end)
	pcall(function()
		shotgunPump.reset("reset")
	end)
	pcall(function()
		shotgunReload.reset("reset")
	end)
	pcall(function()
		rifleBolt.reset("reset")
	end)
	pcall(function()
		rifleReload.reset("reset")
	end)
end)