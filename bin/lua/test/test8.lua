local gevent = require("tinynet/core/gevent") 
local server
local function dispatcher(evt)
    if evt.type == "onopen" then
        log.warning("Connection count:%s", server:get_session_size())
    elseif evt.type == "onmessage" then
        server:send_msg(evt.guid, evt.data)
    elseif evt.type == "onclose" then
    end
end
local address = "ws://*:8080"
local function start_server()
    server = tinynet.ws.server.new()
    local err = server:start(address, dispatcher)
    if err ~= nil then
        log.error("Start server faild, err:%s", err)
        return
    end
    log.warning("Server listen on %s success", address)
end

gevent.pspawn(start_server)
