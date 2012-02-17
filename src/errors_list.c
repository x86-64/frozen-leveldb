
static err_item errs_list[] = {
 { -193, "src/main_leveldb.c: leveldb_enum error" },
 { -181, "src/main_leveldb.c: leveldb_delete error" },
 { -169, "src/main_leveldb.c: leveldb_set error" },
 { -145, "src/main_leveldb.c: leveldb_get error" },
 { -134, "src/main_leveldb.c: no action specified" },
 { -95, "src/main_leveldb.c: leveldb init failed" },
 { -86, "src/main_leveldb.c: invalid path specified" },
 { -53, "src/main_leveldb.c: calloc failed" },

	{ 0, NULL }
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
