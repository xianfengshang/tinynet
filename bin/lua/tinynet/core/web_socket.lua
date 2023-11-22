local class = require("tinynet/core/class")

local WebSocket = class("WebSocket")

function WebSocket:constructor(websocket_or_opts)
    --websocket
    if type(websocket_or_opts) == "userdata" then
        self.ws = websocket_or_opts
    else
        self.ws = tinynet.ws.client.new(websocket_or_opts)
    end
    self.ws:on_event(function (evt) self:on_event(evt) end)

    --events
    self.onopen = nil

    self.onclose = nil

    self.onmessage = nil

    self.onerror = nil

    assert(self.ws, "Cant not create WebSocket")
end

function WebSocket:open()
    self.ws:open()
end

function WebSocket:close()
    self.ws:close()
end

function WebSocket:send_bytes(msg)
    return self.ws:send_bytes(msg)
end

function WebSocket:send_text(data)
    return self.ws:send_text(data)
end

function WebSocket:is_closed()
    return self.ws:is_closed()
end

function WebSocket:is_connected()
    return self.ws:is_connected()
end

function WebSocket:is_connecting()
    return self.ws:is_connecting()
end

function WebSocket:get_peer_ip()
    return self.ws:get_peer_ip()
end

function WebSocket:on_event(evt)
    local handler = self[evt.type]
    if handler then
        handler(evt)
    end
end

return WebSocket
