local json = require("tinynet/web/json")

local reqData = {}
reqData.name = "ĳһһ"
reqData.idNum = "110000190101010001"
reqData.aid = md5_sum("100000000000000001")

local content = json.encode(reqData)
log.warning(content)
local key = hex2bin("f0145b4718d56104fa755c70277e0a63")
log.warning("key=%s", bin2hex(key))
local iv_len = openssl_cipher_iv_length("aes-128-gcm")
log.warning("iv_len=%d", iv_len)
local iv = openssl_random_pseudo_bytes(iv_len)
log.warning("iv=%s", bin2hex(iv))
local encrypted_data, tag = openssl_encrypt(content, "aes-128-gcm", key, iv)
log.warning("encrypted_data=%s, tag=%s", bin2hex(encrypted_data), bin2hex(tag))
local base = base64.encode(iv .. encrypted_data .. tag)
log.warning("base=%s, len=%s", base, #base)

local decrypted_data = openssl_decrypt(encrypted_data, "aes-128-gcm", key, iv, nil, tag)
log.warning("decrypted_data=%s, len=%s", decrypted_data, #decrypted_data)

local data = "phy3crbq31uep/bYq7aFrmXaPSVyQ0s7i1dv6youMwXC4QmHs01naw3pZdcFK4i5zSZco+xgbI1r/7kggwxTIVVl3cHgmhstZEvgzyE46I0aOqbR4H3wCveWeAlN97MXnGx3xznq+Q=="
local bin_data = base64.decode(data)
log.warning("bin_data=%s, len=%s", bin2hex(bin_data), #bin_data)
local iv1 = string.sub(bin_data, 1, 12)
log.warning("iv1 len=%s data=%s", #iv1, bin2hex(iv1))
local tag1 = string.sub(bin_data, -16)
log.warning("tag1 len=%s data=%s", #tag1, bin2hex(tag1))
local content1 = string.sub(bin_data, 13, -17)
log.warning("content1 len=%s data=%s", #content1, bin2hex(content1))

local decrypted_data = openssl_decrypt(content1, "aes-128-gcm", key, iv1, nil, tag1)
log.warning("decrypted_data=%s, len=%s", decrypted_data, #decrypted_data)

local encrypted_data, tag = openssl_encrypt(decrypted_data, "aes-128-gcm", key, iv1)
high_resolution_time()
log.warning("encrypted_data=%s, tag=%s", bin2hex(encrypted_data), bin2hex(tag))
local base = base64.encode(iv1 .. encrypted_data .. tag)
log.warning("base=%s, len=%s", base, #base)

if base == data then
    log.warning("base ok")
else
    log.warning("base no")
end

local final_string = 'f0145b4718d56104fa755c70277e0a63appId7bec5932ac504fc99daf04a3fbe4c762bizId1101999999timestamps1617096205229{"data":"GcEGhAFPyjYlz3i8bffbZji10RzFF7VVL1mrae/auCFz1dKt6N2H0U49+ZbkcrGByepKVR3UqNOm2BNwk+/f78g9i5+PqmYV7uZYBVCGCCzJk57tG/1Z9TO0SgwABf7LIno/ECot8w=="}'

local sign = string.lower(sha256_sum(final_string))
log.warning("sign=%s", sign)
local signData = "4e70b9b4417a1ebf131d50c8907fdec6308780f5f03b6b2af56eec3acb951e92"
if sign == signData then
    log.warning("sign ok")
else
    log.warning("sign no")
end

local data1 = [[f0145b4718d56104fa755c70277e0a63appId7bec5932ac504fc99daf04a3fbe4c762bizId1101999999timestamps1617096205229{"data":"GcEGhAFPyjYlz3i8bffbZji10RzFF7VVL1mrae/auCFz1dKt6N2H0U49+ZbkcrGByepKVR3UqNOm2BNwk+/f78g9i5+PqmYV7uZYBVCGCCzJk57tG/1Z9TO0SgwABf7LIno/ECot8w=="}]]
local sum = sha256_sum(data1)
log.warning(string.lower(sum))
