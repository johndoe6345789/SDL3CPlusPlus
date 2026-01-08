local math3d = {}

local function require_glm(name)
    local fn = _G[name]
    if type(fn) ~= "function" then
        error("math3d requires missing binding: " .. name)
    end
    return fn
end

local glm_identity = require_glm("glm_matrix_identity")
local glm_multiply = require_glm("glm_matrix_multiply")
local glm_translation = require_glm("glm_matrix_translation")
local glm_rotation_x = require_glm("glm_matrix_rotation_x")
local glm_rotation_y = require_glm("glm_matrix_rotation_y")
local glm_from_transform = require_glm("glm_matrix_from_transform")
local glm_look_at = require_glm("glm_matrix_look_at")
local glm_perspective = require_glm("glm_matrix_perspective")

local function normalize(vec)
    local x, y, z = vec[1], vec[2], vec[3]
    local len = math.sqrt(x * x + y * y + z * z)
    if len == 0.0 then
        return {x, y, z}
    end
    return {x / len, y / len, z / len}
end

local function cross(a, b)
    return {
        a[2] * b[3] - a[3] * b[2],
        a[3] * b[1] - a[1] * b[3],
        a[1] * b[2] - a[2] * b[1],
    }
end

local function dot(a, b)
    return a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
end

local function identity_matrix()
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

function math3d.identity()
    return glm_identity()
end

function math3d.multiply(a, b)
    return glm_multiply(a, b)
end

function math3d.translation(x, y, z)
    return glm_translation(x, y, z)
end

function math3d.rotation_x(radians)
    return glm_rotation_x(radians)
end

function math3d.rotation_y(radians)
    return glm_rotation_y(radians)
end

function math3d.from_transform(translation, rotation)
    return glm_from_transform(translation, rotation)
end

function math3d.look_at(eye, center, up)
    return glm_look_at(eye, center, up)
end

function math3d.perspective(fov, aspect, zNear, zFar)
    return glm_perspective(fov, aspect, zNear, zFar)
end

return math3d
