require "lunatest"
require "hashchop"

-- The C library has most of the testing, this is just a sanity check.

function test_single_chunk_in_out()
    local h = hashchop.new(10)
    print("tostring -> ", h)
    local mt = getmetatable(h)
    local data = "bananas"
    local res, err = h:sink(data)
    assert_equal(res, "ok")
    local out, err = h:finish()
    assert_equal(data, out)
    assert_nil(err)
end

function test_single_chunk_added_byte_by_byte()
    local h = hashchop.new(10)
    local mt = getmetatable(h)
    local data = "bananas"
    for c in data:gmatch("(.)") do
        local res, err = h:sink(c)
        assert_equal(res, "ok")
    end
    local out, err = h:finish()
    assert_equal(data, out)
end

lunatest.run()
