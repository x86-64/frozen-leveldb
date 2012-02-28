
static err_item errs_list[] = {
 { -206, "src/main_leveldb.c: leveldb_enum error" },
 { -194, "src/main_leveldb.c: leveldb_delete error" },
 { -182, "src/main_leveldb.c: leveldb_set error" },
 { -159, "src/main_leveldb.c: leveldb_get error" },
 { -143, "src/main_leveldb.c: no action specified" },
 { -95, "src/main_leveldb.c: leveldb init failed" },
 { -86, "src/main_leveldb.c: invalid path specified" },
 { -53, "src/main_leveldb.c: calloc failed" },

	{ 0, NULL }
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
