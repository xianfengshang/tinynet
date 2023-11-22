local WebUtil = require("tinynet/web/web_util")
local WebRouter = require("tinynet/web/web_router")

local StatusCode =
{
    NOT_FOUND = 404
}

local exports = WebRouter.new({name="index"})

exports.get["/.*%.png$"] = WebUtil.send_file

exports.get["/.*%.jpg$"] = WebUtil.send_file

exports.get["/.*%.jpeg$"] = WebUtil.send_file

exports.get["/.*%.ico$"] = WebUtil.send_file

exports.get["/.*%.html$"] = WebUtil.send_file

exports.get["/.*%.js$"] = WebUtil.send_file

exports.get["/.*%.json$"] = WebUtil.send_file

exports.get["/.*%.css$"] = WebUtil.send_file

exports.get["/.*%.ttf$"] = WebUtil.send_file

exports.get["/.*%.woff$"] = WebUtil.send_file

exports.get["/.*%.woff2$"] = WebUtil.send_file

exports.get["/.*%.map$"] = WebUtil.send_file

exports.get["/.*%.mp3$"] = WebUtil.send_file

exports.get["/favicon.ico"] = WebUtil.send_file

exports.get["/"] = function(ctx) WebUtil.send_file(ctx, "index.html") end

return exports
