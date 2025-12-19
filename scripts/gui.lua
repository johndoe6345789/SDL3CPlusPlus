-- Lightweight Lua-based 2D GUI framework that emits draw commands
-- and handles interaction for buttons, textboxes, and list views.
local Gui = {}

-- {r,g,b,a} colors
local DEFAULT_STYLE = {
    spacing = 8,
    radius = 6,
    fontSize = 16,
    button = {
        background = {0.2, 0.24, 0.28, 1.0},
        hovered = {0.26, 0.3, 0.36, 1.0},
        active = {0.16, 0.22, 0.28, 1.0},
        border = {0.45, 0.52, 0.6, 1.0},
        text = {1.0, 1.0, 1.0, 1.0},
    },
    textbox = {
        background = {0.05, 0.06, 0.08, 1.0},
        border = {0.38, 0.4, 0.45, 1.0},
        focusedBorder = {0.35, 0.55, 0.9, 1.0},
        text = {0.97, 0.97, 0.97, 1.0},
        placeholder = {0.5, 0.5, 0.5, 1.0},
        cursor = {0.98, 0.98, 0.98, 1.0},
    },
    listview = {
        background = {0.04, 0.05, 0.07, 1.0},
        border = {0.25, 0.28, 0.32, 1.0},
        alternate = {0.06, 0.07, 0.09, 1.0},
        selection = {0.12, 0.35, 0.65, 1.0},
        text = {0.93, 0.94, 0.96, 1.0},
    },
}

local function clamp(value, minValue, maxValue)
    if minValue and value < minValue then
        return minValue
    end
    if maxValue and value > maxValue then
        return maxValue
    end
    return value
end

local function rectContains(rect, x, y)
    return x >= rect.x and x <= rect.x + rect.width and y >= rect.y and y <= rect.y + rect.height
end

local InputState = {}
InputState.__index = InputState

function InputState:new()
    local instance = {
        mouseX = 0,
        mouseY = 0,
        mouseDown = false,
        mouseDownPrevious = false,
        wheel = 0,
        textInput = "",
        keys = {},
    }
    return setmetatable(instance, self)
end

function InputState:setMouse(x, y, isDown)
    self.mouseDownPrevious = self.mouseDown
    self.mouseX = x
    self.mouseY = y
    self.mouseDown = isDown
end

function InputState:setWheel(deltaY)
    self.wheel = deltaY
end

function InputState:mouseJustPressed()
    return self.mouseDown and not self.mouseDownPrevious
end

function InputState:mouseJustReleased()
    return not self.mouseDown and self.mouseDownPrevious
end

function InputState:setKey(keyName, isDown)
    self.keys[keyName] = isDown
end

function InputState:isKeyDown(keyName)
    return self.keys[keyName]
end

function InputState:addTextInput(text)
    self.textInput = (self.textInput or "") .. (text or "")
end

function InputState:resetTransient()
    self.textInput = ""
    self.wheel = 0
end

local Context = {}
Context.__index = Context

function Context:new(options)
    options = options or {}
    local style = options.style or DEFAULT_STYLE
    local instance = {
        commands = {},
        input = nil,
        hotWidget = nil,
        activeWidget = nil,
        focusWidget = nil,
        nextFocus = nil,
        style = style,
        mousePressed = false,
        mouseReleased = false,
    }
    return setmetatable(instance, self)
end

function Context:beginFrame(input)
    if not input then
        error("Context requires an InputState for each frame")
    end
    self.input = input
    self.commands = {}
    self.hotWidget = nil
    self.nextFocus = nil
    self.mousePressed = input:mouseJustPressed()
    self.mouseReleased = input:mouseJustReleased()
end

function Context:endFrame()
    if self.mouseReleased then
        self.activeWidget = nil
    end
    if self.nextFocus ~= nil then
        self.focusWidget = self.nextFocus
    elseif self.mousePressed and not self.hotWidget then
        self.focusWidget = nil
    end
end

function Context:requestFocus(widgetId)
    self.nextFocus = widgetId
end

function Context:addCommand(command)
    table.insert(self.commands, command)
end

function Context:getCommands()
    return self.commands
end

function Context:isMouseOver(rect)
    return rectContains(rect, self.input.mouseX, self.input.mouseY)
end

function Context:markHot(widgetId, hovered)
    if hovered then
        self.hotWidget = widgetId
    end
end

function Context:pushRect(rect, params)
    params = params or {}
    local command = {
        type = "rect",
        x = rect.x,
        y = rect.y,
        width = rect.width,
        height = rect.height,
        color = params.color,
        borderColor = params.borderColor,
        borderWidth = params.borderWidth or (params.borderColor and 1 or 0),
        radius = params.radius or self.style.radius,
    }
    self:addCommand(command)
end

