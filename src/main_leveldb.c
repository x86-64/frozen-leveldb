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
 *              path                    = "somepath/",            // path to directory with leveldb db
 *              key                     = (hashkey_t)"input",     // hashkey for key, default "key"
 *              value                   = (hashkey_t)"output",    // hashkey for value, default "value"
 *              create                  = (uint_t)"1",            // create db if not exist, default 1
 *              create_only             = (uint_t)"1",            // emit error if db already exist, default 0
 *              compress                = (uint_t)"1",            // compress data, default 1
 *              paranoid                = (uint_t)"1",            // paranoid checks (see leveldb man), default 0
 *              
 *              // Value type management modes, pick any
 *
 *              // Mode 1: all values saved to db in specified format (DEFAULT)
 *              value_same              = (uint_t)"1",            // default 1
 *              value_type              = (datatype_t)"ipv4_t",   // restrict type to this one, default no
 *              value_format            = (format_t)"packed",     // storage format, default "native"
 *              
 *              // Mode 2: value can have any type, type would be saved along with data itself
 *              //         value data would be stored in "packed" format
 *              value_any               = (uint_t)"1",            // default 0
 *              value_format            = (format_t)"packed"      // storage format, default "native"
 * }
 * @endcode
 */

#define HK_VALUE_create_only 55662
#define HK_VALUE_compress    22797
#define HK_VALUE_paranoid    21369
#define HK_VALUE_value_type      portable_hash("value_type")
#define HK_VALUE_value_format    portable_hash("value_format")
#define HK_VALUE_value_same      portable_hash("value_same")
#define HK_VALUE_value_any       portable_hash("value_any")
#define HK_VALUE_value_native    portable_hash("value_native")

typedef enum leveldb_value_mode {
	VALUE_MODE_SAME_TYPE,
	VALUE_MODE_ANY_TYPE,
} leveldb_value_mode;

