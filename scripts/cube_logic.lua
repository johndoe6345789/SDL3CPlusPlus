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

local math3d = require("math3d")

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

local camera = {
    eye = {2.0, 2.0, 2.5},
    center = {0.0, 0.0, 0.0},
    up = {0.0, 1.0, 0.0},
    fov = 0.78,
    near = 0.1,
    far = 10.0,
}

local function build_model(time)
    local y = math3d.rotation_y(time * rotation_speeds.y)
    local x = math3d.rotation_x(time * rotation_speeds.x)
    return math3d.multiply(y, x)
end

local function create_cube(position, speed_scale, shader_key)
    local function compute_model_matrix(time)
        local base = build_model(time * speed_scale)
        local offset = math3d.translation(position[1], position[2], position[3])
        return math3d.multiply(offset, base)
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
        local offset = math3d.translation(position[1], position[2], position[3])
        return math3d.multiply(offset, base)
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

function get_view_projection(aspect)
    local view = math3d.look_at(camera.eye, camera.center, camera.up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)
    return math3d.multiply(projection, view)
end