function Context:pushText(rect, params)
    params = params or {}
    local padding = params.padding or self.style.spacing / 2
    local alignX = params.alignX or "left"
    local alignY = params.alignY or "center"
    local x = rect.x + padding
    local y = rect.y
    if alignX == "center" then
        x = rect.x + rect.width / 2
    elseif alignX == "right" then
        x = rect.x + rect.width - padding
    end
    if alignY == "center" then
        y = rect.y + rect.height / 2
    elseif alignY == "bottom" then
        y = rect.y + rect.height - padding
    else
        y = rect.y + padding
    end
    local command = {
        type = "text",
        text = params.text or "",
        x = x,
        y = y,
        color = params.color,
        fontSize = params.fontSize or self.style.fontSize,
        alignX = alignX,
        alignY = alignY,
        bounds = {x = rect.x, y = rect.y, width = rect.width, height = rect.height},
    }
    if params.clip then
        command.clipRect = {
            x = rect.x,
            y = rect.y,
            width = rect.width,
            height = rect.height,
        }
    end
    self:addCommand(command)
end

function Context:pushSvg(svgPath, rect, opts)
    opts = opts or {}
    local command = {
        type = "svg",
        path = svgPath,
        x = rect.x,
        y = rect.y,
        width = rect.width,
        height = rect.height,
        color = opts.color,
        tint = opts.tint,
    }
    self:addCommand(command)
end

function Context:pushClip(rect)
    self:addCommand({type = "clip_push", rect = rect})
end

function Context:popClip()
    self:addCommand({type = "clip_pop"})
end

function Context:processInteraction(widgetId, rect)
    local hovered = self:isMouseOver(rect)
    self:markHot(widgetId, hovered)
    if hovered and self.mousePressed then
        self.activeWidget = widgetId
    end
    local clicked = hovered and self.mouseReleased and self.activeWidget == widgetId
    return hovered, clicked
end

local function ensureRect(rect)
    return {
        x = rect.x or rect[1] or 0,
        y = rect.y or rect[2] or 0,
        width = rect.width or rect[3] or 0,
        height = rect.height or rect[4] or 0,
    }
end

function Gui.newContext(options)
    return Context:new(options)
end

function Gui.newInputState()
    return InputState:new()
end

function Gui.button(context, widgetId, rectDef, label, opts)
    opts = opts or {}
    local rect = ensureRect(rectDef)
    local style = context.style.button
    local hovered, clicked = context:processInteraction(widgetId, rect)
    local active = context.activeWidget == widgetId
    local fillColor = style.background
    if active then
        fillColor = style.active
    elseif hovered then
        fillColor = style.hovered
    end
    context:pushRect(rect, {
        color = fillColor,
        borderColor = style.border,
        radius = opts.radius,
    })
    context:pushText(rect, {
        text = label or "",
        color = style.text,
        alignX = "center",
        alignY = "center",
        fontSize = opts.fontSize,
    })
    if clicked and opts.onClick then
        opts.onClick()
    end
    return clicked
end

function Gui.text(context, rectDef, text, opts)
    opts = opts or {}
    local rect = ensureRect(rectDef)
    context:pushText(rect, {
        text = text or "",
        color = opts.color,
        alignX = opts.alignX,
        alignY = opts.alignY,
        padding = opts.padding,
        fontSize = opts.fontSize,
        clip = opts.clip,
    })
end

function Gui.svg(context, rectDef, svgPath, opts)
    opts = opts or {}
    local rect = ensureRect(rectDef)
    context:pushSvg(svgPath, rect, opts)
end

