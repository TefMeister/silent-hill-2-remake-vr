-- sh2vr_bridge.lua -- the two-way line between the Lua pack and the C++ plugin (sh2vr.dll) while both run.
-- Lua -> plugin: every 200 ms (the pace ik.lua used) the answers the pack's modules still decide: whether the
--   hands should be hidden (UI open, montage) and whether each arm is animated by the game's own mesh (cutscenes),
--   sent as one custom event "sh2vr_flags".
-- plugin -> Lua: the plugin names the objects it made (the IK arm mesh) with Lua events; the pack's modules that
--   hang things on them (attachments, melee) read SH2VR_IK_MESH_NAME through ik.getCurrentMesh().
local uevrUtils = require("libs/uevr_utils")

local LEFT, RIGHT = 0, 1   -- Handed.Left / Handed.Right
SH2VR_IK_MESH_NAME = SH2VR_IK_MESH_NAME or nil

local function flag(v)
	if v == true then return "1" end
	return "0"
end

local said = false
uevrUtils.setInterval(200, function()
	local hidden = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_hidden")
	local animLeft = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", LEFT)
	local animRight = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", RIGHT)
	local data = "hidden=" .. flag(hidden) .. ";animL=" .. flag(animLeft) .. ";animR=" .. flag(animRight)
	local ok, err = pcall(function() uevr.params.functions.dispatch_custom_event("sh2vr_flags", data) end)
	if not ok and not said then
		said = true
		print("[sh2vr_bridge] dispatch_custom_event is not available: " .. tostring(err))
	end
end)

uevr.sdk.callbacks.on_lua_event(function(eventName, eventData)
	if eventName == "sh2vr_ik_mesh" then
		if eventData ~= nil and eventData ~= "" then SH2VR_IK_MESH_NAME = eventData else SH2VR_IK_MESH_NAME = nil end
	end
end)
