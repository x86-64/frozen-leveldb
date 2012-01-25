#include <libfrozen.h>
#include <leveldb.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_leveldb module/leveldb
 */
/**
 * @ingroup mod_machine_leveldb
 * @page page_leveldb_info Description
 *
 */
/**
 * @ingroup mod_machine_leveldb
 * @page page_leveldb_config Configuration
 *  
 * Accepted configuration:
 * @code
 * {
 *              class                   = "modules/leveldb",
 * }
 * @endcode
 */

#define EMODULE 104

#define HK_VALUE_create_only 55662
#define HK_VALUE_compress    22797
#define HK_VALUE_paranoid    21369

typedef struct leveldb_userdata {
	uintmax_t              inited;
	ldb                   *db;
	hashkey_t              key;
	hashkey_t              value;
} leveldb_userdata;

typedef struct leveldb_ctx {
	machine_t             *machine;
	request_t             *request;
	ssize_t                ret;
} leveldb_ctx;

static int leveldb_init(machine_t *machine){ // {{{
	leveldb_userdata         *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(leveldb_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key   = HK(key);
	userdata->value = HK(value);
	return 0;
} // }}}
static int leveldb_destroy(machine_t *machine){ // {{{
	leveldb_userdata     *userdata          = (leveldb_userdata *)machine->userdata;
	
	if(userdata->inited != 0)
		ldb_close(userdata->db);
	free(userdata);
	return 0;
} // }}}
static int leveldb_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              flags             = 0;
	uintmax_t              c_create          = 1;
	uintmax_t              c_compress        = 1;
	uintmax_t              c_error           = 0;
	uintmax_t              c_paranoid        = 0;
	char                  *c_path            = NULL;
	leveldb_userdata      *userdata          = (leveldb_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_STRINGT,  c_path,          config, HK(path));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key,   config, HK(key));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->value, config, HK(value));
	hash_data_get(ret, TYPE_UINTT,    c_create,        config, HK(create));
	hash_data_get(ret, TYPE_UINTT,    c_error,         config, HK(create_only));
	hash_data_get(ret, TYPE_UINTT,    c_compress,      config, HK(compress));
	hash_data_get(ret, TYPE_UINTT,    c_paranoid,      config, HK(paranoid));

	flags |= (c_create   != 0) ? CREATE_IF_MISSING : 0;
	flags |= (c_error    != 0) ? ERROR_IF_EXIST : 0;
	flags |= (c_compress != 0) ? 0 : NO_COMPRESSION;
	flags |= (c_paranoid != 0) ? PARANOID : 0;
	
	if(c_path == NULL)
		return error("invalid path specified");
	
	if(ldb_open(&userdata->db, c_path, flags) < 0)
		return error("leveldb init failed");
	
	userdata->inited = 1;
	return 0;
} // }}}

static void    leveldb_get_callback(leveldb_ctx *ctx, ldb_slice *value){ // {{{
	leveldb_userdata      *userdata          = (leveldb_userdata *)ctx->machine->userdata;
	
	request_t r_next[] = {
		{ userdata->value, DATA_RAW(value->data, value->size) },
		hash_inline(ctx->request),
		hash_end
	};
	ctx->ret = machine_pass(ctx->machine, r_next);
} // }}}
static ssize_t get_slice(data_t *data, ldb_slice *slice){ // {{{
	ssize_t                ret               = 0;
	data_t                 raw_data          = { TYPE_RAWT, NULL };
	
	if(data == NULL)
		return -EINVAL;
	
	// direct pointers
	fastcall_physicallen r_len = { { 3, ACTION_LOGICALLEN } };
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_len) == 0 && data_query(data, &r_ptr) == 0 && r_ptr.ptr != NULL){
		slice->data = r_ptr.ptr;
		slice->size = r_len.length;
		return 0;
	}
	
	// strange types
	fastcall_transfer r_transfer = { { 4, ACTION_TRANSFER }, &raw_data };
	if(data_query(data, &r_transfer) < 0)
		return -EINVAL;
	
	if( data_query(data, &r_len) == 0 && data_query(data, &r_ptr) == 0 && r_ptr.ptr != NULL){
		slice->data = r_ptr.ptr;
		slice->size = r_len.length;
	}else{
		ret = -EFAULT;
	}
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&raw_data, &r_free);
	return ret;
} // }}}

static ssize_t leveldb_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	action_t               action;
	ldb_slice              key;
	ldb_slice              value;
	leveldb_ctx            ctx               = { machine, request, 0 };
	leveldb_userdata      *userdata          = (leveldb_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(ret != 0)
		return error("no action specified");
	
	switch(action){
		case ACTION_READ:
			if( (ret = get_slice(hash_data_find(request, userdata->key), &key)) < 0)
				return ret;
			
			if(ldb_get(userdata->db, &key, (ldb_callback)&leveldb_get_callback, &ctx) < 0)
				return error("leveldb_get error");
			
			return ctx.ret;
		
		case ACTION_WRITE:
			if(
				((ret = get_slice( hash_data_find(request, userdata->key),   &key))   < 0) ||
				((ret = get_slice( hash_data_find(request, userdata->value), &value)) < 0)
			)
				return ret;
			
			if(ldb_set(userdata->db, &key, &value) < 0)
				return error("leveldb_set error");
			
			break;
		case ACTION_DELETE:
			if( (ret = get_slice(hash_data_find(request, userdata->key), &key)) < 0)
				return ret;
			
			if(ldb_delete(userdata->db, &key) < 0)
				return error("leveldb_delete error");
			
			break;
		default:
			return -ENOSYS;
	};
	return machine_pass(machine, request);
} // }}}

static machine_t c_leveldb_proto = {
	.class          = "modules/leveldb",
	.supported_api  = API_HASH,
	.func_init      = &leveldb_init,
	.func_configure = &leveldb_configure,
	.func_destroy   = &leveldb_destroy,
	.machine_type_hash = {
		.func_handler = &leveldb_handler
	}
};

int main(void){
	class_register(&c_leveldb_proto);
	return 0;
}
