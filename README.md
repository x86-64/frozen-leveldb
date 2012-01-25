This is glue code for leveldb support in frozen.

Installation
-----------------

1. Compile and install frozen
2. Compile and install leveldb. Required CFLAGS="-fPIC"
3. Download or clone this repo
4. ./configure --prefix=/usr
5. make
6. make install


Usage
-----------------

{
	class       = "modules/leveldb",
	path        = "leveldb_test"                      // leveldb database folder
	key         = (hashkey_t)"key",                   // key hashkey, default "key"
	value       = (hashkey_t)"value",                 // value hashkey, default "value"
	compress    = (uint_t)"1",                        // compress data, default 1
	create      = (uint_t)"1",                        // create db if not exist, default 1
	create_only = (uint_t)"0",                        // emit error if db already exist, default 0
	paranoid    = (uint_t)"0"                         // paranoid mode (see leveldb docs), default 0
},
{ class = "stdout", input = (hashkey_t)"value" }


Accept only read, write and delete requests. Any successful request passed to next machine.

