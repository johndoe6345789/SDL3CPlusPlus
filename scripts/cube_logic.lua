local cube_vertices = {
    { position = {-1.0, -1.0, -1.0}, color = {1.0, 0.0, 0.0} },
    { position = {1.0, -1.0, -1.0}, color = {0.0, 1.0, 0.0} },
    { position = {1.0, 1.0, -1.0}, color = {0.0, 0.0, 1.0} },
    { position = {-1.0, 1.0, -1.0}, color = {1.0, 1.0, 0.0} },
    { position = {-1.0, -1.0, 1.0}, color = {1.0, 0.0, 1.0} },
    { position = {1.0, -1.0, 1.0}, color = {0.0, 1.0, 1.0} },
    { position = {1.0, 1.0, 1.0}, color = {1.0, 1.0, 1.0} },
    { position = {-1.0, 1.0, 1.0}, color = {0.2, 0.2, 0.2} },
}

local cube_indices = {
    1, 2, 3, 3, 4, 1, -- back
    5, 6, 7, 7, 8, 5, -- front
    1, 5, 8, 8, 4, 1, -- left
    2, 6, 7, 7, 3, 2, -- right
    4, 3, 7, 7, 8, 4, -- top
    1, 2, 6, 6, 5, 1, -- bottom
}

local pyramid_vertices = {
    { position = {0.0, 1.0, 0.0}, color = {1.0, 0.5, 0.0} },
    { position = {-1.0, -1.0, -1.0}, color = {0.0, 1.0, 1.0} },
    { position = {1.0, -1.0, -1.0}, color = {1.0, 0.0, 1.0} },
    { position = {1.0, -1.0, 1.0}, color = {1.0, 1.0, 0.0} },
    { position = {-1.0, -1.0, 1.0}, color = {0.0, 0.0, 1.0} },
}

local pyramid_indices = {
    1, 2, 3,
    1, 3, 4,
    1, 4, 5,
    1, 5, 2,
    2, 3, 4,
    4, 5, 2,
}

local rotation_speeds = {x = 0.5, y = 0.7}

local shader_variants = {
    default = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
    cube = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
    pyramid = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
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

local function translation(x, y, z)
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        x,   y,   z,   1.0,
    }
end

local function build_model(time)
    local y = rotation_y(time * rotation_speeds.y)
    local x = rotation_x(time * rotation_speeds.x)
    return multiply_matrices(y, x)
end

local function create_cube(position, speed_scale, shader_key)
    local function compute_model_matrix(time)
        local base = build_model(time * speed_scale)
        local offset = translation(position[1], position[2], position[3])
        return multiply_matrices(offset, base)
    end

    return {
        vertices = cube_vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = shader_key or "cube",
    }
end

local function create_pyramid(position, shader_key)
    local function compute_model_matrix(time)
        local base = build_model(time * 0.6)
        local offset = translation(position[1], position[2], position[3])
        return multiply_matrices(offset, base)
    end

    return {
        vertices = pyramid_vertices,
        indices = pyramid_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = shader_key or "pyramid",
    }
end

function get_scene_objects()
    return {
        create_cube({0.0, 0.0, 0.0}, 1.0, "cube"),
        create_cube({3.0, 0.0, 0.0}, 0.8, "cube"),
        create_cube({-3.0, 0.0, 0.0}, 1.2, "cube"),
        create_pyramid({0.0, -0.5, -4.0}, "pyramid"),
    }
end

function get_shader_paths()
    return shader_variants
end
