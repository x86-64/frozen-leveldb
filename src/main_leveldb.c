#include <libfrozen.h>
#include <leveldb.h>

#include <errors_list.c>

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

#define HK_VALUE_create_only 55662
#define HK_VALUE_compress    22797
#define HK_VALUE_paranoid    21369

typedef struct leveldb_userdata {
	uintmax_t              inited;
	ldb                   *db;
	hashkey_t              key;
	hashkey_t              value;
} leveldb_userdata;

typedef struct leveldb_get_ctx {
	machine_t             *machine;
	request_t             *request;
	ssize_t                ret;
	hashkey_t              hashkey;
} leveldb_get_ctx;
typedef struct leveldb_enum_ctx {
	data_t                *dest_data;
	ssize_t                ret;
} leveldb_enum_ctx;

static ssize_t leveldb_init(machine_t *machine){ // {{{
	leveldb_userdata         *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(leveldb_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key   = HK(key);
	userdata->value = HK(value);
	return 0;
} // }}}
static ssize_t leveldb_destroy(machine_t *machine){ // {{{
	leveldb_userdata     *userdata          = (leveldb_userdata *)machine->userdata;
	
	if(userdata->inited != 0)
		ldb_close(userdata->db);
	free(userdata);
	return 0;
} // }}}
static ssize_t leveldb_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              flags             = 0;
	uintmax_t              c_create          = 1;
	uintmax_t              c_compress        = 1;
	uintmax_t              c_error           = 0;
	uintmax_t              c_paranoid        = 0;
	char                  *c_path            = NULL;
	leveldb_userdata      *userdata          = (leveldb_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key,   config, HK(key));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->value, config, HK(value));
	hash_data_get(ret, TYPE_UINTT,    c_create,        config, HK(create));
	hash_data_get(ret, TYPE_UINTT,    c_error,         config, HK(create_only));
	hash_data_get(ret, TYPE_UINTT,    c_compress,      config, HK(compress));
	hash_data_get(ret, TYPE_UINTT,    c_paranoid,      config, HK(paranoid));
	
	hash_data_convert(ret, TYPE_STRINGT,  c_path,      config, HK(path));
	if(ret != 0 || c_path == NULL)
		return error("invalid path specified");
	
	flags |= (c_create   != 0) ? CREATE_IF_MISSING : 0;
	flags |= (c_error    != 0) ? ERROR_IF_EXIST : 0;
	flags |= (c_compress != 0) ? 0 : NO_COMPRESSION;
	flags |= (c_paranoid != 0) ? PARANOID : 0;
	
	if(ldb_open(&userdata->db, c_path, flags) < 0){
		free(c_path);
		return error("leveldb init failed");
	}
	
	userdata->inited = 1;
	free(c_path);
	return 0;
} // }}}

static ssize_t leveldb_get_callback(leveldb_get_ctx *ctx, ldb_slice *value){ // {{{
	if(value == NULL){ // not found
		request_t r_next[] = {
			{ ctx->hashkey, DATA_VOID },
			hash_inline(ctx->request),
			hash_end
		};
		return (ctx->ret = machine_pass(ctx->machine, r_next));
	}else{
		request_t r_next[] = {
			{ ctx->hashkey, DATA_RAW(value->data, value->size) },
			hash_inline(ctx->request),
			hash_end
		};
		return (ctx->ret = machine_pass(ctx->machine, r_next));
	}
} // }}}
static ssize_t leveldb_enum_callback(leveldb_enum_ctx *ctx, ldb_slice *value){ // {{{
	data_t                 d_value           = DATA_RAW(value->data, value->size);
	data_t                 d_copy;
	
	fastcall_copy r_copy = { { 3, ACTION_COPY }, &d_copy };
	if( (ctx->ret = data_query(&d_value, &r_copy)) < 0)
		return ctx->ret;
	
	fastcall_push r_push = { { 3, ACTION_PUSH }, &d_copy };
	return (ctx->ret = data_query(ctx->dest_data, &r_push));
} // }}}

static ssize_t leveldb_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	action_t               action;
	data_t                 free_key;
	data_t                 free_value;
	ldb_slice              key;
	ldb_slice              value;
	leveldb_userdata      *userdata          = (leveldb_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(ret != 0)
		return error("no action specified");
	
	switch(action){
		case ACTION_READ:
			if( (ret = data_get_continious(hash_data_find(request, userdata->key), &free_key, (void **)&key.data, &key.size)) < 0){
				data_free(&free_key);
				return ret;
			}
			
			leveldb_get_ctx        ctx_get           = { machine, request, 0, userdata->value };
			switch(ldb_get(userdata->db, &key, (ldb_callback)&leveldb_get_callback, &ctx_get)){
				case 0:
				case -ENOENT:
					ret = ctx_get.ret;
					break;
				default:
					ret = error("leveldb_get error");
					break;
			}
			
			data_free(&free_key);
			return ret;
		
		case ACTION_WRITE:
			if( (ret = data_get_continious(hash_data_find(request, userdata->key),   &free_key,   (void **)&key.data,   &key.size)) < 0){
				data_free(&free_key);
				return ret;
			}
			if( (ret = data_get_continious(hash_data_find(request, userdata->value), &free_value, (void **)&value.data, &value.size)) < 0){
				data_free(&free_value);
				return ret;
			}
			
			ret = ldb_set(userdata->db, &key, &value);
			
			data_free(&free_key);
			data_free(&free_value);
			
			if(ret < 0)
				return error("leveldb_set error");
			
			break;
		case ACTION_DELETE:
			if( (ret = data_get_continious(hash_data_find(request, userdata->key), &free_key, (void **)&key.data, &key.size)) < 0)
				return ret;
			
			ret = ldb_delete(userdata->db, &key);
			
			data_free(&free_key);
			
			if(ret < 0)
				return error("leveldb_delete error");
			
			break;
		
		case ACTION_ENUM:;
			data_t             *dest_data;
			
			if( (dest_data = hash_data_find(request, HK(data))) == NULL)
				return -EINVAL;
			
			leveldb_enum_ctx       ctx_enum          = { dest_data, 0 };
			if(ldb_enum(userdata->db, (ldb_callback)&leveldb_enum_callback, &ctx_enum) < 0)
				return error("leveldb_enum error");
	
			fastcall_push r_push = { { 3, ACTION_PUSH }, NULL };
			data_query(dest_data, &r_push);
			
			if(ctx_enum.ret < 0)
				return ctx_enum.ret;
			
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
	errors_register((err_item *)&errs_list, &emodule);
	class_register(&c_leveldb_proto);
	return 0;
}