typedef struct leveldb_userdata {
	uintmax_t              inited;
	ldb                   *db;
	hashkey_t              key;
	hashkey_t              value;
	leveldb_value_mode     value_mode;
	uintmax_t              value_type_restrict;
	datatype_t             value_type;
	format_t               value_format;
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
	
	userdata->key                 = HK(key);
	userdata->value               = HK(value);
	userdata->value_mode          = VALUE_MODE_SAME_TYPE;
	userdata->value_type_restrict = 0;
	userdata->value_type          = TYPE_VOIDT;
	userdata->value_format        = FORMAT(native);
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
	uintmax_t              t;
	uintmax_t              flags             = 0;
	uintmax_t              c_create          = 1;
	uintmax_t              c_compress        = 1;
	uintmax_t              c_error           = 0;
	uintmax_t              c_paranoid        = 0;
	uintmax_t              c_value_same      = 0;
	uintmax_t              c_value_any       = 0;
	char                  *c_path            = NULL;
	leveldb_userdata      *userdata          = (leveldb_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT,  userdata->key,          config, HK(key));
	hash_data_get(ret, TYPE_HASHKEYT,  userdata->value,        config, HK(value));
	hash_data_get(ret, TYPE_FORMATT,   userdata->value_format, config, HK(value_format));
	hash_data_get(ret, TYPE_UINTT,     c_value_same,           config, HK(value_same));
	hash_data_get(ret, TYPE_UINTT,     c_value_any,            config, HK(value_any));
	hash_data_get(ret, TYPE_UINTT,     c_create,               config, HK(create));
	hash_data_get(ret, TYPE_UINTT,     c_error,                config, HK(create_only));
	hash_data_get(ret, TYPE_UINTT,     c_compress,             config, HK(compress));
	hash_data_get(ret, TYPE_UINTT,     c_paranoid,             config, HK(paranoid));
	
	hash_data_get(ret, TYPE_DATATYPET, userdata->value_type,   config, HK(value_type));
	if(ret == 0){
		userdata->value_type_restrict = 1;
	}
	
	hash_data_convert(ret, TYPE_STRINGT,  c_path,              config, HK(path));
	if(ret != 0 || c_path == NULL)
		return error("invalid path specified");
	
	c_value_same   = (c_value_same   > 0) ? 1 : 0;
	c_value_any    = (c_value_any    > 0) ? 1 : 0;
	
	if(c_value_same + c_value_any > 1)
		return error("leveldb configuration error: wrong management modes configuration");
	
	userdata->value_mode = c_value_same   ? VALUE_MODE_SAME_TYPE : userdata->value_mode;
	userdata->value_mode = c_value_any    ? VALUE_MODE_ANY_TYPE  : userdata->value_mode;
	
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

static ssize_t leveldb_value_serialize(leveldb_userdata *userdata, data_t *input, ldb_slice *output, data_t *freeme){ // {{{
	switch(userdata->value_mode){
		case VALUE_MODE_SAME_TYPE:
			return data_make_flat(input,   userdata->value_format, freeme, (void **)&output->data, &output->size);
			
		case VALUE_MODE_ANY_TYPE:;
			data_t           d_data        = DATA_PTR_DATAT(input);
			
			return data_make_flat(&d_data, userdata->value_format, freeme, (void **)&output->data, &output->size);
	}
} // }}}
static ssize_t leveldb_value_unserialize(leveldb_userdata *userdata, ldb_slice *input, data_t *output){ // {{{
	ssize_t                ret;
	
	if(input == NULL)
		return 0;
	
	data_t                 d_input    = DATA_RAW(input->data, input->size);
	
	switch(userdata->value_mode){
		case VALUE_MODE_SAME_TYPE:
			// accept only void_t or same data type
			
			if(userdata->value_type_restrict != 0){
				// restrict type to user-supplied
				
				if(output->type == TYPE_VOIDT){                     // void_t can be overridden
					output->type = userdata->value_type;
				}else if(output->type != userdata->value_type){     // if types not match - emit error
					return error("leveldb_value_unserialize wrong output data supplied");
				}
			}else{
				// do not restrict type
				
				if(output->type == TYPE_VOIDT && input->size != 0)  // void_t as output and have actual data?
					return error("leveldb_value_unserialize unknown output datatype: pass output key or set value_type in configuration");
			}
			
			fastcall_convert_from r_same_convert  = { { 4, ACTION_CONVERT_FROM }, &d_input, userdata->value_format };
			return data_query(output, &r_same_convert);
			
		case VALUE_MODE_ANY_TYPE:;
			data_t                d_data          = DATA_PTR_DATAT(output);
			
			fastcall_convert_from r_convert       = { { 4, ACTION_CONVERT_FROM }, &d_input, userdata->value_format };
			return data_query(&d_data, &r_convert);
	}
} // }}}

static ssize_t leveldb_get_callback(leveldb_get_ctx *ctx, ldb_slice *value){ // {{{
	ssize_t                ret;
	data_t                *output;
	uintmax_t              need_free;
	leveldb_userdata      *userdata          = (leveldb_userdata *)ctx->machine->userdata;
	
	if(value == NULL) // not found
		return (ctx->ret = machine_pass(ctx->machine, ctx->request));
	
	if( (output = hash_data_find(ctx->request, ctx->hashkey)) != NULL){
		// already exist in request
		
		need_free = (output->type == TYPE_VOIDT) ? 1 : 0;
		
		if( (ctx->ret = leveldb_value_unserialize(userdata, value, output)) < 0)
			return ctx->ret;
		
		ctx->ret = machine_pass(ctx->machine, ctx->request);
		
		if(need_free)
			data_free(output);
	}else{
		// assign new
		
		request_t r_next[] = {
			{ ctx->hashkey, DATA_VOID },
			hash_inline(ctx->request),
			hash_end
		};
		output = &r_next[0].data;
		
		if( (ctx->ret = leveldb_value_unserialize(userdata, value, output)) < 0)
			return ctx->ret;
		
		ctx->ret = machine_pass(ctx->machine, r_next);
		
		data_free(output)
	}
	return ctx->ret;
} // }}}
static ssize_t leveldb_enum_callback(leveldb_enum_ctx *ctx, ldb_slice *key){ // {{{
	data_t                 d_key             = DATA_RAW(key->data, key->size);
	data_t                 d_copy;
	
	fastcall_copy r_copy = { { 3, ACTION_COPY }, &d_copy };
	if( (ctx->ret = data_query(&d_key, &r_copy)) < 0)
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
			if( (ret = leveldb_value_serialize(
				userdata,
				hash_data_find(request, userdata->value),
				&value,
				&free_value) < 0)
			){
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
