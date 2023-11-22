local add = bit.lshift(1, 1)
log.warning("add=%s", add)
local remove = bit.lshift(1, 2)
local add1 = bit.lshift(1, 3)
log.warning("add1=%s", add1)
local remove1 = bit.lshift(1, 4)

local flag = bit.bor(add, add1)
log.warning("flag=%s", flag)

local ret = bit.band(flag, add)
log.warning("ret=%s", ret)
ret = bit.rshift(ret, 1)
log.warning("ret=%s", ret)

assert(ret == 1)

local BitUtil = require("tinynet/util/bit_util")

local flag = 8
flag =  BitUtil.set(flag, 1)

assert(flag == 10)

assert(BitUtil.test(flag, 1)==true)
assert(BitUtil.test(flag, 3)==true)

flag = BitUtil.clear(flag, 1)

assert(flag == 8)

assert(BitUtil.test(flag, 1)==false)
assert(BitUtil.test(flag, 3)==true)