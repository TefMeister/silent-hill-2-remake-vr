-- sh2_bone_dump.lua - one-shot probe: read James's REAL skeleton and object graph from the live game.
-- Why: every bone/object name in the dossier (§8e) came from a community profile's config, not the game.
-- Runs under UEVR. Waits for the player pawn, dumps once, then goes quiet. Writes to UEVR's log and,
-- if the sandbox allows it, to sh2_bone_dump.txt in the UEVR profile folder.
-- Probe only: archive it once its question is answered.

local api = uevr.api
local done = false
local tries = 0
local lines = {}

local function out(s)
    lines[#lines + 1] = s
    print("[sh2dump] " .. s)
end

local function flush()
    local ok = pcall(function()
        local f = io.open("sh2_bone_dump.txt", "w")
        f:write(table.concat(lines, "\n"), "\n")
        f:close()
    end)
    if not ok then pcall(function() fs.write("sh2_bone_dump.txt", table.concat(lines, "\n")) end) end
end

local function str(v)
    if v == nil then return "nil" end
    local ok, s = pcall(function() return v:to_string() end)
    if ok then return s end
    return tostring(v)
end

local function full_name(o)
    if o == nil then return "nil" end
    local ok, s = pcall(function() return o:get_full_name() end)
    return ok and s or tostring(o)
end

local function field(o, name)
    local ok, v = pcall(function() return o[name] end)
    if ok then return v end
    return nil
end

local function dump()
    lines = {}
    local pawn = api:get_local_pawn(0)
    if pawn == nil then return false end
    local mesh = field(pawn, "Mesh")
    if mesh == nil then return false end
    local n = mesh:GetNumBones()
    if n == nil or n == 0 then return false end

    local ksl = api:find_uobject("Class /Script/Engine.KismetStringLibrary")
    ksl = ksl and ksl:get_class_default_object()
    local function fname(s) return ksl:Conv_StringToName(s) end

    out("pawn   " .. full_name(pawn))
    out("mesh   " .. full_name(mesh))
    out("skel   " .. full_name(field(mesh, "SkeletalMesh")))
    out("bones  " .. tostring(n))
    for i = 0, n - 1 do
        local b = mesh:GetBoneName(i)
        local p = mesh:GetParentBone(b)
        out(string.format("bone %4d  %-40s parent %s", i, str(b), str(p)))
    end

    for _, s in ipairs({"hand_l_socket", "hand_r_socket", "hand_l_bn", "hand_r_bn", "spine_05_bn",
                        "neck_02_bn", "clavicle_01_l_bn", "clavicle_01_r_bn", "upperarm_l_bn",
                        "upperarm_r_bn", "lowerarm_l_bn", "lowerarm_r_bn"}) do
        local ok, r = pcall(function() return mesh:DoesSocketExist(fname(s)) end)
        out(string.format("exists %-20s %s", s, ok and tostring(r) or "call failed"))
    end

    -- Object graph from dossier §8e
    local anim = field(mesh, "AnimScriptInstance")
    out("anim   " .. full_name(anim))
    local wm = anim and field(anim, "WeaponManageCmbSubcomp")
    out("wmgr   " .. full_name(wm))
    local weap = wm and field(wm, "EquippedWeapon")
    out("weapon " .. full_name(weap))
    if weap ~= nil then
        local root = field(weap, "RootComponent")
        out("w.root   " .. full_name(root))
        out("w.parent " .. full_name(root and field(root, "AttachParent")))
        out("w.socket " .. str(root and field(root, "AttachSocketName")))
    end
    local items = field(pawn, "Items")
    out("items  " .. full_name(items))
    out("move   " .. full_name(field(pawn, "Movement")))
    local mv = field(pawn, "Movement")
    out("push   " .. full_name(mv and field(mv, "PushableComponent")))
    out("view   " .. full_name(field(pawn, "View")))
    out("camovl " .. full_name(field(pawn, "CameraOverlapHandler")))
    flush()
    return true
end

uevr.sdk.callbacks.on_pre_engine_tick(function(engine, delta)
    if done then return end
    tries = tries + 1
    if tries % 120 ~= 0 then return end -- about every 1-2 s
    local ok, r = pcall(dump)
    if ok and r then
        done = true
        print("[sh2dump] DONE")
    elseif not ok then
        print("[sh2dump] error: " .. tostring(r))
    end
end)

print("[sh2dump] loaded")
