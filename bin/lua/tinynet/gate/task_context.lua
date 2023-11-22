local class = require("tinynet/core/class")
local Message = require("tinynet/message")

local ErrorCode = {
    ERROR_OK = 0,

    ERROR_INTERNAL_SERVER_ERROR = 3 -- //Internal Server Error
}

---@class TaskContext:Object
local TaskContext = class("TaskContext")

function TaskContext:constructor(gate, session, raw_req)
    self.gate = gate
    self.session = session
    self.raw_request = raw_req
    ---@type Message
    self.request = Message.new()
    ---@type Message
    self.response = nil
    self.create_time = high_resolution_time()
    self.addr = ""
    self.route_table = nil
    if session then
        self.addr = session.addr
    end
end

function TaskContext:run()
    if self.gate then
        self.gate:run(self)
    end
end

function TaskContext:OnError()
    if self.response == nil then
        local type_info = Message.TypeMeta[self.request.type]
        if type_info ~= nil and not type_info.oneway then
            self.response = Message.new()
            self.response.seq = self.request.seq
            self.response.route = self.request.route
            self.response.type = type_info.ack
        end
    end
    if self.response ~= nil then
        self.response.body = {}
        self.response.body.errorCode = ErrorCode.ERROR_INTERNAL_SERVER_ERROR
        self.response.body.err = "Internal server error"
    end
end

return TaskContext
