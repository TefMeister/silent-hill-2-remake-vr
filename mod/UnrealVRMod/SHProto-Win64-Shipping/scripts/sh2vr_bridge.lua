-- sh2vr_bridge.lua -- the two-way line between the Lua pack and the C++ plugin (sh2vr.dll) while both run.
-- Lua -> plugin: every 200 ms (the pace ik.lua used) the answers the pack's modules still decide: whether the
--   hands should be hidden (UI open, montage), whether each arm is animated by the game's own mesh (cutscenes),
--   which grip pose each hand holds (attachments / accessories: "-" none, "+" the default grip, else its name)
--   and how many hand montages have ended (the C++ finger poses re-pose on a change), one custom event "sh2vr_flags".
-- plugin -> Lua: the plugin names the objects it made (the IK arm mesh) with Lua events; the pack's modules that
--   hang things on them (attachments, melee) read SH2VR_IK_MESH_NAME through ik.getCurrentMesh().
-- 2026-10-31 evening: plugin -> Lua "sh2vr_grips" ("L=1;R=0"): the REAL grips. The plugin's button layer keeps the grips from
--   the pad, so the shotgun / rifle reloads and the pump / bolt below read SH2VR_GRIP_L instead of the pad's LB.
--   Lua -> plugin "sh2vr_gate" ("gate=1"): a pump or bolt is owed; the plugin's lowered trigger waits for it.
local uevrUtils = require("libs/uevr_utils")
local handsAnimation = require("libs/hands_animation")

local LEFT, RIGHT = 0, 1   -- Handed.Left / Handed.Right
SH2VR_IK_MESH_NAME = SH2VR_IK_MESH_NAME or nil
SH2VR_GRIP_L = SH2VR_GRIP_L or nil   -- nil until the plugin speaks: the scripts then read the pad as before
SH2VR_GRIP_R = SH2VR_GRIP_R or nil

-- the pump / bolt gate, from the pack's own modules (main.lua required them first; require hands back the same table)
local okPump, shotgunPump = pcall(require, "shotgun_pump")
local okBolt, rifleBolt = pcall(require, "rifle_bolt")
local function gateShut()
	local shut = false
	if okPump and shotgunPump ~= nil and shotgunPump.isBlockingFire ~= nil then
		pcall(function() shut = shut or shotgunPump.isBlockingFire() == true end)
	end
	if okBolt and rifleBolt ~= nil and rifleBolt.isBlockingFire ~= nil then
		pcall(function() shut = shut or rifleBolt.isBlockingFire() == true end)
	end
	return shut
end
local gateSent = nil
uevrUtils.registerPreEngineTickCallback(function(engine, delta)
	local shut = gateShut()
	if shut ~= gateSent then
		gateSent = shut
		pcall(function() uevr.params.functions.dispatch_custom_event("sh2vr_gate", "gate=" .. (shut and "1" or "0")) end)
	end
end)

local function flag(v)
	if v == true then return "1" end
	return "0"
end

local function grip(hand)
	local a = handsAnimation.getEffectiveAttachment(hand)
	if a == nil or a == false or a == "attachment_none" then return "-" end
	if a == true or a == "" then return "+" end
	return tostring(a)
end

local montagesEnded = 0
uevrUtils.registerUEVRCallback("on_module_montage_change", function(montageObject, montageName, label, animInstance)
	if montageName == nil or montageName == "" then montagesEnded = montagesEnded + 1 end
end)

local said = false
uevrUtils.setInterval(200, function()
	local hidden = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_hidden")
	local animLeft = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", LEFT)
	local animRight = uevrUtils.executeUEVRCallbacksWithPriorityBooleanResult("is_hands_animating_from_mesh", RIGHT)
	local data = "hidden=" .. flag(hidden) .. ";animL=" .. flag(animLeft) .. ";animR=" .. flag(animRight)
		.. ";gripL=" .. grip(LEFT) .. ";gripR=" .. grip(RIGHT) .. ";mont=" .. tostring(montagesEnded)
		.. ";gate=" .. flag(gateShut())
	local ok, err = pcall(function() uevr.params.functions.dispatch_custom_event("sh2vr_flags", data) end)
	if not ok and not said then
		said = true
		print("[sh2vr_bridge] dispatch_custom_event is not available: " .. tostring(err))
	end
end)

uevr.sdk.callbacks.on_lua_event(function(eventName, eventData)
	if eventName == "sh2vr_grips" and eventData ~= nil then
		SH2VR_GRIP_L = string.find(eventData, "L=1", 1, true) ~= nil
		SH2VR_GRIP_R = string.find(eventData, "R=1", 1, true) ~= nil
	elseif eventName == "sh2vr_ik_mesh" then
		if eventData ~= nil and eventData ~= "" then SH2VR_IK_MESH_NAME = eventData else SH2VR_IK_MESH_NAME = nil end
	end
end)
