
static err_item errs_list[] = {
 { -334, "src/main_leveldb.c: leveldb_enum error" },
 { -322, "src/main_leveldb.c: leveldb_delete error" },
 { -310, "src/main_leveldb.c: leveldb_set error" },
 { -282, "src/main_leveldb.c: leveldb_get error" },
 { -266, "src/main_leveldb.c: no action specified" },
 { -186, "src/main_leveldb.c: leveldb_value_unserialize unknown output datatype: pass output key or set value_type in configuration" },
 { -180, "src/main_leveldb.c: leveldb_value_unserialize wrong output data supplied" },
 { -143, "src/main_leveldb.c: leveldb init failed" },
 { -131, "src/main_leveldb.c: leveldb configuration error: wrong management modes configuration" },
 { -125, "src/main_leveldb.c: invalid path specified" },
 { -77, "src/main_leveldb.c: calloc failed" },

	{ 0, NULL }
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