function Gui.textbox(context, widgetId, rectDef, state, opts)
    opts = opts or {}
    state = state or {}
    state.text = state.text or ""
    state.cursor = clamp(state.cursor or #state.text, 0, #state.text)
    state.offset = clamp(state.offset or 0, 0, math.max(0, #state.text))
    local rect = ensureRect(rectDef)
    local style = context.style.textbox
    local hovered, clicked = context:processInteraction(widgetId, rect)
    local focused = context.focusWidget == widgetId
    if clicked then
        context:requestFocus(widgetId)
    end
    local background = style.background
    local borderColor = focused and style.focusedBorder or style.border
    context:pushRect(rect, {
        color = background,
        borderColor = borderColor,
    })
    local pad = opts.padding or context.style.spacing / 2
    local charWidth = opts.charWidth or (context.style.fontSize * 0.55)
    local innerWidth = math.max(0, rect.width - pad * 2)
    local maxChars = math.max(1, math.floor(innerWidth / charWidth))
    if focused then
        local function deleteRange(startIdx, count)
            if startIdx < 1 or startIdx > #state.text then
                return
            end
            local before = state.text:sub(1, startIdx - 1)
            local after = state.text:sub(startIdx + count)
            state.text = before .. after
        end
        local textInput = context.input.textInput or ""
        if textInput ~= "" then
            local before = state.text:sub(1, state.cursor)
            local after = state.text:sub(state.cursor + 1)
            state.text = before .. textInput .. after
            state.cursor = state.cursor + #textInput
            context.input.textInput = ""
        end
        if context.input:isKeyDown("backspace") and state.cursor > 0 then
            deleteRange(state.cursor, 1)
            state.cursor = state.cursor - 1
            context.input.keys.backspace = false
        end
        if context.input:isKeyDown("delete") and state.cursor < #state.text then
            deleteRange(state.cursor + 1, 1)
            context.input.keys.delete = false
        end
        if context.input:isKeyDown("left") and state.cursor > 0 then
            state.cursor = state.cursor - 1
            context.input.keys.left = false
        end
        if context.input:isKeyDown("right") and state.cursor < #state.text then
            state.cursor = state.cursor + 1
            context.input.keys.right = false
        end
        if context.input:isKeyDown("home") then
            state.cursor = 0
            context.input.keys.home = false
        end
        if context.input:isKeyDown("end") then
            state.cursor = #state.text
            context.input.keys.end = false
        end
        if context.input:isKeyDown("enter") then
            if opts.onSubmit then
                opts.onSubmit(state.text)
            end
            context.input.keys.enter = false
        end
    end
    state.cursor = clamp(state.cursor, 0, #state.text)
    if state.cursor < state.offset then
        state.offset = state.cursor
    end
    if state.cursor > state.offset + maxChars then
        state.offset = state.cursor - maxChars
    end
    local offset = clamp(state.offset, 0, math.max(0, #state.text - maxChars))
    state.offset = offset
    local visibleText = state.text:sub(offset + 1, offset + maxChars)
    local display = visibleText
    if display == "" and opts.placeholder and not focused then
        context:pushText(rect, {
            text = opts.placeholder,
            color = style.placeholder,
            alignX = "left",
            alignY = "center",
            padding = pad,
        })
    else
        context:pushText(rect, {
            text = display,
            color = style.text,
            alignX = "left",
            alignY = "center",
            padding = pad,
            clip = true,
        })
    end
    if focused then
        context:pushRect({
            x = rect.x + pad + (state.cursor - offset) * charWidth - 1,
            y = rect.y + pad / 2,
            width = opts.caretWidth or 2,
            height = rect.height - pad,
        }, {
            color = style.cursor,
        })
    end
    return state
end

function Gui.listView(context, widgetId, rectDef, items, state, opts)
    opts = opts or {}
    state = state or {}
    items = items or {}
    state.scroll = state.scroll or 0
    local itemCount = #items
    if itemCount == 0 then
        state.selectedIndex = 0
    else
        state.selectedIndex = clamp(state.selectedIndex or 1, 1, itemCount)
    end
    local rect = ensureRect(rectDef)
    local style = context.style.listview
    local hovered, clicked = context:processInteraction(widgetId, rect)
    local focus = context.focusWidget == widgetId
    if clicked then
        context:requestFocus(widgetId)
    end
    context:pushRect(rect, {
        color = style.background,
        borderColor = style.border,
    })
    local itemHeight = opts.itemHeight or (context.style.fontSize + context.style.spacing)
    local contentHeight = #items * itemHeight
    state.scroll = clamp(state.scroll, 0, math.max(0, contentHeight - rect.height))
    if state.selectedIndex > 0 and opts.scrollToSelection then
        local scrollTarget = (state.selectedIndex - 1) * itemHeight
        if scrollTarget < state.scroll then
            state.scroll = scrollTarget
        elseif scrollTarget + itemHeight > state.scroll + rect.height then
            state.scroll = scrollTarget + itemHeight - rect.height
        end
    end
    local scrollDelta = context.input.wheel or 0
    if (hovered or focus) and scrollDelta ~= 0 then
        state.scroll = clamp(state.scroll - scrollDelta * (opts.scrollSpeed or 20), 0, math.max(0, contentHeight - rect.height))
    end
    context:pushClip(rect)
    local baseY = rect.y - state.scroll
    for index, item in ipairs(items) do
        local rowY = baseY + (index - 1) * itemHeight
        if rowY + itemHeight >= rect.y and rowY <= rect.y + rect.height then
            local rowRect = {x = rect.x, y = rowY, width = rect.width, height = itemHeight}
            local rowColor = style.alternate
            if index % 2 == 0 then
                rowColor = style.background
            end
            if index == state.selectedIndex then
                rowColor = style.selection
            end
            context:pushRect(rowRect, {color = rowColor})
            context:pushText(rowRect, {
                text = (opts.itemFormatter and opts.itemFormatter(item, index)) or tostring(item),
                color = style.text,
                alignX = "left",
                alignY = "center",
                padding = context.style.spacing,
                clip = true,
            })
        end
    end
    context:popClip()
    if clicked then
        local relativeY = context.input.mouseY - rect.y + state.scroll
        local clickedIndex = math.floor(relativeY / itemHeight) + 1
        if clickedIndex >= 1 and clickedIndex <= #items then
            state.selectedIndex = clickedIndex
            if opts.onSelect then
                opts.onSelect(clickedIndex, items[clickedIndex])
            end
        end
    end
    if focus then
        if context.input:isKeyDown("up") and state.selectedIndex > 1 then
            state.selectedIndex = state.selectedIndex - 1
        end
        if context.input:isKeyDown("down") and state.selectedIndex < #items then
            state.selectedIndex = state.selectedIndex + 1
        end
    end
    return state
end

function Gui.newTextState(initial)
    return {
        text = initial or "",
        cursor = #initial,
        offset = 0,
    }
end

function Gui.newListState()
    return {
        scroll = 0,
        selectedIndex = 1,
    }
end

Gui.style = DEFAULT_STYLE
return Gui
