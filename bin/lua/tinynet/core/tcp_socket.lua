local class = require("tinynet/core/class")
local Event = require("tinynet/core/event")
local table_concat = table.concat

---@class TcpSocket: Object
local TcpSocket = class("TcpSocket")

local INPUT_FILTER_TYPE_UNSPEC = 0
local INPUT_FILTER_TYPE_LINE = 1
local INPUT_FILTER_TYPE_ALL = 2
local INPUT_FILTER_TYPE_LENGTH = 3

function TcpSocket:constructor(sock)
    self.sock = sock or tinynet.socket.tcp.new()
    self.sock:on_event(function(evt) self:on_event(evt) end)

    self.read_evt = Event.new() ---@type Event
    self.write_evt = Event.new() ---@type Event
    self.data_queue = {}
    self.data_buffer = ""
    self.last_error = nil
    self.block_timeout = -1
    self.total_timeout = -1
    self.input_filter = INPUT_FILTER_TYPE_UNSPEC
    self.input_length = 0
    self.input_timeout = false
end

function TcpSocket:connect(host, port)
    if self:is_connected() then
        return 1
    end
    if self.write_evt:is_wait() then
        return nil, "Operation in progress"
    end
    local err = self.sock:connect(host, port)
    if err ~= nil then
        return nil, string.format("Connect error:%s", err)
    end
    err = self.write_evt:wait(self:gettimeout())
    if not self.write_evt:is_set() then
        return nil, "timeout"
    end
    self.write_evt:reset()
    if err ~= nil then
        return nil, string.format("Connect error:%s", err)
    end
    return 1
end

function TcpSocket:send(data, i, j)
    if self.write_evt:is_wait() then
        return nil, "Operation in progress"
    end
    if not self:is_connected() then
        return nil, "closed"
    end
    if i and i ~= 1 then
        data = string.gsub(data, i, j)
    end
    local err  = self.sock:send(data)
    if err ~= nil then
        return nil,  err
    end
    return j or #data
end

function TcpSocket:receive(opt)
    if self.read_evt:is_wait() then
        return nil, "Operation in progress"
    end
    if not self:is_connected() then
        return nil, "closed"
    end
    local opt_type = type(opt)
    if opt_type == "nil" then
        --read line
        self.input_filter = INPUT_FILTER_TYPE_LINE
    elseif opt_type == "number" then
        self.input_filter = INPUT_FILTER_TYPE_LENGTH
        self.input_length = opt
    elseif opt_type == "string" then
        if opt == "*a" then
            --read all
            self.input_filter = INPUT_FILTER_TYPE_ALL
        elseif opt == "*l" then
            --read line
            self.input_filter = INPUT_FILTER_TYPE_LINE
        else
            return nil, "Invalid argument"
        end
    end
    local data, err
    data = self:read_data()
    if data then
        return data
    end
    data, err = self.read_evt:wait(self:gettimeout())
    if not self.read_evt:is_set() then
        return nil, "timeout"
    end
    self.read_evt:reset()
    self.input_filter = INPUT_FILTER_TYPE_UNSPEC
    if err ~= nil then
        return data
    end
    return nil, err
end

function TcpSocket:listen(...)
    local err = self.sock:listen(...)
    if err ~= nil then
        return nil, err
    end
    return 1
end

function TcpSocket:accept()
    if self.read_evt:is_wait() then
        return nil, "Operation in progress"
    end
    if #self.data_queue > 0 then
        return table.remove(self.data_queue, 1)
    end
    local sock, err = self.read_evt:wait(self:gettimeout())
    if not self.read_evt:is_set() then
        return nil, "timeout"
    end
    self.read_evt:reset()
    if err ~= nil then
        return nil, err
    end
    return sock
end

function TcpSocket:close()
    if self:is_closed() then
        return
    end
    self.sock:close()
end

function TcpSocket:settimeout(timeout, opt)
    if opt == nil then
        self.block_timeout = timeout
    elseif opt == "b" then
        self.block_timeout = timeout
    elseif opt == "r" then
        self.total_timeout = timeout
    elseif opt == "t" then
        self.total_timeout = timeout
    end
end

function TcpSocket:gettimeout()
    local block_timeout = self.block_timeout
    if block_timeout > 0 then
        self.block_timeout = -1
    end
    local timeout = math.min(self.block_timeout, self.total_timeout)
    if timeout < 0 then
        timeout = 2^32
    end
    return timeout
end

function TcpSocket:on_event(evt)
    local handler = self[evt.type]
    if not handler then
        return
    end
    handler(self, evt)
end

function TcpSocket:onaccept(evt)
    local sock = TcpSocket.new(evt.data)
    if self.read_evt:is_wait() then
        return self.read_evt:set(sock)
    end
    table.insert(self.data_queue, sock)
end

function TcpSocket:onopen(evt)
    if self.write_evt:is_wait() then
        self.write_evt:set()
    end
end

function TcpSocket:onread(evt)
    self.data_buffer = self.data_buffer .. evt.data
    if self.read_evt:is_wait() then
        local data = self:read_data()
        if data then
            return self.read_evt:set(data)
        end
    end
end

function TcpSocket:onwrite(evt)
end

function TcpSocket:onerror(evt)
    if self.read_evt:is_wait() then
        if self.input_filter == INPUT_FILTER_TYPE_ALL then
            self.read_evt:set(self:read_all())
        else
            self.read_evt:set(nil, evt.data)
        end
    end
    if self.write_evt:is_wait() then
        self.write_evt:set(nil, evt.data)
    end
end

function TcpSocket:onclose(evt)
    if self.read_evt:is_wait() then
        if self.input_filter ~= INPUT_FILTER_TYPE_UNSPEC then
            return self.read_evt:set(self:read_data())
        end
        return self.read_evt:set(nil, self.last_error)
    end
end

function TcpSocket:read_data()
    if self.input_filter == INPUT_FILTER_TYPE_LINE then
        local i, j = string.find(self.data_buffer, "\r*\n+")
        if i and j then
            local data = string.sub(self.data_buffer, 1, i - 1)
            self.data_buffer = string.sub(self.data_buffer, j+1) or ""
            return data
        end
    elseif self.input_filter == INPUT_FILTER_TYPE_ALL then
    elseif self.input_filter == INPUT_FILTER_TYPE_LENGTH then
        if #self.data_buffer >= self.input_length then
            local data = string.sub(self.data_buffer, 1, self.input_length)
            self.data_buffer = string.sub(self.data_buffer, self.input_length + 1) or ""
            return data
        end
    end
end

function TcpSocket:read_all()
    local data = self.data_buffer
    self.data_buffer = ""
    return data
end

function TcpSocket:is_closed()
    local status = self.sock:get_status()
    return status == "closed"
end

function TcpSocket:is_connected()
    return self.sock:get_status() == "connected"
end

return TcpSocket
