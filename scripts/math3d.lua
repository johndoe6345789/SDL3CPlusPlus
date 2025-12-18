local math3d = {}

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
    return identity_matrix()
end

function math3d.multiply(a, b)
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

function math3d.translation(x, y, z)
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        x,   y,   z,   1.0,
    }
end

function math3d.rotation_x(radians)
    local c = math.cos(radians)
    local s = math.sin(radians)
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0,  c,  s, 0.0,
        0.0, -s,  c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

function math3d.rotation_y(radians)
    local c = math.cos(radians)
    local s = math.sin(radians)
    return {
        c,  0.0, -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        s,  0.0,  c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

function math3d.look_at(eye, center, up)
    local f = normalize({center[1] - eye[1], center[2] - eye[2], center[3] - eye[3]})
    local s = normalize(cross(f, up))
    local u = cross(s, f)

    local result = identity_matrix()
    result[1] = s[1]
    result[2] = u[1]
    result[3] = -f[1]
    result[5] = s[2]
    result[6] = u[2]
    result[7] = -f[2]
    result[9] = s[3]
    result[10] = u[3]
    result[11] = -f[3]
    result[13] = -dot(s, eye)
    result[14] = -dot(u, eye)
    result[15] = dot(f, eye)
    return result
end

function math3d.perspective(fov, aspect, zNear, zFar)
    local tanHalf = math.tan(fov / 2.0)
    local result = {
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
    }
    result[1] = 1.0 / (aspect * tanHalf)
    result[6] = -1.0 / tanHalf
    result[11] = zFar / (zNear - zFar)
    result[12] = -1.0
    result[15] = (zNear * zFar) / (zNear - zFar)
    return result
end

return math3d
