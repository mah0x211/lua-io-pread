# lua-io-pread

[![test](https://github.com/mah0x211/lua-io-pread/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-io-pread/actions/workflows/test.yml)
[![codecov](https://codecov.io/gh/mah0x211/lua-io-pread/branch/master/graph/badge.svg)](https://codecov.io/gh/mah0x211/lua-io-pread)


read nbytes of data from the specified position in the file without modifying the file pointer.


## Installation

```
luarocks install io-pread
```


## Error handling

the following return the erorr object that created by https://github.com/mah0x211/lua-errno or https://github.com/mah0x211/lua-error module when an error occurs.


## s, err, again = pread( f [, nbytes [, offset]] )

read nbytes of data from the specified position in the file without modifying the file pointer.


**Parameters**

- `f:file*|integer`: lua file object or file descriptor.
- `nbytes:integer`: number of bytes to read. if omitted, then read all data from the specified position.
- `offset:integer`: the position in the file to read from. if omitted, then read from the current position.

**Returns**

- `s:string`: read data from the file or `nil`. if a `nbytes` is `0` then return an `nil`.
- `err:any`: error object if an error occurs.
- `again:boolean`: `true` if it cannot read data because of end of file, or `pread` sets the `errno` to `EAGAIN` or `EWOULDBLOCK`. 

**Example**

```lua
local dump = require('dump')
local pread = require('io.pread')

-- create a temporary file
local f = assert(io.tmpfile())
f:write('hello world')
f:seek('set', 0)

-- read all data from a file starting at the current position
local s, err, again = pread(f)
print(dump {
    s = s,
    err = err,
    again = again,
})
-- {
--     s = "hello world"
-- }

-- read 5 bytes from a file starting at the current position
s, err, again = pread(f, 5)
print(dump {
    s = s,
    err = err,
    again = again,
})
-- {
--     s = "hello"
-- }

-- read 5 bytes from a file starting at the 5th byte from the beginning
s, err, again = pread(f, 5, 5)
print(dump {
    s = s,
    err = err,
    again = again,
})
-- {
--     s = " worl"
-- }

-- read all data from a file starting at the 100th byte from the beginning
s, err, again = pread(f, nil, 100)
print(dump {
    s = s,
    err = err,
    again = again,
})
-- {
--     again = true
-- }
```


## LICENSE

MIT License
