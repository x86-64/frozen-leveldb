#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <leveldb.h>
#include <leveldb/db.h>

struct ldb {
	leveldb::DB           *db;
};

ssize_t ldb_open   (ldb **db, char *path, uintmax_t flags){ // {{{
	ldb                   *new_db = new ldb;
	leveldb::DB           *ldb;
	leveldb::Status        status;
	leveldb::Options       options;
	
	if( (flags & CREATE_IF_MISSING) != 0) options.create_if_missing = true;
	if( (flags & ERROR_IF_EXIST)    != 0) options.error_if_exists = true;
	if( (flags & PARANOID)          != 0) options.paranoid_checks = true;
	if( (flags & NO_COMPRESSION)    != 0) options.compression = leveldb::kNoCompression;
	
	status = leveldb::DB::Open(options, std::string(path), &ldb);
	if(!status.ok())
		return -EFAULT;
	
	new_db->db = ldb;
	*db = new_db;
	return 0;
} // }}}
void    ldb_close  (ldb *db){ // {{{
	delete db->db;
	delete db;
} // }}}

ssize_t ldb_get    (ldb *db, ldb_slice *key, ldb_callback callback, void *userdata){
	leveldb::Status        status;
	std::string            value;
	ldb_slice              value_slice;
	
	status = db->db->Get(
		leveldb::ReadOptions(),
		leveldb::Slice(key->data, key->size),
		&value
	);
	if(status.IsNotFound()){
		callback(userdata, NULL);
		return -ENOENT;
	}
	if(status.ok()){
		value_slice.data = value.data();
		value_slice.size = value.size();
		
		callback(userdata, &value_slice);
		return 0;
	}
	return -EFAULT;
}
ssize_t ldb_set    (ldb *db, ldb_slice *key, ldb_slice *value){
	leveldb::Status        status;
	
	status = db->db->Put(
		leveldb::WriteOptions(),
		leveldb::Slice(key->data, key->size),
		leveldb::Slice(value->data, value->size)
	);
	return (status.ok()) ? 0 : -EFAULT;
}
ssize_t ldb_delete (ldb *db, ldb_slice *key){
	leveldb::Status        status;
	
	status = db->db->Delete(
		leveldb::WriteOptions(),
		leveldb::Slice(key->data, key->size)
	);
	if(status.IsNotFound()){
		return -ENOENT;
	}
	return (status.ok()) ? 0 : -EFAULT;
}

ssize_t ldb_enum   (ldb *db, ldb_callback callback, void *userdata){
	std::string            key;
	ldb_slice              key_slice;
	leveldb::Iterator*     it                = db->db->NewIterator(leveldb::ReadOptions());
	
	for (it->SeekToFirst(); it->Valid(); it->Next()){
		key            = it->key().ToString();
		key_slice.data = key.data();
		key_slice.size = key.size();
		
		if(callback(userdata, &key_slice) < 0)
			break;
	}
	delete it;
}

