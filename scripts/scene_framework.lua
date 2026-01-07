-- Scene Framework: Reusable components for 3D scene construction
-- Provides mesh generation, object builders, and utility functions

local math3d = require("math3d")

local framework = {}

-- ============================================================================
-- Config Resolution Utilities
-- ============================================================================

function framework.resolve_number(value, fallback)
    if type(value) == "number" then
        return value
    end
    return fallback
end

function framework.resolve_boolean(value, fallback)
    if type(value) == "boolean" then
        return value
    end
    return fallback
end

function framework.resolve_string(value, fallback)
    if type(value) == "string" then
        return value
    end
    return fallback
end

function framework.resolve_table(value, fallback)
    if type(value) == "table" then
        return value
    end
    return fallback or {}
end

function framework.resolve_vec3(value, fallback)
    if type(value) == "table"
        and type(value[1]) == "number"
        and type(value[2]) == "number"
        and type(value[3]) == "number" then
        return {value[1], value[2], value[3]}
    end
    return {fallback[1], fallback[2], fallback[3]}
end

-- ============================================================================
-- Mesh Generation
-- ============================================================================

--- Generate a tessellated plane mesh with normals pointing up (+Y)
--- @param width number Width along X axis
--- @param depth number Depth along Z axis
--- @param subdivisions number Number of divisions per axis (e.g., 20 = 400 quads = 441 vertices)
--- @param color table RGB color {r, g, b} with values 0-1, defaults to white
--- @return table vertices Array of vertex tables with position, normal, color, texcoord
--- @return table indices Array of uint16 triangle indices
function framework.generate_plane_mesh(width, depth, subdivisions, color)
    color = color or {1.0, 1.0, 1.0}
    subdivisions = subdivisions or 1
    
    local vertices = {}
    local indices = {}
    local half_width = width / 2
    local half_depth = depth / 2
    local step_x = width / subdivisions
    local step_z = depth / subdivisions
    
    -- Generate vertices
    for z = 0, subdivisions do
        for x = 0, subdivisions do
            local px = -half_width + x * step_x
            local pz = -half_depth + z * step_z
            local u = x / subdivisions
            local v = z / subdivisions
            
            table.insert(vertices, {
                position = {px, 0.0, pz},
                normal = {0.0, 1.0, 0.0},
                color = color,
                texcoord = {u, v}
            })
        end
    end
    
    -- Generate indices (two triangles per quad)
    for z = 0, subdivisions - 1 do
        for x = 0, subdivisions - 1 do
            local base = z * (subdivisions + 1) + x
            local v0 = base
            local v1 = base + 1
            local v2 = base + subdivisions + 1
            local v3 = base + subdivisions + 2
            
            -- First triangle
            table.insert(indices, v0)
            table.insert(indices, v2)
            table.insert(indices, v1)
            
            -- Second triangle
            table.insert(indices, v1)
            table.insert(indices, v2)
            table.insert(indices, v3)
        end
    end
    
    return vertices, indices
end

--- Apply a color to a copy of vertices array
--- @param vertices table Source vertices array
--- @param color table RGB color {r, g, b} with values 0-1
--- @return table New vertices array with color applied
function framework.apply_color_to_vertices(vertices, color)
    local colored = {}
    for i = 1, #vertices do
        local v = vertices[i]
        colored[i] = {
            position = v.position,
            normal = v.normal,
            color = color,
            texcoord = v.texcoord
        }
    end
    return colored
end

--- Flip normals for a vertices array (useful for ceilings, inside-out geometry)
--- @param vertices table Vertices array to modify in-place
function framework.flip_normals(vertices)
    for i = 1, #vertices do
        local n = vertices[i].normal
        vertices[i].normal = {-n[1], -n[2], -n[3]}
    end
end

