local vertices = {
    { position = {-1.0, -1.0, -1.0}, color = {1.0, 0.0, 0.0} },
    { position = {1.0, -1.0, -1.0}, color = {0.0, 1.0, 0.0} },
    { position = {1.0, 1.0, -1.0}, color = {0.0, 0.0, 1.0} },
    { position = {-1.0, 1.0, -1.0}, color = {1.0, 1.0, 0.0} },
    { position = {-1.0, -1.0, 1.0}, color = {1.0, 0.0, 1.0} },
    { position = {1.0, -1.0, 1.0}, color = {0.0, 1.0, 1.0} },
    { position = {1.0, 1.0, 1.0}, color = {1.0, 1.0, 1.0} },
    { position = {-1.0, 1.0, 1.0}, color = {0.2, 0.2, 0.2} },
}

local indices = {
    1, 2, 3, 3, 4, 1, -- back
    5, 6, 7, 7, 8, 5, -- front
    1, 5, 8, 8, 4, 1, -- left
    2, 6, 7, 7, 3, 2, -- right
    4, 3, 7, 7, 8, 4, -- top
    1, 2, 6, 6, 5, 1, -- bottom
}

local rotation_speeds = {x = 0.5, y = 0.7}
local shader_paths = {
    vertex = "shaders/cube.vert.spv",
    fragment = "shaders/cube.frag.spv",
}

local function rotation_y(radians)
    local c = math.cos(radians)
    local s = math.sin(radians)
    return {
        c,  0.0, -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        s,  0.0,  c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

local function rotation_x(radians)
    local c = math.cos(radians)
    local s = math.sin(radians)
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0,  c,  s, 0.0,
        0.0, -s,  c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

local function multiply_matrices(a, b)
    local result = {}
    for row = 1, 4 do
        for col = 1, 4 do
            local sum = 0.0
            for idx = 1, 4 do
                sum = sum + a[(idx - 1) * 4 + row] * b[(col - 1) * 4 + idx]
            end
            result[(col - 1) * 4 + row] = sum
        end
    end
    return result
end

local function build_model(time)
    local y = rotation_y(time * rotation_speeds.y)
    local x = rotation_x(time * rotation_speeds.x)
    return multiply_matrices(y, x)
end

function get_cube_vertices()
    return vertices
end

function get_cube_indices()
    return indices
end

function compute_model_matrix(time)
    return build_model(time)
end

function get_shader_paths()
    return shader_paths
end
