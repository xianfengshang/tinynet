local gevent = require("tinynet/core/gevent")
local TcpSocket = require("tinynet/core/tcp_socket")


local server_sock

local function handle_client(sock)
    while true do
        local data, err = sock:receive()
        if not data then
            log.warning(err)
            return
        end
        sock:send("+PONG\r\n")
    end
end

local function test_tcp_server()
    server_sock = TcpSocket.new()
    local ret, err = server_sock:listen(6379)
    if err ~= nil  then
        log.warning(err)
        return
    end
    while true do
        local sock, err = server_sock:accept()
        if sock then 
            gevent.spawn(handle_client, sock)
        else
            log.warning(err)
        end
    end
end

gevent.pspawn(test_tcp_server)
