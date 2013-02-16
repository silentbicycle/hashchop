package = "hashchop"
version = "0.8-0"
source = {
    url = "https://github.com/silentbicycle/hashchop/archive/v0.8-0.zip",
}
description = {
    summary = "Rolling-hash-based data stream chopper",
    detailed = [[
Hashchop uses a rolling hash to deterministically break streams of raw
data into bounded-size chunks. If two byte streams have mostly identical
content, with only a few changed bytes in one spot, there is a high
probability that the two chunk streams will quickly fall back in sync
with each other -- most of the chunks will be identical, and can be
shared. This chunking process is an excellent complement to persistent
data structures.
]],
    homepage = "http://github.com/silentbicycle/hashchop/",
    license = "ISC"
}
dependencies = {
    "lua >= 5.1"
}
build = {
    type = "builtin",
    modules = {
        hashchop = {"hashchop.c",
                    "lhashchop.c"},
    }
}
