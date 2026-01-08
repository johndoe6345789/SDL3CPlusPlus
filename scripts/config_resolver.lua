local resolver = {}

local function as_table(value)
    if type(value) == "table" then
        return value
    end
    return nil
end

function resolver.resolve_materialx(config)
    local root = as_table(config)
    if not root then
        return nil
    end
    local rendering = as_table(root.rendering)
    if rendering then
        local materialx = as_table(rendering.materialx)
        if materialx then
            return materialx
        end
    end
    return as_table(root.materialx)
end

function resolver.resolve_materialx_materials(config)
    local materialx = resolver.resolve_materialx(config)
    if materialx then
        local materials = as_table(materialx.materials)
        if materials then
            return materials
        end
    end
    local root = as_table(config)
    if root then
        return as_table(root.materialx_materials)
    end
    return nil
end

function resolver.resolve_input_bindings(config)
    local root = as_table(config)
    if not root then
        return nil
    end
    local input = as_table(root.input)
    if input then
        local bindings = as_table(input.bindings)
        if bindings then
            return bindings
        end
    end
    return as_table(root.input_bindings)
end

function resolver.resolve_gui_font(config)
    local root = as_table(config)
    if not root then
        return nil
    end
    local gui = as_table(root.gui)
    if gui then
        local font = as_table(gui.font)
        if font then
            return font
        end
    end
    return as_table(root.gui_font)
end

function resolver.resolve_gui_opacity(config)
    local root = as_table(config)
    if not root then
        return nil
    end
    local gui = as_table(root.gui)
    if gui and type(gui.opacity) == "number" then
        return gui.opacity
    end
    if type(root.gui_opacity) == "number" then
        return root.gui_opacity
    end
    return nil
end

function resolver.resolve_scene(config)
    local root = as_table(config)
    if not root then
        return nil
    end
    return as_table(root.scene)
end

return resolver
