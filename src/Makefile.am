modules_LTLIBRARIES = module_c_leveldb.la
module_c_leveldb_la_CFLAGS = -I@includedir@/frozen/ -DFROZEN_MODULE
module_c_leveldb_la_LDFLAGS = -module -avoid-version -shared -lleveldb -lfrozen
module_c_leveldb_la_SOURCES = main_leveldb.c leveldb.cc leveldb.h
EXTRA_DIST = errors_list.c
