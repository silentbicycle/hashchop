package = "hashchop"
version = "0.8-0"
source = {
    url = "https://github.com/silentbicycle/hashchop/archive/master.zip",
}
description = {
    summary = "Rolling-hash-based data stream chopper",
    detailed = [[
TODO descr
]],
    homepage = "http://github.com/silentbicycle/hashchop/",
    license = "ISC"
}
dependencies = {
    "lua ~> 5.1"
}
build = {
    type = "builtin",
    modules = {
        hashchop = {"hashchop.c",
                    "lhashchop.c"},
    }
}
