require("UnityEngine/init")

local a = Vector3.New(10, 10)
local b = Vector3.New(11, 10)

log.warning(Vector3.Distance(a, b))
log.warning(Vector3.Distance(b, a))