
local profiler = require 'lulip'
local p = profiler:new()
p:dont('some-module')
p:maxrows(25)
p:start()

local patterns1 = {
    [1002] = "[/\\*|\\!]*union((#\\w*)|(/\\*.*)|(\\s\\w*)|\\r|\\n|(\\*/.*)|(\\*.*)|((\\x0b|\\xa0)\\w*))*select",
}

local patterns = {
    -- [ID] = "pattern"
    -- 目前ID需要为数字或字符串类型的数字
    [1002] = "[/\\*|\\!]*union((#\\w*)|(/\\*.*)|(\\s\\w*)|\\r|\\n|(\\*/.*)|(\\*.*)|((\\x0b|\\xa0)\\w*))*select",
    [3015] = "java\\.io\\.File|java\\.lang\\.ProcessBuilder|java\\.lang\\.Object|java\\.lang\\.Runtime|java\\.lang\\.System|java\\.lang\\.Class|java\\.lang\\.ClassLoader|java\\.lang\\.Shutdown|javax\\.script\\.ScriptEngineManager|ognl\\.OgnlContext|ognl\\.MemberAccess|ognl\\.ClassResolver|ognl\\.TypeConverter|com\\.opensymphony\\.xwork2\\.ActionContext|_memberAccess",
    [2004] = "\\bon(error|load|mouse|click|dblclick|drag|grop|scroll|key|submit|change|select|resize|cut|focus|start)\\b\\W*?=",
    [2008] = "background\\b\\W*?:\\W*?url|background-image\\b\\W*?:|behavior\\b\\W*?:\\W*?url|-moz-binding\\b|@import\\b|expression\\b\\W*?\\(",
    [3008] = "(-d(\\s|%20|\\+)*?auto_prepend_file(=|%3D))|(-d(\\s|%20|\\+)*?auto_append_file(=|%3D))",
    [3009] = "(fmdo\\s*?=\\s*?rename.+?|activepath\\s*?=.+?|oldfilename\\s*?=.+?|newfilename\\s*?=.+?\\.(?:php|php2|php3|php4|php5|phtml|asp|aspx|ascx|jsp|cfm|pl|cgi|cer|asa|htr|cdx|ashx|jspx|htaccess|asmx|jspf)\\b.*?){4}"
}

local hyperscan = require 'hyperscan'

local h = hyperscan.new()

local ret, err = h:compile(patterns1)
if not ret then
    return error("hyperscan compile error: ", err)
end

local ret, err = h:match("http://113.209.111.97/hyperscan?a=1 union select 1,2,3,4")

if ret and type(ret) == "table" then
    for _, v in ipairs(ret) do
        print("in lua: id=", tonumber(v))
    end
else
    print("match failure.")
end


p:stop()
p:dump("a.html")
