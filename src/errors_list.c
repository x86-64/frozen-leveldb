
static err_item errs_list[] = {
 { -340, "src/main_leveldb.c: leveldb_enum error" },
 { -328, "src/main_leveldb.c: leveldb_delete error" },
 { -316, "src/main_leveldb.c: leveldb_set error" },
 { -288, "src/main_leveldb.c: leveldb_get error" },
 { -272, "src/main_leveldb.c: no action specified" },
 { -195, "src/main_leveldb.c: leveldb_value_unserialize unknown output datatype: pass output key or set value_type in configuration" },
 { -189, "src/main_leveldb.c: leveldb_value_unserialize wrong output data supplied" },
 { -152, "src/main_leveldb.c: leveldb init failed" },
 { -140, "src/main_leveldb.c: leveldb configuration error: wrong management modes configuration" },
 { -134, "src/main_leveldb.c: invalid path specified" },
 { -86, "src/main_leveldb.c: calloc failed" },

	{ 0, NULL }
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
