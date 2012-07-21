
static err_item errs_list[] = {
 { -534, "src/main_leveldb.c: leveldb_enum error" },
 { -522, "src/main_leveldb.c: leveldb_delete error" },
 { -510, "src/main_leveldb.c: leveldb_set error" },
 { -482, "src/main_leveldb.c: leveldb_get error" },
 { -466, "src/main_leveldb.c: no action specified" },
 { -386, "src/main_leveldb.c: leveldb_value_unserialize unknown output datatype: pass output key or set value_type in configuration" },
 { -380, "src/main_leveldb.c: leveldb_value_unserialize wrong output data supplied" },
 { -343, "src/main_leveldb.c: leveldb init failed" },
 { -331, "src/main_leveldb.c: leveldb configuration error: wrong management modes configuration" },
 { -325, "src/main_leveldb.c: invalid path specified" },
 { -277, "src/main_leveldb.c: calloc failed" },

};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
