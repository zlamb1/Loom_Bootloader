# Core Modules

create_module(fat ${LOOM_MOD_SRC_DIR}/fat/cluster.c ${LOOM_MOD_SRC_DIR}/fat/dirent.c 
                  ${LOOM_MOD_SRC_DIR}/fat/fat.c ${LOOM_MOD_SRC_DIR}/fat/name.c)
create_module(hello ${LOOM_MOD_SRC_DIR}/hello.c)
create_module(gpt ${LOOM_MOD_SRC_DIR}/gpt.c)
create_module(mbr ${LOOM_MOD_SRC_DIR}/mbr.c)