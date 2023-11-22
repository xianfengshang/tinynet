local pugixml = tinynet.pugixml
local content = [[
<xml>
   <appid>wx2421b1c4370ec43b</appid>
   <attach>支付测试</attach>
   <body>APP支付测试</body>
   <mch_id>10000100</mch_id>
   <nonce_str>1add1a30ac87aa2db72f57a2375d8fec</nonce_str>
   <notify_url>http://wxpay.wxutil.com/pub_v2/pay/notify.v2.php</notify_url>
   <out_trade_no>1415659990</out_trade_no>
   <spbill_create_ip>14.23.150.211</spbill_create_ip>
   <total_fee>1</total_fee>
   <trade_type>APP</trade_type>
   <sign>0CB01533B8C1EF103065174F50BCA001</sign>
</xml>
]]

local function totable(str)
    local doc = pugixml.new()
    doc:load_buffer(content)
    local root = doc:document_element()
    local obj = {}
    local child = root:first_child()
    while child do
        if child:type() == pugixml.xml_node_type.node_element then
            obj[child:name()] = child:child_value()
        end
        child = child:next_sibling()
    end
    --[[
    obj.appid = root:child("appid"):child_value()
    obj.attach = root:child("attach"):child_value()
    obj.mch_id = root:child("mch_id"):child_value()
    obj.nonce_str = root:child("nonce_str"):child_value()
    obj.notify_url = root:child("notify_url"):child_value()
    obj.out_trade_no = root:child("out_trade_no"):child_value()
    obj.spbill_create_ip = root:child("spbill_create_ip"):child_value()
    obj.total_fee = root:child("total_fee"):child_value()
    obj.trade_type = root:child("trade_type"):child_value()
    obj.sign = root:child("sign"):child_value()
    --]]
    return obj
end

local function toxml(obj)
    local doc = pugixml.new()
    local xml_node = doc:as_node():append_child("xml")
    --[[for k, v in pairs(obj) do
        xml_node:append_child(k):append_child(pugixml.xml_node_type.node_pcdata):set_value(v) 
    end--]]
    xml_node:append_child("appid"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.appid)
    xml_node:append_child("attach"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.attach)
    xml_node:append_child("mch_id"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.mch_id)
    xml_node:append_child("nonce_str"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.nonce_str)
    xml_node:append_child("notify_url"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.notify_url)
    xml_node:append_child("out_trade_no"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.out_trade_no)
    xml_node:append_child("spbill_create_ip"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.spbill_create_ip)
    xml_node:append_child("total_fee"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.total_fee)
    xml_node:append_child("trade_type"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.trade_type)
    xml_node:append_child("sign"):append_child(pugixml.xml_node_type.node_pcdata):set_value(obj.sign)
    return doc:save()
end

local obj = totable(content)
print(obj.appid)
local str = toxml(obj)
print(str)

local gevent = require("tinynet/core/gevent")
local Socket = require("tinynet/core/tcp_socket")

local server

local function handle_client(session)
    log.info("accept new client")
    while true do
        local ok, data = session:receive(1024)
        if ok then
            log.info("Session receive:%s", tostring(data))
            --session:send(data .. '\n')
            --session:close()
        else
            log.error(tostring(data))
            break
        end
    end
end

local function start_server()
    log.info("start server")
    server = Socket.new()
    local result = server:listen(8080, "0.0.0.0")
    if not result then
        log.info("listen failed")
    end
    while true do
        local ok, session = server:accept()
        if ok then
            gevent.spawn(handle_client, session)
        else
            log.error(session)
        end
    end
end

local client
local function start_client()
    log.info("start client")
    client = Socket.new()
    local ok, err = client:connect("127.0.0.1", 8080)
    if not ok then
        log.info("Connect to server failed, err:%s", err)
        return
    end
    log.info("Connect to server success")
    client:send("HelloWorld")
    --local ok, data = client:receive("*a")
    --log.info("Client receive:%s", data)
    client:close()
end

gevent.spawn(start_server)
gevent.spawnlater(1000, start_client)
