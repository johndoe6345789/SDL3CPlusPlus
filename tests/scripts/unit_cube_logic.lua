local function identity_matrix()
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

function get_scene_objects()
    return {
        {
            vertices = {
                { position = {0.0, 0.0, 0.0}, color = {1.0, 0.0, 0.0} },
                { position = {1.0, 0.0, 0.0}, color = {0.0, 1.0, 0.0} },
                { position = {0.0, 1.0, 0.0}, color = {0.0, 0.0, 1.0} },
            },
            indices = {1, 2, 3},
            compute_model_matrix = function(time)
                return identity_matrix()
            end,
            shader_key = "test",
        },
    }
end

function get_shader_paths()
    return {
        test = {
            vertex = "shaders/test.vert.spv",
            fragment = "shaders/test.frag.spv",
        },
    }
end

function get_view_projection(aspect)
    return identity_matrix()
end

function compute_model_matrix(time)
    return identity_matrix()
end
