Hashchop uses a rolling hash to deterministically break streams of raw
data into bounded-size chunks. If two byte streams have mostly identical
content, with only a few changed bytes in one spot, there is a high
probability that the two chunk streams will quickly fall back in sync
with each other -- most of the chunks will be identical, and can be
shared. This chunking process is an excellent complement to persistent
data structures.


## Current status

It works, is fully tested, and benchmarks show that it is quite fast
enough (e.g. ~450 MB/sec on my MacBook Air). I haven't configured things
to make a proper static/dynamic library yet -- I am currently wrapping
it in Lua and using it in another project (scatterbrain, a distributed
filesystem), which will be on github eventually. Also, the tests could
be factored out better.


## Building

    $ make

This will make the test suite (`test`) and a simple benchmark (`bench`).

If you want to build this for use from Lua (the main use case, so far),
use `luarocks make $ROCKSPEC`, or `make lua`.


## Use

Specify the average desired chunk size (as a power of 2), then
repeatedly sink data into it and/or poll it for completed chunks until
end-of-stream is reached. Call `hashchop_finish` to get the remaining
chunk. More details are in the header file.
