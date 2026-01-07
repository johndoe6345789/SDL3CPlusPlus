-- Test suite for scene_framework.lua
-- Run with: lua scripts/test_scene_framework.lua

-- Mock math3d for standalone testing
package.preload['math3d'] = function()
    local math3d = {}
    
    function math3d.translation(x, y, z)
        return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1}
    end
    
    function math3d.multiply(a, b)
        -- Simplified matrix multiply for test purposes
        return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}
    end
    
    return math3d
end

local framework = require("scene_framework")

local tests_run = 0
local tests_passed = 0
local tests_failed = 0

local function assert_equal(actual, expected, message)
    tests_run = tests_run + 1
    if actual == expected then
        tests_passed = tests_passed + 1
        return true
    else
        tests_failed = tests_failed + 1
        print(string.format("FAIL: %s (expected %s, got %s)", message, tostring(expected), tostring(actual)))
        return false
    end
end

local function assert_near(actual, expected, epsilon, message)
    tests_run = tests_run + 1
    local diff = math.abs(actual - expected)
    if diff <= epsilon then
        tests_passed = tests_passed + 1
        return true
    else
        tests_failed = tests_failed + 1
        print(string.format("FAIL: %s (expected %f±%f, got %f, diff %f)", 
            message, expected, epsilon, actual, diff))
        return false
    end
end

local function assert_true(condition, message)
    return assert_equal(condition, true, message)
end

local function assert_not_nil(value, message)
    tests_run = tests_run + 1
    if value ~= nil then
        tests_passed = tests_passed + 1
        return true
    else
        tests_failed = tests_failed + 1
        print(string.format("FAIL: %s (value is nil)", message))
        return false
    end
end

-- ============================================================================
-- Config Resolution Tests
-- ============================================================================

print("Testing config resolution utilities...")

assert_equal(framework.resolve_number(42, 0), 42, "resolve_number with number")
assert_equal(framework.resolve_number("not a number", 99), 99, "resolve_number with fallback")
assert_equal(framework.resolve_number(nil, 123), 123, "resolve_number with nil")

assert_equal(framework.resolve_boolean(true, false), true, "resolve_boolean with true")
assert_equal(framework.resolve_boolean(false, true), false, "resolve_boolean with false")
assert_equal(framework.resolve_boolean("not bool", true), true, "resolve_boolean with fallback")

assert_equal(framework.resolve_string("hello", "default"), "hello", "resolve_string with string")
assert_equal(framework.resolve_string(123, "default"), "default", "resolve_string with fallback")

local tbl = {1, 2, 3}
assert_equal(framework.resolve_table(tbl, {}), tbl, "resolve_table with table")
assert_equal(type(framework.resolve_table("not table", nil)), "table", "resolve_table creates empty table")

local vec = framework.resolve_vec3({1, 2, 3}, {0, 0, 0})
assert_equal(vec[1], 1, "resolve_vec3 x component")
assert_equal(vec[2], 2, "resolve_vec3 y component")
assert_equal(vec[3], 3, "resolve_vec3 z component")

local fallback_vec = framework.resolve_vec3("invalid", {7, 8, 9})
assert_equal(fallback_vec[1], 7, "resolve_vec3 fallback x")
assert_equal(fallback_vec[2], 8, "resolve_vec3 fallback y")
assert_equal(fallback_vec[3], 9, "resolve_vec3 fallback z")

-- ============================================================================
-- Mesh Generation Tests
-- ============================================================================

print("Testing mesh generation...")

