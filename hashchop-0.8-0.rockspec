package = "hashchop"
version = "0.8-0"
source = {
   url = "git://github.com/silentbicycle/hashchop.git",
   tag = "v0.8-0",
   file = "hashchop-0.8-0.tar.gz",
   md5 = "e600b6e89c6abbf6b149809e149f6fce",
   dir = "hashchop",
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
