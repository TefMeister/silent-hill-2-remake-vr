-- sh2vr_bridge.lua -- hands the C++ plugin (sh2vr.dll) the answers the Lua pack's modules still decide:
-- whether the hands should be hidden (UI open, montage) and whether each arm is being animated by the game's own
-- mesh (cutscenes). The pack asks these through its callback web; this script asks the same way every 200 ms
-- (the pace ik.lua used) and publishes the answers as UEVR mod values the plugin reads each frame.
local uevrUtils = require("libs/uevr_utils")

local LEFT, RIGHT = 0, 1   -- Handed.Left / Handed.Right

local function flag(v)
	if v == true then return "1" elseif v == false then return "0" end
	return ""
end

uevrUtils.setInterval(200, function()
	local hidden = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_hidden")
	local animLeft = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", LEFT)
	local animRight = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", RIGHT)
	uevr.params.vr.set_mod_value("sh2vr_hands_hidden", flag(hidden))
	uevr.params.vr.set_mod_value("sh2vr_anim_left", flag(animLeft))
	uevr.params.vr.set_mod_value("sh2vr_anim_right", flag(animRight))
end)
