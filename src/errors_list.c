
static err_item errs_list[] = {
 { -188, "src/main_leveldb.c: leveldb_enum error" },
 { -175, "src/main_leveldb.c: leveldb_delete error" },
 { -167, "src/main_leveldb.c: leveldb_set error" },
 { -155, "src/main_leveldb.c: leveldb_get error" },
 { -146, "src/main_leveldb.c: no action specified" },
 { -90, "src/main_leveldb.c: leveldb init failed" },
 { -87, "src/main_leveldb.c: invalid path specified" },
 { -49, "src/main_leveldb.c: calloc failed" },

	{ 0, NULL }
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
