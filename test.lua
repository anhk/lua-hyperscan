

local patterns = {
    -- [ID] = "pattern"
    -- 目前ID需要为数字或字符串类型的数字
    ["3"] = "World",
    ["333"] = "bar",
    [1000] = "333",
    [100000000] = "foo",
}

local hyperscan = require 'hyperscan'

local h = hyperscan.new()

local ret, err = h:compile(patterns)
if not ret then
    return error("hyperscan compile error: ", err)
end

local ret, err = h:match("Hello World! foobar, 45563248222333444")

if ret and type(ret) == "table" then
    for _, v in ipairs(ret) do
        print("in lua: id=", tonumber(v))
    end
else
    print("match failure.")
end

local ret, err = h:match("Test world 3boforbar3984")

if ret and type(ret) == "table" then
    for _, v in ipairs(ret) do
        print("in lua: id=", tonumber(v))
    end
else
    print("match failure.")
end

