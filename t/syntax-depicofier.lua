-- This file is taken from the Depicofier test suite.
-- https://github.com/Enichan/Depicofier/blob/master/Depicofier/testsource.txt

print("hello world") -- hi!

--[[
?"this is a comment"
]]

---[[
?"this is not a comment"
--]]

aString = "this is a multi-line\
?'this is inside a string'\
single-line string"

someGlob = {
    value = 3
} -- this is a comment

if (someGlob != nil) someGlob.value *= 8 else print("someGlob undefined") // elseif end

?"Print shorthand", 1, 3 -- a comment here would normally mess things up

binnum = 0b10001.11

function add(a, b)
    someGlob["value"] += 1 * 3
    c -= d // another alt comment style
    e *= f
    g /= h
    i %= 20
    
    -- this is a normal lua operator and will give a warning unless processed in unified mode
    j = k << 4
    
    return a
end
