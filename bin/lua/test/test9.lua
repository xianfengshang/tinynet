local address = "ws://127.0.0.1:8080"
local task_queue = {}
local clients = {}
local function run_task()
    local client = table.remove(task_queue)
    if client then
        client:open()
    end
end
local function init_client(client)
    client:on_event(function(evt) 
        if evt.type == "onopen" then
            local currentTime = time() * 1000
            client:send_msg(tostring(currentTime))
            if #task_queue > 0 then
                run_task()
            end
        elseif evt.type == "onclose" then
            sleep(0.005)
            client:open()
        elseif evt.type == "onmessage" then
            local currentTime = time() * 1000
            client:send_msg(tostring(currentTime))
        end
    end)
end
local client_count = 3000
for i = 1, client_count do
    clients[i] = tinynet.ws.client.new(address)
    task_queue[i] = clients[i]
    init_client(clients[i])
end

for i = 1, 100 do
    run_task()
end
