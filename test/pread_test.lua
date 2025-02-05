local testcase = require('testcase')
local assert = require('assert')
local pread = require('io.pread')

function testcase.pread()
    local f = assert(io.tmpfile())
    f:write('hello pread world')
    f:seek('set', 0)

    -- test that read from a file
    local s, err, again = pread(f, 1024)
    assert.is_nil(err)
    assert.is_nil(again)
    assert.equal(s, 'hello pread world')

    -- test that file position is not changed
    s, err, again = pread(f, 1024)
    assert.is_nil(err)
    assert.is_nil(again)
    assert.equal(s, 'hello pread world')

    -- test that read from a file with current position
    f:seek('set', 3)
    s, err, again = pread(f, 10)
    assert.is_nil(err)
    assert.is_nil(again)
    assert.equal(s, 'lo pread w')

    -- test that read all bytes from a current position if number of bytes is not passed
    s, err, again = pread(f)
    assert.is_nil(err)
    assert.is_nil(again)
    assert.equal(s, 'lo pread world')

    -- test that read with offset
    s, err, again = pread(f, nil, 0)
    assert.is_nil(err)
    assert.is_nil(again)
    assert.equal(s, 'hello pread world')

    -- test that return nil if determines that reaches end of file before reading any data
    s, err, again = pread(f, nil, 1024)
    assert.is_nil(s)
    assert.is_nil(err)
    assert.is_true(again)

    -- test that return nil if reaches end of file
    s, err, again = pread(f, 1024, 1024)
    assert.is_nil(s)
    assert.is_nil(err)
    assert.is_true(again)

    -- test that return nil if number of bytes is equal to 0
    s, err, again = pread(f, 0)
    assert.is_nil(s)
    assert.is_nil(err)
    assert.is_nil(again)

    -- test that lseek fails if invalid file descriptor is passed
    s, err, again = pread(-1)
    assert.is_nil(s)
    assert.is_nil(again)
    assert.re_match(err, 'EBADF.+lseek')

    -- test that fstat fails if invalid file descriptor is passed
    s, err, again = pread(-1, nil, 1)
    assert.is_nil(s)
    assert.is_nil(again)
    assert.re_match(err, 'EBADF.+fstat')

    -- test that return ENOMEM if allocation failed
    s, err, again = pread(f, 0x1FFFFFFFFFFFFF)
    assert.is_nil(s)
    assert.is_nil(again)
    assert.match(err, 'ENOMEM')

    -- test that throw error if file is not integer or file* object
    err = assert.throws(pread, 'not a file')
    assert.match(err, 'FILE* expected, got string')
end
