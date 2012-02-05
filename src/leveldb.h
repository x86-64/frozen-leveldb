#ifndef LEVELDB_H
#define LEVELDB_H
#ifdef __cplusplus
extern "C" {
#endif

enum ldb_flags {
	CREATE_IF_MISSING = 1,
	ERROR_IF_EXIST = 2,
	PARANOID = 4,
	NO_COMPRESSION = 8,
};

typedef struct ldb        ldb;
typedef struct ldb_slice  ldb_slice;
typedef enum   ldb_flags  ldb_flags;

typedef ssize_t (*ldb_callback)(void *userdata, ldb_slice *value);

struct ldb_slice {
	const char             *data;
	uintmax_t               size;
};

ssize_t ldb_open   (ldb **db, char *path, uintmax_t flags);
void    ldb_close  (ldb *db);

ssize_t ldb_get    (ldb *db, ldb_slice *key, ldb_callback callback, void *userdata);
ssize_t ldb_set    (ldb *db, ldb_slice *key, ldb_slice *value);
ssize_t ldb_delete (ldb *db, ldb_slice *key);
ssize_t ldb_enum   (ldb *db, ldb_callback callback, void *userdata);

#ifdef __cplusplus
}
#endif
#endif