--- Generate a cube mesh (unit cube from -0.5 to 0.5)
--- @param double_sided boolean If true, generates back faces for inside-out rendering
--- @return table vertices Array of 24 vertices (4 per face)
--- @return table indices Array of triangle indices (36 for single-sided, 72 for double-sided)
function framework.generate_cube_mesh(double_sided)
    -- Standard unit cube vertices (8 unique positions, but 24 vertices for proper normals per face)
    local vertices = {
        -- Front face (+Z)
        {position = {-0.5, -0.5,  0.5}, normal = { 0.0,  0.0,  1.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = { 0.5, -0.5,  0.5}, normal = { 0.0,  0.0,  1.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = { 0.5,  0.5,  0.5}, normal = { 0.0,  0.0,  1.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = {-0.5,  0.5,  0.5}, normal = { 0.0,  0.0,  1.0}, color = {1, 1, 1}, texcoord = {0, 1}},
        
        -- Back face (-Z)
        {position = { 0.5, -0.5, -0.5}, normal = { 0.0,  0.0, -1.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = {-0.5, -0.5, -0.5}, normal = { 0.0,  0.0, -1.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = {-0.5,  0.5, -0.5}, normal = { 0.0,  0.0, -1.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = { 0.5,  0.5, -0.5}, normal = { 0.0,  0.0, -1.0}, color = {1, 1, 1}, texcoord = {0, 1}},
        
        -- Top face (+Y)
        {position = {-0.5,  0.5,  0.5}, normal = { 0.0,  1.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = { 0.5,  0.5,  0.5}, normal = { 0.0,  1.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = { 0.5,  0.5, -0.5}, normal = { 0.0,  1.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = {-0.5,  0.5, -0.5}, normal = { 0.0,  1.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 1}},
        
        -- Bottom face (-Y)
        {position = {-0.5, -0.5, -0.5}, normal = { 0.0, -1.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = { 0.5, -0.5, -0.5}, normal = { 0.0, -1.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = { 0.5, -0.5,  0.5}, normal = { 0.0, -1.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = {-0.5, -0.5,  0.5}, normal = { 0.0, -1.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 1}},
        
        -- Right face (+X)
        {position = { 0.5, -0.5,  0.5}, normal = { 1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = { 0.5, -0.5, -0.5}, normal = { 1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = { 0.5,  0.5, -0.5}, normal = { 1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = { 0.5,  0.5,  0.5}, normal = { 1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 1}},
        
        -- Left face (-X)
        {position = {-0.5, -0.5, -0.5}, normal = {-1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 0}},
        {position = {-0.5, -0.5,  0.5}, normal = {-1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 0}},
        {position = {-0.5,  0.5,  0.5}, normal = {-1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {1, 1}},
        {position = {-0.5,  0.5, -0.5}, normal = {-1.0,  0.0,  0.0}, color = {1, 1, 1}, texcoord = {0, 1}},
    }
    
    local indices = {
        -- Front
        0, 1, 2,  2, 3, 0,
        -- Back
        4, 5, 6,  6, 7, 4,
        -- Top
        8, 9, 10,  10, 11, 8,
        -- Bottom
        12, 13, 14,  14, 15, 12,
        -- Right
        16, 17, 18,  18, 19, 16,
        -- Left
        20, 21, 22,  22, 23, 20,
    }
    
    if double_sided then
        -- Add reversed winding order for back faces
        local reverse_indices = {
            -- Front (reversed)
            2, 1, 0,  0, 3, 2,
            -- Back (reversed)
            6, 5, 4,  4, 7, 6,
            -- Top (reversed)
            10, 9, 8,  8, 11, 10,
            -- Bottom (reversed)
            14, 13, 12,  12, 15, 14,
            -- Right (reversed)
            18, 17, 16,  16, 19, 18,
            -- Left (reversed)
            22, 21, 20,  20, 23, 22,
        }
        for i = 1, #reverse_indices do
            indices[#indices + 1] = reverse_indices[i]
        end
    end
    
    return vertices, indices
end

-- ============================================================================
-- Transform Utilities
-- ============================================================================

--- Build a scale matrix manually (math3d doesn't have scale function)
--- @param x number X-axis scale
--- @param y number Y-axis scale
--- @param z number Z-axis scale
--- @return table 4x4 scale matrix
local function scale_matrix(x, y, z)
    return {
        x, 0.0, 0.0, 0.0,
        0.0, y, 0.0, 0.0,
        0.0, 0.0, z, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

--- Build a static model matrix from position and scale
--- @param position table {x, y, z} world position
--- @param scale table {sx, sy, sz} scale factors
--- @return table 4x4 matrix as flat array of 16 floats
function framework.build_static_model_matrix(position, scale)
    local translation = math3d.translation(position[1], position[2], position[3])
    local scaling = scale_matrix(scale[1], scale[2], scale[3])
    return math3d.multiply(translation, scaling)
end

-- ============================================================================
-- Scene Object Builders
-- ============================================================================

--- Create a static scene object (no animation)
--- @param vertices table Vertex array
--- @param indices table Index array
--- @param position table {x, y, z} world position
--- @param scale table {sx, sy, sz} scale factors
--- @param shader_key string Material/shader identifier
--- @param object_type string Semantic object type for identification
--- @return table Scene object with standard structure
function framework.create_static_object(vertices, indices, position, scale, shader_key, object_type)
    local model_matrix = framework.build_static_model_matrix(position, scale)
    
    local function compute_model_matrix()
        return model_matrix
    end
    
    return {
        vertices = vertices,
        indices = indices,
        compute_model_matrix = compute_model_matrix,
        shader_keys = {shader_key},
        object_type = object_type or "static",
    }
end

--- Create a static cube object with color
--- @param position table {x, y, z} world position
--- @param scale table {sx, sy, sz} scale factors
--- @param color table {r, g, b} vertex color
--- @param shader_key string Material/shader identifier
--- @param object_type string Semantic object type
--- @param cube_mesh table Optional pre-generated cube mesh {vertices, indices}
--- @return table Scene object
function framework.create_static_cube(position, scale, color, shader_key, object_type, cube_mesh)
    if not cube_mesh then
        cube_mesh = {framework.generate_cube_mesh(false)}
    end
    
    local vertices = cube_mesh[1] or cube_mesh.vertices
    local indices = cube_mesh[2] or cube_mesh.indices
    
    if color then
        vertices = framework.apply_color_to_vertices(vertices, color)
    end
    
    return framework.create_static_object(vertices, indices, position, scale, shader_key, object_type)
end

--- Create a dynamic scene object with animation callback
--- @param vertices table Vertex array
--- @param indices table Index array
--- @param compute_fn function Function(time) -> matrix that computes model matrix
--- @param shader_key string Material/shader identifier
--- @param object_type string Semantic object type
--- @return table Scene object with dynamic transform
function framework.create_dynamic_object(vertices, indices, compute_fn, shader_key, object_type)
    return {
        vertices = vertices,
        indices = indices,
        compute_model_matrix = compute_fn,
        shader_keys = {shader_key},
        object_type = object_type or "dynamic",
    }
end

-- ============================================================================
-- Material Registry
-- ============================================================================

framework.MaterialRegistry = {}
framework.MaterialRegistry.__index = framework.MaterialRegistry

function framework.MaterialRegistry.new(config)
    local self = setmetatable({}, framework.MaterialRegistry)
    self.materials = {}
    self.default_key = nil
    
    if config and config.materialx_materials then
        for i, mat in ipairs(config.materialx_materials) do
            if mat.shader_key then
                self.materials[mat.shader_key] = mat
                if i == 1 then
                    self.default_key = mat.shader_key
                end
            end
        end
    end
    
    return self
end

function framework.MaterialRegistry:get(shader_key)
    return self.materials[shader_key]
end

function framework.MaterialRegistry:get_key(shader_key)
    if self.materials[shader_key] then
        return shader_key
    end
    return self.default_key
end

function framework.MaterialRegistry:has(shader_key)
    return self.materials[shader_key] ~= nil
end

return framework