-- Test plane mesh generation
local plane_verts, plane_indices = framework.generate_plane_mesh(10, 10, 2, {1.0, 0.5, 0.25})
assert_equal(#plane_verts, 9, "plane 2x2 subdivision has 9 vertices (3x3 grid)")
assert_equal(#plane_indices, 24, "plane 2x2 subdivision has 24 indices (8 triangles)")

-- Check first vertex
assert_not_nil(plane_verts[1], "plane first vertex exists")
assert_equal(type(plane_verts[1].position), "table", "vertex has position")
assert_equal(type(plane_verts[1].normal), "table", "vertex has normal")
assert_equal(type(plane_verts[1].color), "table", "vertex has color")
assert_equal(plane_verts[1].color[1], 1.0, "plane vertex color r")
assert_equal(plane_verts[1].color[2], 0.5, "plane vertex color g")
assert_equal(plane_verts[1].color[3], 0.25, "plane vertex color b")

-- Test plane with default color
local white_verts, _ = framework.generate_plane_mesh(10, 10, 1)
assert_equal(white_verts[1].color[1], 1.0, "default plane color is white r")
assert_equal(white_verts[1].color[2], 1.0, "default plane color is white g")
assert_equal(white_verts[1].color[3], 1.0, "default plane color is white b")

-- Test cube mesh generation
local cube_verts, cube_indices = framework.generate_cube_mesh(false)
assert_equal(#cube_verts, 24, "cube has 24 vertices (4 per face)")
assert_equal(#cube_indices, 36, "single-sided cube has 36 indices (12 triangles)")

local cube_double, cube_double_idx = framework.generate_cube_mesh(true)
assert_equal(#cube_double, 24, "double-sided cube still has 24 vertices")
assert_equal(#cube_double_idx, 72, "double-sided cube has 72 indices (24 triangles)")

-- Test apply color
local colored_verts = framework.apply_color_to_vertices(cube_verts, {0.8, 0.6, 0.4})
assert_equal(#colored_verts, #cube_verts, "colored vertices same count as input")
assert_equal(colored_verts[1].color[1], 0.8, "applied color r")
assert_equal(colored_verts[1].color[2], 0.6, "applied color g")
assert_equal(colored_verts[1].color[3], 0.4, "applied color b")
-- Verify original unchanged
assert_equal(cube_verts[1].color[1], 1.0, "original vertices unchanged")

-- Test flip normals
local test_verts = {
    {position = {0, 0, 0}, normal = {0, 1, 0}, color = {1, 1, 1}, texcoord = {0, 0}},
    {position = {1, 0, 0}, normal = {1, 0, 0}, color = {1, 1, 1}, texcoord = {1, 0}},
}
framework.flip_normals(test_verts)
assert_equal(test_verts[1].normal[2], -1, "flipped normal y to -1")
assert_equal(test_verts[2].normal[1], -1, "flipped normal x to -1")

-- ============================================================================
-- Object Builder Tests
-- ============================================================================

print("Testing object builders...")

-- Test static object creation
local static_obj = framework.create_static_object(
    cube_verts,
    cube_indices,
    {5, 10, 15},
    {2, 3, 4},
    "test_shader",
    "test_object"
)

assert_not_nil(static_obj, "static object created")
assert_equal(static_obj.vertices, cube_verts, "static object has vertices")
assert_equal(static_obj.indices, cube_indices, "static object has indices")
assert_equal(static_obj.shader_keys[1], "test_shader", "static object has shader key")
assert_equal(static_obj.object_type, "test_object", "static object has type")
assert_equal(type(static_obj.compute_model_matrix), "function", "static object has matrix function")

local matrix = static_obj.compute_model_matrix()
assert_equal(type(matrix), "table", "compute_model_matrix returns table")

-- Test static cube creation
local cube_obj = framework.create_static_cube(
    {1, 2, 3},
    {0.5, 0.5, 0.5},
    {0.9, 0.1, 0.1},
    "cube_shader",
    "my_cube"
)

assert_not_nil(cube_obj, "static cube created")
assert_equal(cube_obj.object_type, "my_cube", "cube has correct type")
assert_equal(cube_obj.shader_keys[1], "cube_shader", "cube has correct shader")
assert_equal(cube_obj.vertices[1].color[1], 0.9, "cube has applied color")

-- Test dynamic object creation
local time_value = 0
local dynamic_obj = framework.create_dynamic_object(
    plane_verts,
    plane_indices,
    function(time)
        time_value = time
        return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}
    end,
    "dynamic_shader",
    "animated"
)

assert_not_nil(dynamic_obj, "dynamic object created")
assert_equal(dynamic_obj.object_type, "animated", "dynamic object has type")
local _ = dynamic_obj.compute_model_matrix(42)
assert_equal(time_value, 42, "dynamic compute function receives time")

-- ============================================================================
-- Material Registry Tests
-- ============================================================================

print("Testing material registry...")

local test_config = {
    materialx_materials = {
        {shader_key = "floor", document = "floor.mtlx", material = "Floor"},
        {shader_key = "wall", document = "wall.mtlx", material = "Wall"},
        {shader_key = "ceiling", document = "ceiling.mtlx", material = "Ceiling"},
    }
}

local registry = framework.MaterialRegistry.new(test_config)
assert_not_nil(registry, "material registry created")

assert_true(registry:has("floor"), "registry has floor material")
assert_true(registry:has("wall"), "registry has wall material")
assert_true(registry:has("ceiling"), "registry has ceiling material")
assert_equal(registry:has("nonexistent"), false, "registry doesn't have fake material")

local floor_mat = registry:get("floor")
assert_not_nil(floor_mat, "can get floor material")
assert_equal(floor_mat.shader_key, "floor", "floor material has correct key")
assert_equal(floor_mat.document, "floor.mtlx", "floor material has document")

assert_equal(registry:get_key("floor"), "floor", "get_key returns existing key")
assert_equal(registry:get_key("fake"), "floor", "get_key returns default for missing key")

-- Test empty registry
local empty_registry = framework.MaterialRegistry.new(nil)
assert_not_nil(empty_registry, "empty registry created")
assert_equal(empty_registry:has("anything"), false, "empty registry has nothing")

-- ============================================================================
-- Summary
-- ============================================================================

print("\n" .. string.rep("=", 60))
print(string.format("Tests run: %d", tests_run))
print(string.format("Passed: %d", tests_passed))
print(string.format("Failed: %d", tests_failed))
print(string.rep("=", 60))

if tests_failed == 0 then
    print("✓ ALL TESTS PASSED")
    os.exit(0)
else
    print("✗ SOME TESTS FAILED")
    os.exit(1)
end
