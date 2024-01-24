/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2010 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: zend_compile.c 300817 2010-06-28 16:37:57Z felipe $ */

#include <zend_language_parser.h>
#include "zend.h"
#include "zend_compile.h"
#include "zend_constants.h"
#include "zend_llist.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "tsrm_virtual_cwd.h"

#ifdef ZEND_MULTIBYTE
#include "zend_multibyte.h"
#endif /* ZEND_MULTIBYTE */

ZEND_API zend_op_array *(*zend_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);
ZEND_API zend_op_array *(*zend_compile_string)(zval *source_string, char *filename TSRMLS_DC);

#ifndef ZTS
ZEND_API zend_compiler_globals compiler_globals;
ZEND_API zend_executor_globals executor_globals;
#endif

static void zend_duplicate_property_info(zend_property_info *property_info) /* {{{ */
{
	property_info->name = estrndup(property_info->name, property_info->name_length);
	if (property_info->doc_comment) {
		property_info->doc_comment = estrndup(property_info->doc_comment, property_info->doc_comment_len);
	}
}
/* }}} */


static void zend_duplicate_property_info_internal(zend_property_info *property_info) /* {{{ */
{
	property_info->name = zend_strndup(property_info->name, property_info->name_length);
}
/* }}} */


static void zend_destroy_property_info(zend_property_info *property_info) /* {{{ */
{
	efree(property_info->name);
	if (property_info->doc_comment) {
		efree(property_info->doc_comment);
	}
}
/* }}} */


static void zend_destroy_property_info_internal(zend_property_info *property_info) /* {{{ */
{
	free(property_info->name);
}
/* }}} */

static void build_runtime_defined_function_key(zval *result, const char *name, int name_length TSRMLS_DC) /* {{{ */
{
	char char_pos_buf[32];
	uint char_pos_len;
	char *filename;

	char_pos_len = zend_sprintf(char_pos_buf, "%p", LANG_SCNG(yy_text));
	if (CG(active_op_array)->filename) {
		filename = CG(active_op_array)->filename;
	} else {
		filename = "-";
	}

	/* NULL, name length, filename length, last accepting char position length */
	result->value.str.len = 1+name_length+strlen(filename)+char_pos_len;
#ifdef ZEND_MULTIBYTE
 	/* must be binary safe */
 	result->value.str.val = (char *) safe_emalloc(result->value.str.len, 1, 1);
 	result->value.str.val[0] = '\0';
 	sprintf(result->value.str.val+1, "%s%s%s", name, filename, char_pos_buf);
#else
	zend_spprintf(&result->value.str.val, 0, "%c%s%s%s", '\0', name, filename, char_pos_buf);
#endif /* ZEND_MULTIBYTE */
	result->type = IS_STRING;
	Z_SET_REFCOUNT_P(result, 1);
}
/* }}} */


int zend_auto_global_arm(zend_auto_global *auto_global TSRMLS_DC) /* {{{ */
{
	auto_global->armed = (auto_global->auto_global_callback ? 1 : 0);
	return 0;
}
/* }}} */


ZEND_API int zend_auto_global_disable_jit(const char *varname, zend_uint varname_length TSRMLS_DC) /* {{{ */
{
	zend_auto_global *auto_global;

	if (zend_hash_find(CG(auto_globals), varname, varname_length+1, (void **) &auto_global)==FAILURE) {
		return FAILURE;
	}
	auto_global->armed = 0;
	return SUCCESS;
}
/* }}} */


static void init_compiler_declarables(TSRMLS_D) /* {{{ */
{
	Z_TYPE(CG(declarables).ticks) = IS_LONG;
	Z_LVAL(CG(declarables).ticks) = 0;
}
/* }}} */


void zend_init_compiler_data_structures(TSRMLS_D) /* {{{ */
{
	zend_stack_init(&CG(bp_stack));
	zend_stack_init(&CG(function_call_stack));
	zend_stack_init(&CG(switch_cond_stack));
	zend_stack_init(&CG(foreach_copy_stack));
	zend_stack_init(&CG(object_stack));
	zend_stack_init(&CG(declare_stack));
	CG(active_class_entry) = NULL;
	zend_llist_init(&CG(list_llist), sizeof(list_llist_element), NULL, 0);
	zend_llist_init(&CG(dimension_llist), sizeof(int), NULL, 0);
	zend_stack_init(&CG(list_stack));
	CG(in_compilation) = 0;
	CG(start_lineno) = 0;
	CG(current_namespace) = NULL;
	CG(in_namespace) = 0;
	CG(has_bracketed_namespaces) = 0;
	CG(current_import) = NULL;
	init_compiler_declarables(TSRMLS_C);
	zend_hash_apply(CG(auto_globals), (apply_func_t) zend_auto_global_arm TSRMLS_CC);
	zend_stack_init(&CG(labels_stack));
	CG(labels) = NULL;

#ifdef ZEND_MULTIBYTE
	CG(script_encoding_list) = NULL;
	CG(script_encoding_list_size) = 0;
	CG(internal_encoding) = NULL;
	CG(encoding_detector) = NULL;
	CG(encoding_converter) = NULL;
	CG(encoding_oddlen) = NULL;
	CG(encoding_declared) = 0;
#endif /* ZEND_MULTIBYTE */
}
/* }}} */


ZEND_API void file_handle_dtor(zend_file_handle *fh) /* {{{ */
{
	TSRMLS_FETCH();

	zend_file_handle_dtor(fh TSRMLS_CC);
}
/* }}} */


void init_compiler(TSRMLS_D) /* {{{ */
{
	CG(active_op_array) = NULL;
	zend_init_compiler_data_structures(TSRMLS_C);
	zend_init_rsrc_list(TSRMLS_C);
	zend_hash_init(&CG(filenames_table), 5, NULL, (dtor_func_t) free_estring, 0);
	zend_llist_init(&CG(open_files), sizeof(zend_file_handle), (void (*)(void *)) file_handle_dtor, 0);
	CG(unclean_shutdown) = 0;
}
/* }}} */


void shutdown_compiler(TSRMLS_D) /* {{{ */
{
	zend_stack_destroy(&CG(bp_stack));
	zend_stack_destroy(&CG(function_call_stack));
	zend_stack_destroy(&CG(switch_cond_stack));
	zend_stack_destroy(&CG(foreach_copy_stack));
	zend_stack_destroy(&CG(object_stack));
	zend_stack_destroy(&CG(declare_stack));
	zend_stack_destroy(&CG(list_stack));
	zend_hash_destroy(&CG(filenames_table));
	zend_llist_destroy(&CG(open_files));
	zend_stack_destroy(&CG(labels_stack));

#ifdef ZEND_MULTIBYTE
	if (CG(script_encoding_list)) {
		efree(CG(script_encoding_list));
	}
#endif /* ZEND_MULTIBYTE */
}
/* }}} */


ZEND_API char *zend_set_compiled_filename(const char *new_compiled_filename TSRMLS_DC) /* {{{ */
{
	char **pp, *p;
	int length = strlen(new_compiled_filename);

	if (zend_hash_find(&CG(filenames_table), new_compiled_filename, length+1, (void **) &pp) == SUCCESS) {
		CG(compiled_filename) = *pp;
		return *pp;
	}
	p = estrndup(new_compiled_filename, length);
	zend_hash_update(&CG(filenames_table), new_compiled_filename, length+1, &p, sizeof(char *), (void **) &pp);
	CG(compiled_filename) = p;
	return p;
}
/* }}} */


ZEND_API void zend_restore_compiled_filename(char *original_compiled_filename TSRMLS_DC) /* {{{ */
{
	CG(compiled_filename) = original_compiled_filename;
}
/* }}} */


ZEND_API char *zend_get_compiled_filename(TSRMLS_D) /* {{{ */
{
	return CG(compiled_filename);
}
/* }}} */


ZEND_API int zend_get_compiled_lineno(TSRMLS_D) /* {{{ */
{
	return CG(zend_lineno);
}
/* }}} */


ZEND_API zend_bool zend_is_compiling(TSRMLS_D) /* {{{ */
{
	return CG(in_compilation);
}
/* }}} */


static zend_uint get_temporary_variable(zend_op_array *op_array) /* {{{ */
{
	return (op_array->T)++ * ZEND_MM_ALIGNED_SIZE(sizeof(temp_variable));
}
/* }}} */

static int lookup_cv(zend_op_array *op_array, char* name, int name_len) /* {{{ */
{
	int i = 0;
	ulong hash_value = zend_inline_hash_func(name, name_len+1);

	while (i < op_array->last_var) {
		if (op_array->vars[i].hash_value == hash_value &&
		    op_array->vars[i].name_len == name_len &&
		    strcmp(op_array->vars[i].name, name) == 0) {
		  efree(name);
		  return i;
		}
		i++;
	}
	i = op_array->last_var;
	op_array->last_var++;
	if (op_array->last_var > op_array->size_var) {
		op_array->size_var += 16; /* FIXME */
		op_array->vars = erealloc(op_array->vars, op_array->size_var*sizeof(zend_compiled_variable));
	}
	op_array->vars[i].name = name; /* estrndup(name, name_len); */
	op_array->vars[i].name_len = name_len;
	op_array->vars[i].hash_value = hash_value;
	return i;
}
/* }}} */


void zend_do_binary_op(zend_uchar op, znode *result, const znode *op1, const znode *op2 TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *op1;
	opline->op2 = *op2;
	*result = opline->result;
}
/* }}} */

void zend_do_unary_op(zend_uchar op, znode *result, const znode *op1 TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *op1;
	*result = opline->result;
	SET_UNUSED(opline->op2);
}
/* }}} */

#define MAKE_NOP(opline)	{ opline->opcode = ZEND_NOP;  memset(&opline->result,0,sizeof(znode)); memset(&opline->op1,0,sizeof(znode)); memset(&opline->op2,0,sizeof(znode)); opline->result.op_type=opline->op1.op_type=opline->op2.op_type=IS_UNUSED;  }


static void zend_do_op_data(zend_op *data_op, const znode *value TSRMLS_DC) /* {{{ */
{
	data_op->opcode = ZEND_OP_DATA;
	data_op->op1 = *value;
	SET_UNUSED(data_op->op2);
}
/* }}} */

void zend_do_binary_assign_op(zend_uchar op, znode *result, const znode *op1, const znode *op2 TSRMLS_DC) /* {{{ */
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_OBJ_RW:
				last_op->opcode = op;
				last_op->extended_value = ZEND_ASSIGN_OBJ;

				zend_do_op_data(opline, op2 TSRMLS_CC);
				SET_UNUSED(opline->result);
				*result = last_op->result;
				return;
			case ZEND_FETCH_DIM_RW:
				last_op->opcode = op;
				last_op->extended_value = ZEND_ASSIGN_DIM;

				zend_do_op_data(opline, op2 TSRMLS_CC);
				opline->op2.u.var = get_temporary_variable(CG(active_op_array));
				opline->op2.u.EA.type = 0;
				opline->op2.op_type = IS_VAR;
				SET_UNUSED(opline->result);
				*result = last_op->result;
				return;
			default:
				break;
		}
	}

	opline->opcode = op;
	opline->op1 = *op1;
	opline->op2 = *op2;
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}
/* }}} */

void fetch_simple_variable_ex(znode *result, znode *varname, int bp, zend_uchar op TSRMLS_DC) /* {{{ */
{
	zend_op opline;
	zend_op *opline_ptr;
	zend_llist *fetch_list_ptr;

	if (varname->op_type == IS_CONST) {
		if (Z_TYPE(varname->u.constant) != IS_STRING) {
			convert_to_string(&varname->u.constant);
		}
		if (!zend_is_auto_global(varname->u.constant.value.str.val, varname->u.constant.value.str.len TSRMLS_CC) &&
		    !(varname->u.constant.value.str.len == (sizeof("this")-1) &&
		      !memcmp(varname->u.constant.value.str.val, "this", sizeof("this"))) &&
		    (CG(active_op_array)->last == 0 ||
		     CG(active_op_array)->opcodes[CG(active_op_array)->last-1].opcode != ZEND_BEGIN_SILENCE)) {
			result->op_type = IS_CV;
			result->u.var = lookup_cv(CG(active_op_array), varname->u.constant.value.str.val, varname->u.constant.value.str.len);
			result->u.EA.type = 0;
			return;
		}
	}

	if (bp) {
		opline_ptr = &opline;
		init_op(opline_ptr TSRMLS_CC);
	} else {
		opline_ptr = get_next_op(CG(active_op_array) TSRMLS_CC);
	}

	opline_ptr->opcode = op;
	opline_ptr->result.op_type = IS_VAR;
	opline_ptr->result.u.EA.type = 0;
	opline_ptr->result.u.var = get_temporary_variable(CG(active_op_array));
	opline_ptr->op1 = *varname;
	*result = opline_ptr->result;
	SET_UNUSED(opline_ptr->op2);

	opline_ptr->op2.u.EA.type = ZEND_FETCH_LOCAL;
	if (varname->op_type == IS_CONST && varname->u.constant.type == IS_STRING) {
		if (zend_is_auto_global(varname->u.constant.value.str.val, varname->u.constant.value.str.len TSRMLS_CC)) {
			opline_ptr->op2.u.EA.type = ZEND_FETCH_GLOBAL;
		}
	}

	if (bp) {
		zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
		zend_llist_add_element(fetch_list_ptr, opline_ptr);
	}
}
/* }}} */

void fetch_simple_variable(znode *result, znode *varname, int bp TSRMLS_DC) /* {{{ */
{
	/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	fetch_simple_variable_ex(result, varname, bp, ZEND_FETCH_W TSRMLS_CC);
}
/* }}} */

void zend_do_fetch_static_member(znode *result, znode *class_name TSRMLS_DC) /* {{{ */
{
	znode class_node;
	zend_llist *fetch_list_ptr;
	zend_llist_element *le;
	zend_op *opline_ptr;
	zend_op opline;

	zend_do_fetch_class(&class_node, class_name TSRMLS_CC);
	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
	if (result->op_type == IS_CV) {
		init_op(&opline TSRMLS_CC);

		opline.opcode = ZEND_FETCH_W;
		opline.result.op_type = IS_VAR;
		opline.result.u.EA.type = 0;
		opline.result.u.var = get_temporary_variable(CG(active_op_array));
		opline.op1.op_type = IS_CONST;
		opline.op1.u.constant.type = IS_STRING;
		opline.op1.u.constant.value.str.val = estrdup(CG(active_op_array)->vars[result->u.var].name);
		opline.op1.u.constant.value.str.len = CG(active_op_array)->vars[result->u.var].name_len;
		SET_UNUSED(opline.op2);
		opline.op2 = class_node;
		opline.op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
		*result = opline.result;

		zend_llist_add_element(fetch_list_ptr, &opline);
	} else {
		le = fetch_list_ptr->head;

		opline_ptr = (zend_op *)le->data;
		if (opline_ptr->opcode != ZEND_FETCH_W && opline_ptr->op1.op_type == IS_CV) {
			init_op(&opline TSRMLS_CC);
			opline.opcode = ZEND_FETCH_W;
			opline.result.op_type = IS_VAR;
			opline.result.u.EA.type = 0;
			opline.result.u.var = get_temporary_variable(CG(active_op_array));
			opline.op1.op_type = IS_CONST;
			opline.op1.u.constant.type = IS_STRING;
			opline.op1.u.constant.value.str.val = estrdup(CG(active_op_array)->vars[opline_ptr->op1.u.var].name);
			opline.op1.u.constant.value.str.len = CG(active_op_array)->vars[opline_ptr->op1.u.var].name_len;
			SET_UNUSED(opline.op2);
			opline.op2 = class_node;
			opline.op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
			opline_ptr->op1 = opline.result;

			zend_llist_prepend_element(fetch_list_ptr, &opline);
		} else {
			opline_ptr->op2 = class_node;
			opline_ptr->op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
		}
	}
}
/* }}} */

void fetch_array_begin(znode *result, znode *varname, znode *first_dim TSRMLS_DC) /* {{{ */
{
	fetch_simple_variable(result, varname, 1 TSRMLS_CC);

	fetch_array_dim(result, result, first_dim TSRMLS_CC);
}
/* }}} */

void fetch_array_dim(znode *result, const znode *parent, const znode *dim TSRMLS_DC) /* {{{ */
{
	zend_op opline;
	zend_llist *fetch_list_ptr;

	init_op(&opline TSRMLS_CC);
	opline.opcode = ZEND_FETCH_DIM_W;	/* the backpatching routine assumes W */
	opline.result.op_type = IS_VAR;
	opline.result.u.EA.type = 0;
	opline.result.u.var = get_temporary_variable(CG(active_op_array));
	opline.op1 = *parent;
	opline.op2 = *dim;
	opline.extended_value = ZEND_FETCH_STANDARD;
	*result = opline.result;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
	zend_llist_add_element(fetch_list_ptr, &opline);
}
/* }}} */

void fetch_string_offset(znode *result, const znode *parent, const znode *offset TSRMLS_DC) /* {{{ */
{
	fetch_array_dim(result, parent, offset TSRMLS_CC);
}
/* }}} */

void zend_do_print(znode *result, const znode *arg TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->opcode = ZEND_PRINT;
	opline->op1 = *arg;
	SET_UNUSED(opline->op2);
	*result = opline->result;
}
/* }}} */

void zend_do_echo(const znode *arg TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ECHO;
	opline->op1 = *arg;
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_abstract_method(const znode *function_name, znode *modifiers, const znode *body TSRMLS_DC) /* {{{ */
{
	char *method_type;

	if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
		Z_LVAL(modifiers->u.constant) |= ZEND_ACC_ABSTRACT;
		method_type = "Interface";
	} else {
		method_type = "Abstract";
	}

	if (modifiers->u.constant.value.lval & ZEND_ACC_ABSTRACT) {
		if(modifiers->u.constant.value.lval & ZEND_ACC_PRIVATE) {
			zend_error(E_COMPILE_ERROR, "%s function %s::%s() cannot be declared private", method_type, CG(active_class_entry)->name, function_name->u.constant.value.str.val);
		}
		if (Z_LVAL(body->u.constant) == ZEND_ACC_ABSTRACT) {
			zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

			opline->opcode = ZEND_RAISE_ABSTRACT_ERROR;
			SET_UNUSED(opline->op1);
			SET_UNUSED(opline->op2);
		} else {
			/* we had code in the function body */
			zend_error(E_COMPILE_ERROR, "%s function %s::%s() cannot contain body", method_type, CG(active_class_entry)->name, function_name->u.constant.value.str.val);
		}
	} else {
		if (body->u.constant.value.lval == ZEND_ACC_ABSTRACT) {
			zend_error(E_COMPILE_ERROR, "Non-abstract method %s::%s() must contain body", CG(active_class_entry)->name, function_name->u.constant.value.str.val);
		}
	}
}
/* }}} */

static zend_bool opline_is_fetch_this(const zend_op *opline TSRMLS_DC) /* {{{ */
{
	if ((opline->opcode == ZEND_FETCH_W) && (opline->op1.op_type == IS_CONST)
		&& (opline->op1.u.constant.type == IS_STRING)
		&& (opline->op1.u.constant.value.str.len == (sizeof("this")-1))
		&& !memcmp(opline->op1.u.constant.value.str.val, "this", sizeof("this"))) {
		return 1;
	} else {
		return 0;
	}
}
/* }}} */

void zend_do_assign(znode *result, znode *variable, const znode *value TSRMLS_DC) /* {{{ */
{
	int last_op_number;
	zend_op *opline;

	if (value->op_type == IS_CV) {
		zend_llist *fetch_list_ptr;

		zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
		if (fetch_list_ptr && fetch_list_ptr->head) {
			opline = (zend_op *)fetch_list_ptr->head->data;

			if (opline->opcode == ZEND_FETCH_DIM_W &&
			    opline->op1.op_type == IS_CV &&
			    opline->op1.u.var == value->u.var) {

				opline = get_next_op(CG(active_op_array) TSRMLS_CC);
				opline->opcode = ZEND_FETCH_R;
				opline->result.op_type = IS_VAR;
				opline->result.u.EA.type = 0;
				opline->result.u.var = get_temporary_variable(CG(active_op_array));
				opline->op1.op_type = IS_CONST;
				ZVAL_STRINGL(&opline->op1.u.constant,
					CG(active_op_array)->vars[value->u.var].name, 
					CG(active_op_array)->vars[value->u.var].name_len, 1);
				SET_UNUSED(opline->op2);
				opline->op2.u.EA.type = ZEND_FETCH_LOCAL;
				value = &opline->result;
			}
		}
	}

	zend_do_end_variable_parse(variable, BP_VAR_W, 0 TSRMLS_CC);

	last_op_number = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (variable->op_type == IS_CV) {
		if (variable->u.var == CG(active_op_array)->this_var) {
			zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
	    }
	} else if (variable->op_type == IS_VAR) {
		int n = 0;

		while (last_op_number - n > 0) {
			zend_op *last_op;

			last_op = &CG(active_op_array)->opcodes[last_op_number-n-1];

			if (last_op->result.op_type == IS_VAR &&
			    last_op->result.u.var == variable->u.var) {
				if (last_op->opcode == ZEND_FETCH_OBJ_W) {
					if (n > 0) {
						int opline_no = (opline-CG(active_op_array)->opcodes)/sizeof(*opline);
						*opline = *last_op;
						MAKE_NOP(last_op);
						/* last_op = opline; */
						opline = get_next_op(CG(active_op_array) TSRMLS_CC);
						/* get_next_op can realloc, we need to move last_op */
						last_op = &CG(active_op_array)->opcodes[opline_no];
					}
					last_op->opcode = ZEND_ASSIGN_OBJ;
					zend_do_op_data(opline, value TSRMLS_CC);
					SET_UNUSED(opline->result);
					*result = last_op->result;
					return;
				} else if (last_op->opcode == ZEND_FETCH_DIM_W) {
					if (n > 0) {
						int opline_no = (opline-CG(active_op_array)->opcodes)/sizeof(*opline);
						*opline = *last_op;
						MAKE_NOP(last_op);
						/* last_op = opline; */
						/* TBFixed: this can realloc opcodes, leaving last_op pointing wrong */
						opline = get_next_op(CG(active_op_array) TSRMLS_CC);
						/* get_next_op can realloc, we need to move last_op */
						last_op = &CG(active_op_array)->opcodes[opline_no];
					}
					last_op->opcode = ZEND_ASSIGN_DIM;
					zend_do_op_data(opline, value TSRMLS_CC);
					opline->op2.u.var = get_temporary_variable(CG(active_op_array));
					opline->op2.u.EA.type = 0;
					opline->op2.op_type = IS_VAR;
					SET_UNUSED(opline->result);
					*result = last_op->result;
					return;
				} else if (opline_is_fetch_this(last_op TSRMLS_CC)) {
					zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
				} else {
					break;
				}
			}
			n++;
		}
	}

	opline->opcode = ZEND_ASSIGN;
	opline->op1 = *variable;
	opline->op2 = *value;
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}
/* }}} */

static inline zend_bool zend_is_function_or_method_call(const znode *variable) /* {{{ */
{
	zend_uint type = variable->u.EA.type;

	return  ((type & ZEND_PARSED_METHOD_CALL) || (type == ZEND_PARSED_FUNCTION_CALL));
}
/* }}} */

void zend_do_assign_ref(znode *result, const znode *lvar, const znode *rvar TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if (lvar->op_type == IS_CV) {
		if (lvar->u.var == CG(active_op_array)->this_var) {
 			zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
	    }
	} else if (lvar->op_type == IS_VAR) {
		int last_op_number = get_next_op_number(CG(active_op_array));

		if (last_op_number > 0) {
			opline = &CG(active_op_array)->opcodes[last_op_number-1];
			if (opline_is_fetch_this(opline TSRMLS_CC)) {
	 			zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
			}
 		}
 	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_ASSIGN_REF;
	if (zend_is_function_or_method_call(rvar)) {
		opline->extended_value = ZEND_RETURNS_FUNCTION;
	} else if (rvar->u.EA.type & ZEND_PARSED_NEW) {
		opline->extended_value = ZEND_RETURNS_NEW;
	} else {
		opline->extended_value = 0;
	}
	if (result) {
		opline->result.op_type = IS_VAR;
		opline->result.u.EA.type = 0;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		*result = opline->result;
	} else {
		/* SET_UNUSED(opline->result); */
		opline->result.u.EA.type |= EXT_TYPE_UNUSED;
	}
	opline->op1 = *lvar;
	opline->op2 = *rvar;
}
/* }}} */

static inline void do_begin_loop(TSRMLS_D) /* {{{ */
{
	zend_brk_cont_element *brk_cont_element;
	int parent;

	parent = CG(active_op_array)->current_brk_cont;
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->last_brk_cont;
	brk_cont_element = get_next_brk_cont_element(CG(active_op_array));
	brk_cont_element->start = get_next_op_number(CG(active_op_array));
	brk_cont_element->parent = parent;
}
/* }}} */

static inline void do_end_loop(int cont_addr, int has_loop_var TSRMLS_DC) /* {{{ */
{
	if (!has_loop_var) {
		/* The start fileld is used to free temporary variables in case of exceptions.
		 * We won't try to free something of we don't have loop variable.
		 */
		CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].start = -1;
	}
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].cont = cont_addr;
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].brk = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].parent;
}
/* }}} */

void zend_do_while_cond(const znode *expr, znode *close_bracket_token TSRMLS_DC) /* {{{ */
{
	int while_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *expr;
	close_bracket_token->u.opline_num = while_cond_op_number;
	SET_UNUSED(opline->op2);

	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_while_end(const znode *while_token, const znode *close_bracket_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	/* add unconditional jump */
	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = while_token->u.opline_num;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	/* update while's conditional jmp */
	CG(active_op_array)->opcodes[close_bracket_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));

	do_end_loop(while_token->u.opline_num, 0 TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_for_cond(const znode *expr, znode *second_semicolon_token TSRMLS_DC) /* {{{ */
{
	int for_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZNZ;
	opline->op1 = *expr;  /* the conditional expression */
	second_semicolon_token->u.opline_num = for_cond_op_number;
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_for_before_statement(const znode *cond_start, const znode *second_semicolon_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = cond_start->u.opline_num;
	CG(active_op_array)->opcodes[second_semicolon_token->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	do_begin_loop(TSRMLS_C);

	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_for_end(const znode *second_semicolon_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = second_semicolon_token->u.opline_num+1;
	CG(active_op_array)->opcodes[second_semicolon_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	do_end_loop(second_semicolon_token->u.opline_num+1, 0 TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_pre_incdec(znode *result, const znode *op1, zend_uchar op TSRMLS_DC) /* {{{ */
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		if (last_op->opcode == ZEND_FETCH_OBJ_RW) {
			last_op->opcode = (op==ZEND_PRE_INC)?ZEND_PRE_INC_OBJ:ZEND_PRE_DEC_OBJ;
			last_op->result.op_type = IS_VAR;
			last_op->result.u.EA.type = 0;
			last_op->result.u.var = get_temporary_variable(CG(active_op_array));
			*result = last_op->result;
			return;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = op;
	opline->op1 = *op1;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}
/* }}} */

void zend_do_post_incdec(znode *result, const znode *op1, zend_uchar op TSRMLS_DC) /* {{{ */
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		if (last_op->opcode == ZEND_FETCH_OBJ_RW) {
			last_op->opcode = (op==ZEND_POST_INC)?ZEND_POST_INC_OBJ:ZEND_POST_DEC_OBJ;
			last_op->result.op_type = IS_TMP_VAR;
			last_op->result.u.var = get_temporary_variable(CG(active_op_array));
			*result = last_op->result;
			return;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = op;
	opline->op1 = *op1;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}
/* }}} */

void zend_do_if_cond(const znode *cond, znode *closing_bracket_token TSRMLS_DC) /* {{{ */
{
	int if_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *cond;
	closing_bracket_token->u.opline_num = if_cond_op_number;
	SET_UNUSED(opline->op2);
	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_if_after_statement(const znode *closing_bracket_token, unsigned char initialize TSRMLS_DC) /* {{{ */
{
	int if_end_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_llist *jmp_list_ptr;

	opline->opcode = ZEND_JMP;
	/* save for backpatching */
	if (initialize) {
		zend_llist jmp_list;

		zend_llist_init(&jmp_list, sizeof(int), NULL, 0);
		zend_stack_push(&CG(bp_stack), (void *) &jmp_list, sizeof(zend_llist));
	}
	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	zend_llist_add_element(jmp_list_ptr, &if_end_op_number);

	CG(active_op_array)->opcodes[closing_bracket_token->u.opline_num].op2.u.opline_num = if_end_op_number+1;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_if_end(TSRMLS_D) /* {{{ */
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_llist *jmp_list_ptr;
	zend_llist_element *le;

	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	for (le=jmp_list_ptr->head; le; le = le->next) {
		CG(active_op_array)->opcodes[*((int *) le->data)].op1.u.opline_num = next_op_number;
	}
	zend_llist_destroy(jmp_list_ptr);
	zend_stack_del_top(&CG(bp_stack));
	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_check_writable_variable(const znode *variable) /* {{{ */
{
	zend_uint type = variable->u.EA.type;

	if (type & ZEND_PARSED_METHOD_CALL) {
		zend_error(E_COMPILE_ERROR, "Can't use method return value in write context");
	}
	if (type == ZEND_PARSED_FUNCTION_CALL) {
		zend_error(E_COMPILE_ERROR, "Can't use function return value in write context");
	}
}
/* }}} */

void zend_do_begin_variable_parse(TSRMLS_D) /* {{{ */
{
	zend_llist fetch_list;

	zend_llist_init(&fetch_list, sizeof(zend_op), NULL, 0);
	zend_stack_push(&CG(bp_stack), (void *) &fetch_list, sizeof(zend_llist));
}
/* }}} */

void zend_do_end_variable_parse(znode *variable, int type, int arg_offset TSRMLS_DC) /* {{{ */
{
	zend_llist *fetch_list_ptr;
	zend_llist_element *le;
	zend_op *opline = NULL;
	zend_op *opline_ptr;
	zend_uint this_var = -1;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	le = fetch_list_ptr->head;

	/* TODO: $foo->x->y->z = 1 should fetch "x" and "y" for R or RW, not just W */

	if (le) {
		opline_ptr = (zend_op *)le->data;
		if (opline_is_fetch_this(opline_ptr TSRMLS_CC)) {
			/* convert to FETCH_?(this) into IS_CV */
			if (CG(active_op_array)->last == 0 ||
			    CG(active_op_array)->opcodes[CG(active_op_array)->last-1].opcode != ZEND_BEGIN_SILENCE) {

				this_var = opline_ptr->result.u.var;
				if (CG(active_op_array)->this_var == -1) {
					CG(active_op_array)->this_var = lookup_cv(CG(active_op_array), Z_STRVAL(opline_ptr->op1.u.constant), Z_STRLEN(opline_ptr->op1.u.constant));
				} else {
					efree(Z_STRVAL(opline_ptr->op1.u.constant));
				}
				le = le->next;
				if (variable->op_type == IS_VAR &&
				    variable->u.var == this_var) {
					variable->op_type = IS_CV;
					variable->u.var = CG(active_op_array)->this_var;
				}
		    } else if (CG(active_op_array)->this_var == -1) {
				CG(active_op_array)->this_var = lookup_cv(CG(active_op_array), estrndup("this", sizeof("this")-1), sizeof("this")-1);
			}
		}

		while (le) {
			opline_ptr = (zend_op *)le->data;
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			memcpy(opline, opline_ptr, sizeof(zend_op));
			if (opline->op1.op_type == IS_VAR &&
			    opline->op1.u.var == this_var) {
				opline->op1.op_type = IS_CV;
				opline->op1.u.var = CG(active_op_array)->this_var;
			}
			switch (type) {
				case BP_VAR_R:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for reading");
					}
					opline->opcode -= 3;
					break;
				case BP_VAR_W:
					break;
				case BP_VAR_RW:
					opline->opcode += 3;
					break;
				case BP_VAR_IS:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for reading");
					}
					opline->opcode += 6; /* 3+3 */
					break;
				case BP_VAR_FUNC_ARG:
					opline->opcode += 9; /* 3+3+3 */
					opline->extended_value = arg_offset;
					break;
				case BP_VAR_UNSET:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for unsetting");
					}
					opline->opcode += 12; /* 3+3+3+3 */
					break;
			}
			le = le->next;
		}
		if (opline && type == BP_VAR_W && arg_offset) {
			opline->extended_value = ZEND_FETCH_MAKE_REF;
		}
	}
	zend_llist_destroy(fetch_list_ptr);
	zend_stack_del_top(&CG(bp_stack));
}
/* }}} */

void zend_do_add_string(znode *result, const znode *op1, znode *op2 TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if (Z_STRLEN(op2->u.constant) > 1) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_ADD_STRING;
	} else if (Z_STRLEN(op2->u.constant) == 1) {
		int ch = *Z_STRVAL(op2->u.constant);

		/* Free memory and use ZEND_ADD_CHAR in case of 1 character strings */
		efree(Z_STRVAL(op2->u.constant));
		ZVAL_LONG(&op2->u.constant, ch);
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_ADD_CHAR;
	} else { /* String can be empty after a variable at the end of a heredoc */
		efree(Z_STRVAL(op2->u.constant));
		return;
	}

	if (op1) {
		opline->op1 = *op1;
		opline->result = *op1;
	} else {
		SET_UNUSED(opline->op1);
		opline->result.op_type = IS_TMP_VAR;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
	}
	opline->op2 = *op2;
	*result = opline->result;
}
/* }}} */

void zend_do_add_variable(znode *result, const znode *op1, const znode *op2 TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ADD_VAR;

	if (op1) {
		opline->op1 = *op1;
		opline->result = *op1;
	} else {
		SET_UNUSED(opline->op1);
		opline->result.op_type = IS_TMP_VAR;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
	}
	opline->op2 = *op2;
	*result = opline->result;
}
/* }}} */

void zend_do_free(znode *op1 TSRMLS_DC) /* {{{ */
{
	if (op1->op_type==IS_TMP_VAR) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_FREE;
		opline->op1 = *op1;
		SET_UNUSED(opline->op2);
	} else if (op1->op_type==IS_VAR) {
		zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];

		while (opline->opcode == ZEND_END_SILENCE || opline->opcode == ZEND_EXT_FCALL_END || opline->opcode == ZEND_OP_DATA) {
			opline--;
		}
		if (opline->result.op_type == IS_VAR
			&& opline->result.u.var == op1->u.var) {
			opline->result.u.EA.type |= EXT_TYPE_UNUSED;
		} else {
			while (opline>CG(active_op_array)->opcodes) {
				if (opline->opcode == ZEND_FETCH_DIM_R
				    && opline->op1.op_type == IS_VAR
				    && opline->op1.u.var == op1->u.var) {
					/* This should the end of a list() construct
					 * Mark its result as unused
					 */
					opline->extended_value = ZEND_FETCH_STANDARD;
					break;
				} else if (opline->result.op_type==IS_VAR
					&& opline->result.u.var == op1->u.var) {
					if (opline->opcode == ZEND_NEW) {
						opline->result.u.EA.type |= EXT_TYPE_UNUSED;
					}
					break;
				}
				opline--;
			}
		}
	} else if (op1->op_type == IS_CONST) {
		zval_dtor(&op1->u.constant);
	}
}
/* }}} */

int zend_do_verify_access_types(const znode *current_access_type, const znode *new_modifier) /* {{{ */
{
	if ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_PPP_MASK)
		&& (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_PPP_MASK)) {
		zend_error(E_COMPILE_ERROR, "Multiple access type modifiers are not allowed");
	}
	if ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_ABSTRACT)
		&& (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_ABSTRACT)) {
		zend_error(E_COMPILE_ERROR, "Multiple abstract modifiers are not allowed");
	}
	if ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_STATIC)
		&& (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_STATIC)) {
		zend_error(E_COMPILE_ERROR, "Multiple static modifiers are not allowed");
	}
	if ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_FINAL)
		&& (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_FINAL)) {
		zend_error(E_COMPILE_ERROR, "Multiple final modifiers are not allowed");
	}
	if (((Z_LVAL(current_access_type->u.constant) | Z_LVAL(new_modifier->u.constant)) & (ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL)) == (ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL)) {
		zend_error(E_COMPILE_ERROR, "Cannot use the final modifier on an abstract class member");
	}
	return (Z_LVAL(current_access_type->u.constant) | Z_LVAL(new_modifier->u.constant));
}
/* }}} */

void zend_do_begin_function_declaration(znode *function_token, znode *function_name, int is_method, int return_reference, znode *fn_flags_znode TSRMLS_DC) /* {{{ */
{
	zend_op_array op_array;
	char *name = function_name->u.constant.value.str.val;
	int name_len = function_name->u.constant.value.str.len;
	int function_begin_line = function_token->u.opline_num;
	zend_uint fn_flags;
	char *lcname;
	zend_bool orig_interactive;
	ALLOCA_FLAG(use_heap)

	if (is_method) {
		if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
			if ((Z_LVAL(fn_flags_znode->u.constant) & ~(ZEND_ACC_STATIC|ZEND_ACC_PUBLIC))) {
				zend_error(E_COMPILE_ERROR, "Access type for interface method %s::%s() must be omitted", CG(active_class_entry)->name, function_name->u.constant.value.str.val);
			}
			Z_LVAL(fn_flags_znode->u.constant) |= ZEND_ACC_ABSTRACT; /* propagates to the rest of the parser */
		}
		fn_flags = Z_LVAL(fn_flags_znode->u.constant); /* must be done *after* the above check */
	} else {
		fn_flags = 0;
	}
	if ((fn_flags & ZEND_ACC_STATIC) && (fn_flags & ZEND_ACC_ABSTRACT) && !(CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error(E_STRICT, "Static function %s%s%s() should not be abstract", is_method ? CG(active_class_entry)->name : "", is_method ? "::" : "", Z_STRVAL(function_name->u.constant));
	}

	function_token->u.op_array = CG(active_op_array);
	lcname = zend_str_tolower_dup(name, name_len);

	orig_interactive = CG(interactive);
	CG(interactive) = 0;
	init_op_array(&op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE TSRMLS_CC);
	CG(interactive) = orig_interactive;

	op_array.function_name = name;
	op_array.return_reference = return_reference;
	op_array.fn_flags |= fn_flags;
	op_array.pass_rest_by_reference = 0;

	op_array.scope = is_method?CG(active_class_entry):NULL;
	op_array.prototype = NULL;

	op_array.line_start = zend_get_compiled_lineno(TSRMLS_C);

	if (is_method) {
		if (zend_hash_add(&CG(active_class_entry)->function_table, lcname, name_len+1, &op_array, sizeof(zend_op_array), (void **) &CG(active_op_array)) == FAILURE) {
			zend_error(E_COMPILE_ERROR, "Cannot redeclare %s::%s()", CG(active_class_entry)->name, name);
		}

		if (fn_flags & ZEND_ACC_ABSTRACT) {
			CG(active_class_entry)->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}

		if (!(fn_flags & ZEND_ACC_PPP_MASK)) {
			fn_flags |= ZEND_ACC_PUBLIC;
		}

		if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
			if ((name_len == sizeof(ZEND_CALL_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __call() must have public visibility and cannot be static");
				}
			} else if ((name_len == sizeof(ZEND_CALLSTATIC_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CALLSTATIC_FUNC_NAME, sizeof(ZEND_CALLSTATIC_FUNC_NAME)-1))) {
				if ((fn_flags & (ZEND_ACC_PPP_MASK ^ ZEND_ACC_PUBLIC)) || (fn_flags & ZEND_ACC_STATIC) == 0) {
					zend_error(E_WARNING, "The magic method __callStatic() must have public visibility and be static");
				}
			} else if ((name_len == sizeof(ZEND_GET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_GET_FUNC_NAME, sizeof(ZEND_GET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __get() must have public visibility and cannot be static");
				}
			} else if ((name_len == sizeof(ZEND_SET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_SET_FUNC_NAME, sizeof(ZEND_SET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __set() must have public visibility and cannot be static");
				}
			} else if ((name_len == sizeof(ZEND_UNSET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_UNSET_FUNC_NAME, sizeof(ZEND_UNSET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __unset() must have public visibility and cannot be static");
				}
			} else if ((name_len == sizeof(ZEND_ISSET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_ISSET_FUNC_NAME, sizeof(ZEND_ISSET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __isset() must have public visibility and cannot be static");
				}
			} else if ((name_len == sizeof(ZEND_TOSTRING_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_TOSTRING_FUNC_NAME, sizeof(ZEND_TOSTRING_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __toString() must have public visibility and cannot be static");
				}
			}
		} else {
			char *class_lcname;
			
			class_lcname = do_alloca(CG(active_class_entry)->name_length + 1, use_heap);
			zend_str_tolower_copy(class_lcname, CG(active_class_entry)->name, CG(active_class_entry)->name_length);
			/* Improve after RC: cache the lowercase class name */

			if ((CG(active_class_entry)->name_length == name_len) && (!memcmp(class_lcname, lcname, name_len))) {
				if (!CG(active_class_entry)->constructor) {
					CG(active_class_entry)->constructor = (zend_function *) CG(active_op_array);
				}
			} else if ((name_len == sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)))) {
				if (CG(active_class_entry)->constructor) {
					zend_error(E_STRICT, "Redefining already defined constructor for class %s", CG(active_class_entry)->name);
				}
				CG(active_class_entry)->constructor = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_DESTRUCTOR_FUNC_NAME, sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1))) {
				CG(active_class_entry)->destructor = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_CLONE_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME)-1))) {
				CG(active_class_entry)->clone = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_CALL_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __call() must have public visibility and cannot be static");
				}
				CG(active_class_entry)->__call = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_CALLSTATIC_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_CALLSTATIC_FUNC_NAME, sizeof(ZEND_CALLSTATIC_FUNC_NAME)-1))) {
				if ((fn_flags & (ZEND_ACC_PPP_MASK ^ ZEND_ACC_PUBLIC)) || (fn_flags & ZEND_ACC_STATIC) == 0) {
					zend_error(E_WARNING, "The magic method __callStatic() must have public visibility and be static");
				}
				CG(active_class_entry)->__callstatic = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_GET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_GET_FUNC_NAME, sizeof(ZEND_GET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __get() must have public visibility and cannot be static");
				}
				CG(active_class_entry)->__get = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_SET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_SET_FUNC_NAME, sizeof(ZEND_SET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __set() must have public visibility and cannot be static");
				}
				CG(active_class_entry)->__set = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_UNSET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_UNSET_FUNC_NAME, sizeof(ZEND_UNSET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __unset() must have public visibility and cannot be static");
				}
				CG(active_class_entry)->__unset = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_ISSET_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_ISSET_FUNC_NAME, sizeof(ZEND_ISSET_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __isset() must have public visibility and cannot be static");
				}
				CG(active_class_entry)->__isset = (zend_function *) CG(active_op_array);
			} else if ((name_len == sizeof(ZEND_TOSTRING_FUNC_NAME)-1) && (!memcmp(lcname, ZEND_TOSTRING_FUNC_NAME, sizeof(ZEND_TOSTRING_FUNC_NAME)-1))) {
				if (fn_flags & ((ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC) ^ ZEND_ACC_PUBLIC)) {
					zend_error(E_WARNING, "The magic method __toString() must have public visibility and cannot be static");
				}				
				CG(active_class_entry)->__tostring = (zend_function *) CG(active_op_array);
			} else if (!(fn_flags & ZEND_ACC_STATIC)) {
				CG(active_op_array)->fn_flags |= ZEND_ACC_ALLOW_STATIC;
			}
			free_alloca(class_lcname, use_heap);
		}

		efree(lcname);
	} else {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		if (CG(current_namespace)) {
			/* Prefix function name with current namespcae name */
			znode tmp;

			tmp.u.constant = *CG(current_namespace);
			zval_copy_ctor(&tmp.u.constant);
			zend_do_build_namespace_name(&tmp, &tmp, function_name TSRMLS_CC);
			op_array.function_name = Z_STRVAL(tmp.u.constant);
			efree(lcname);
			name_len = Z_STRLEN(tmp.u.constant);
			lcname = zend_str_tolower_dup(Z_STRVAL(tmp.u.constant), name_len);
		}

		opline->opcode = ZEND_DECLARE_FUNCTION;
		opline->op1.op_type = IS_CONST;
		build_runtime_defined_function_key(&opline->op1.u.constant, lcname, name_len TSRMLS_CC);
		opline->op2.op_type = IS_CONST;
		opline->op2.u.constant.type = IS_STRING;
		opline->op2.u.constant.value.str.val = lcname;
		opline->op2.u.constant.value.str.len = name_len;
		Z_SET_REFCOUNT(opline->op2.u.constant, 1);
		opline->extended_value = ZEND_DECLARE_FUNCTION;
		zend_hash_update(CG(function_table), opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, &op_array, sizeof(zend_op_array), (void **) &CG(active_op_array));
	}

	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_EXT_NOP;
		opline->lineno = function_begin_line;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
	}

	{
		/* Push a seperator to the switch and foreach stacks */
		zend_switch_entry switch_entry;

		switch_entry.cond.op_type = IS_UNUSED;
		switch_entry.default_case = 0;
		switch_entry.control_var = 0;

		zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry, sizeof(switch_entry));

		{
			/* Foreach stack separator */
			zend_op dummy_opline;

			dummy_opline.result.op_type = IS_UNUSED;
			dummy_opline.op1.op_type = IS_UNUSED;

			zend_stack_push(&CG(foreach_copy_stack), (void *) &dummy_opline, sizeof(zend_op));
		}
	}

	if (CG(doc_comment)) {
		CG(active_op_array)->doc_comment = CG(doc_comment);
		CG(active_op_array)->doc_comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}

	zend_stack_push(&CG(labels_stack), (void *) &CG(labels), sizeof(HashTable*));
	CG(labels) = NULL;
}
/* }}} */

void zend_do_begin_lambda_function_declaration(znode *result, znode *function_token, int return_reference TSRMLS_DC) /* {{{ */
{
	znode          function_name;
	zend_op_array *current_op_array = CG(active_op_array);
	int            current_op_number = get_next_op_number(CG(active_op_array));
	zend_op       *current_op;

	function_name.op_type = IS_CONST;
	ZVAL_STRINGL(&function_name.u.constant, "{closure}", sizeof("{closure}")-1, 1);

	zend_do_begin_function_declaration(function_token, &function_name, 0, return_reference, NULL TSRMLS_CC);

	result->op_type = IS_TMP_VAR;
	result->u.var = get_temporary_variable(current_op_array);

	current_op = &current_op_array->opcodes[current_op_number];
	current_op->opcode = ZEND_DECLARE_LAMBDA_FUNCTION;
	zval_dtor(&current_op->op2.u.constant);
	ZVAL_LONG(&current_op->op2.u.constant, zend_hash_func(Z_STRVAL(current_op->op1.u.constant), Z_STRLEN(current_op->op1.u.constant)));
	current_op->result = *result;
	CG(active_op_array)->fn_flags |= ZEND_ACC_CLOSURE;
}
/* }}} */

void zend_do_handle_exception(TSRMLS_D) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_HANDLE_EXCEPTION;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_end_function_declaration(const znode *function_token TSRMLS_DC) /* {{{ */
{
	char lcname[16];
	int name_len;

	zend_do_extended_info(TSRMLS_C);
	zend_do_return(NULL, 0 TSRMLS_CC);

	pass_two(CG(active_op_array) TSRMLS_CC);
	zend_release_labels(TSRMLS_C);

	if (CG(active_class_entry)) {
		zend_check_magic_method_implementation(CG(active_class_entry), (zend_function*)CG(active_op_array), E_COMPILE_ERROR TSRMLS_CC);
	} else {
		/* we don't care if the function name is longer, in fact lowercasing only 
		 * the beginning of the name speeds up the check process */
		name_len = strlen(CG(active_op_array)->function_name);
		zend_str_tolower_copy(lcname, CG(active_op_array)->function_name, MIN(name_len, sizeof(lcname)-1));
		lcname[sizeof(lcname)-1] = '\0'; /* zend_str_tolower_copy won't necessarily set the zero byte */
		if (name_len == sizeof(ZEND_AUTOLOAD_FUNC_NAME) - 1 && !memcmp(lcname, ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME)) && CG(active_op_array)->num_args != 1) {
			zend_error(E_COMPILE_ERROR, "%s() must take exactly 1 argument", ZEND_AUTOLOAD_FUNC_NAME);
		}		
	}

	CG(active_op_array)->line_end = zend_get_compiled_lineno(TSRMLS_C);
	CG(active_op_array) = function_token->u.op_array;


	/* Pop the switch and foreach seperators */
	zend_stack_del_top(&CG(switch_cond_stack));
	zend_stack_del_top(&CG(foreach_copy_stack));
}
/* }}} */

void zend_do_receive_arg(zend_uchar op, const znode *var, const znode *offset, const znode *initialization, znode *class_type, const znode *varname, zend_uchar pass_by_reference TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	zend_arg_info *cur_arg_info;

	if (class_type->op_type == IS_CONST &&
	    Z_TYPE(class_type->u.constant) == IS_STRING &&
	    Z_STRLEN(class_type->u.constant) == 0) {
		/* Usage of namespace as class name not in namespace */
		zval_dtor(&class_type->u.constant);
		zend_error(E_COMPILE_ERROR, "Cannot use 'namespace' as a class name");
		return;
	}

	if (var->op_type == IS_CV &&
	    var->u.var == CG(active_op_array)->this_var &&
	    (CG(active_op_array)->fn_flags & ZEND_ACC_STATIC) == 0) {
 		zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
	} else if (var->op_type == IS_VAR &&
	    CG(active_op_array)->scope &&
		((CG(active_op_array)->fn_flags & ZEND_ACC_STATIC) == 0) &&
		(Z_TYPE(varname->u.constant) == IS_STRING) &&
		(Z_STRLEN(varname->u.constant) == sizeof("this")-1) &&
		(memcmp(Z_STRVAL(varname->u.constant), "this", sizeof("this")) == 0)) {
		zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	CG(active_op_array)->num_args++;
	opline->opcode = op;
	opline->result = *var;
	opline->op1 = *offset;
	if (op == ZEND_RECV_INIT) {
		opline->op2 = *initialization;
	} else {
		CG(active_op_array)->required_num_args = CG(active_op_array)->num_args;
		SET_UNUSED(opline->op2);
	}
	CG(active_op_array)->arg_info = erealloc(CG(active_op_array)->arg_info, sizeof(zend_arg_info)*(CG(active_op_array)->num_args));
	cur_arg_info = &CG(active_op_array)->arg_info[CG(active_op_array)->num_args-1];
	cur_arg_info->name = estrndup(varname->u.constant.value.str.val, varname->u.constant.value.str.len);
	cur_arg_info->name_len = varname->u.constant.value.str.len;
	cur_arg_info->array_type_hint = 0;
	cur_arg_info->allow_null = 1;
	cur_arg_info->pass_by_reference = pass_by_reference;
	cur_arg_info->class_name = NULL;
	cur_arg_info->class_name_len = 0;

	if (class_type->op_type != IS_UNUSED) {
		cur_arg_info->allow_null = 0;
		if (class_type->u.constant.type == IS_STRING) {
			if (ZEND_FETCH_CLASS_DEFAULT == zend_get_class_fetch_type(Z_STRVAL(class_type->u.constant), Z_STRLEN(class_type->u.constant))) {
				zend_resolve_class_name(class_type, &opline->extended_value, 1 TSRMLS_CC);
			}
			cur_arg_info->class_name = class_type->u.constant.value.str.val;
			cur_arg_info->class_name_len = class_type->u.constant.value.str.len;
			if (op == ZEND_RECV_INIT) {
				if (Z_TYPE(initialization->u.constant) == IS_NULL || (Z_TYPE(initialization->u.constant) == IS_CONSTANT && !strcasecmp(Z_STRVAL(initialization->u.constant), "NULL"))) {
					cur_arg_info->allow_null = 1;
				} else {
					zend_error(E_COMPILE_ERROR, "Default value for parameters with a class type hint can only be NULL");
				}
			}
		} else {
			cur_arg_info->array_type_hint = 1;
			cur_arg_info->class_name = NULL;
			cur_arg_info->class_name_len = 0;
			if (op == ZEND_RECV_INIT) {
				if (Z_TYPE(initialization->u.constant) == IS_NULL || (Z_TYPE(initialization->u.constant) == IS_CONSTANT && !strcasecmp(Z_STRVAL(initialization->u.constant), "NULL"))) {
					cur_arg_info->allow_null = 1;
				} else if (Z_TYPE(initialization->u.constant) != IS_ARRAY && Z_TYPE(initialization->u.constant) != IS_CONSTANT_ARRAY) {
					zend_error(E_COMPILE_ERROR, "Default value for parameters with array type hint can only be an array or NULL");
				}
			}
		}
	}
	opline->result.u.EA.type |= EXT_TYPE_UNUSED;
}
/* }}} */

int zend_do_begin_function_call(znode *function_name, zend_bool check_namespace TSRMLS_DC) /* {{{ */
{
	zend_function *function;
	char *lcname;
	char *is_compound = memchr(Z_STRVAL(function_name->u.constant), '\\', Z_STRLEN(function_name->u.constant));

	zend_resolve_non_class_name(function_name, check_namespace TSRMLS_CC);

	if (check_namespace && CG(current_namespace) && !is_compound) {
			/* We assume we call function from the current namespace
			if it is not prefixed. */

			/* In run-time PHP will check for function with full name and
			internal function with short name */
			zend_do_begin_dynamic_function_call(function_name, 1 TSRMLS_CC);
			return 1;
	} 

	lcname = zend_str_tolower_dup(function_name->u.constant.value.str.val, function_name->u.constant.value.str.len);
	if ((zend_hash_find(CG(function_table), lcname, function_name->u.constant.value.str.len+1, (void **) &function)==FAILURE) ||
	 	((CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS) &&
 		(function->type == ZEND_INTERNAL_FUNCTION))) {
 			zend_do_begin_dynamic_function_call(function_name, 0 TSRMLS_CC);
 			efree(lcname);
 			return 1; /* Dynamic */
 	} 
	efree(function_name->u.constant.value.str.val);
	function_name->u.constant.value.str.val = lcname;
	
	zend_stack_push(&CG(function_call_stack), (void *) &function, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
	return 0;
}
/* }}} */

void zend_do_begin_method_call(znode *left_bracket TSRMLS_DC) /* {{{ */
{
	zend_op *last_op;
	int last_op_number;
	unsigned char *ptr = NULL;

	zend_do_end_variable_parse(left_bracket, BP_VAR_R, 0 TSRMLS_CC);
	zend_do_begin_variable_parse(TSRMLS_C);

	last_op_number = get_next_op_number(CG(active_op_array))-1;
	last_op = &CG(active_op_array)->opcodes[last_op_number];

	if ((last_op->op2.op_type == IS_CONST) && (last_op->op2.u.constant.type == IS_STRING) && (last_op->op2.u.constant.value.str.len == sizeof(ZEND_CLONE_FUNC_NAME)-1)
		&& !zend_binary_strcasecmp(last_op->op2.u.constant.value.str.val, last_op->op2.u.constant.value.str.len, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME)-1)) {
		zend_error(E_COMPILE_ERROR, "Cannot call __clone() method on objects - use 'clone $obj' instead");
	}

	if (last_op->opcode == ZEND_FETCH_OBJ_R) {
		last_op->opcode = ZEND_INIT_METHOD_CALL;
		SET_UNUSED(last_op->result);
		Z_LVAL(left_bracket->u.constant) = ZEND_INIT_FCALL_BY_NAME;
	} else {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_INIT_FCALL_BY_NAME;
		opline->op2 = *left_bracket;
		if (opline->op2.op_type == IS_CONST) {
			opline->op1.op_type = IS_CONST;
			Z_TYPE(opline->op1.u.constant) = IS_STRING;
			Z_STRVAL(opline->op1.u.constant) = zend_str_tolower_dup(Z_STRVAL(opline->op2.u.constant), Z_STRLEN(opline->op2.u.constant));
			Z_STRLEN(opline->op1.u.constant) = Z_STRLEN(opline->op2.u.constant);
			opline->extended_value = zend_hash_func(Z_STRVAL(opline->op1.u.constant), Z_STRLEN(opline->op1.u.constant) + 1);
		} else {
			opline->extended_value = 0;
			SET_UNUSED(opline->op1);
		}
	}

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
}
/* }}} */

void zend_do_clone(znode *result, const znode *expr TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_CLONE;
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}
/* }}} */

void zend_do_begin_dynamic_function_call(znode *function_name, int ns_call TSRMLS_DC) /* {{{ */
{
	unsigned char *ptr = NULL;
	zend_op *opline, *opline2;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	if (ns_call) {
		char *slash;
		int prefix_len, name_len;
		/* In run-time PHP will check for function with full name and
		   internal function with short name */
		opline->opcode = ZEND_INIT_NS_FCALL_BY_NAME;
		opline->op2 = *function_name;
		opline->extended_value = 0;
		opline->op1.op_type = IS_CONST;
		Z_TYPE(opline->op1.u.constant) = IS_STRING;
		Z_STRVAL(opline->op1.u.constant) = zend_str_tolower_dup(Z_STRVAL(opline->op2.u.constant), Z_STRLEN(opline->op2.u.constant));
		Z_STRLEN(opline->op1.u.constant) = Z_STRLEN(opline->op2.u.constant);
		opline->extended_value = zend_hash_func(Z_STRVAL(opline->op1.u.constant), Z_STRLEN(opline->op1.u.constant) + 1);
		slash = zend_memrchr(Z_STRVAL(opline->op1.u.constant), '\\', Z_STRLEN(opline->op1.u.constant));
		prefix_len = slash-Z_STRVAL(opline->op1.u.constant)+1;
		name_len = Z_STRLEN(opline->op1.u.constant)-prefix_len;
		opline2 = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline2->opcode = ZEND_OP_DATA;
		opline2->op1.op_type = IS_CONST;
		Z_TYPE(opline2->op1.u.constant) = IS_LONG;
		if(!slash) {
			zend_error(E_CORE_ERROR, "Namespaced name %s should contain slash", Z_STRVAL(opline->op1.u.constant));
		}
		/* this is the length of namespace prefix */
		Z_LVAL(opline2->op1.u.constant) = prefix_len;
		/* this is the hash of the non-prefixed part, lowercased */
		opline2->extended_value = zend_hash_func(slash+1, name_len+1);
		SET_UNUSED(opline2->op2);
	} else {
		opline->opcode = ZEND_INIT_FCALL_BY_NAME;
		opline->op2 = *function_name;
		if (opline->op2.op_type == IS_CONST) {
			opline->op1.op_type = IS_CONST;
			Z_TYPE(opline->op1.u.constant) = IS_STRING;
			Z_STRVAL(opline->op1.u.constant) = zend_str_tolower_dup(Z_STRVAL(opline->op2.u.constant), Z_STRLEN(opline->op2.u.constant));
			Z_STRLEN(opline->op1.u.constant) = Z_STRLEN(opline->op2.u.constant);
			opline->extended_value = zend_hash_func(Z_STRVAL(opline->op1.u.constant), Z_STRLEN(opline->op1.u.constant) + 1);
		} else {
			opline->extended_value = 0;
			SET_UNUSED(opline->op1);
		}
	}

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
}
/* }}} */

void zend_resolve_non_class_name(znode *element_name, zend_bool check_namespace TSRMLS_DC) /* {{{ */
{
	znode tmp;
	int len;
	zval **ns;
	char *lcname, *compound = memchr(Z_STRVAL(element_name->u.constant), '\\', Z_STRLEN(element_name->u.constant));

	if (Z_STRVAL(element_name->u.constant)[0] == '\\') {
		/* name starts with \ so it is known and unambiguos, nothing to do here but shorten it */
		memmove(Z_STRVAL(element_name->u.constant), Z_STRVAL(element_name->u.constant)+1, Z_STRLEN(element_name->u.constant));
		--Z_STRLEN(element_name->u.constant);
		return;
	}

	if(!check_namespace) {
		return;
	}

	if (compound && CG(current_import)) {
		len = compound - Z_STRVAL(element_name->u.constant);
		lcname = zend_str_tolower_dup(Z_STRVAL(element_name->u.constant), len);
		/* Check if first part of compound name is an import name */
		if (zend_hash_find(CG(current_import), lcname, len+1, (void**)&ns) == SUCCESS) {
			/* Substitute import name */
			tmp.op_type = IS_CONST;
			tmp.u.constant = **ns;
			zval_copy_ctor(&tmp.u.constant);
			len += 1;
			Z_STRLEN(element_name->u.constant) -= len;
			memmove(Z_STRVAL(element_name->u.constant), Z_STRVAL(element_name->u.constant)+len, Z_STRLEN(element_name->u.constant)+1);
			zend_do_build_namespace_name(&tmp, &tmp, element_name TSRMLS_CC);
			*element_name = tmp;
			efree(lcname);
			return;
		}
		efree(lcname);
	}

	if (CG(current_namespace)) {
		tmp = *element_name;
		Z_STRLEN(tmp.u.constant) = sizeof("\\")-1 + Z_STRLEN(element_name->u.constant) + Z_STRLEN_P(CG(current_namespace));
		Z_STRVAL(tmp.u.constant) = (char *) emalloc(Z_STRLEN(tmp.u.constant)+1);
		memcpy(Z_STRVAL(tmp.u.constant), Z_STRVAL_P(CG(current_namespace)), Z_STRLEN_P(CG(current_namespace)));
		memcpy(&(Z_STRVAL(tmp.u.constant)[Z_STRLEN_P(CG(current_namespace))]), "\\", sizeof("\\")-1);
		memcpy(&(Z_STRVAL(tmp.u.constant)[Z_STRLEN_P(CG(current_namespace)) + sizeof("\\")-1]), Z_STRVAL(element_name->u.constant), Z_STRLEN(element_name->u.constant)+1);
		STR_FREE(Z_STRVAL(element_name->u.constant));
		*element_name = tmp;
	}
}
/* }}} */

void zend_resolve_class_name(znode *class_name, ulong *fetch_type, int check_ns_name TSRMLS_DC) /* {{{ */
{
	char *compound;
	char *lcname;
	zval **ns;
	znode tmp;
	int len;

	compound = memchr(Z_STRVAL(class_name->u.constant), '\\', Z_STRLEN(class_name->u.constant));
	if (compound) {
		/* This is a compound class name that contains namespace prefix */
		if (Z_STRVAL(class_name->u.constant)[0] == '\\') {
		    /* The STRING name has "\" prefix */
		    Z_STRLEN(class_name->u.constant) -= 1;
		    memmove(Z_STRVAL(class_name->u.constant), Z_STRVAL(class_name->u.constant)+1, Z_STRLEN(class_name->u.constant)+1);
			Z_STRVAL(class_name->u.constant) = erealloc(
				Z_STRVAL(class_name->u.constant),
				Z_STRLEN(class_name->u.constant) + 1);

			if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(Z_STRVAL(class_name->u.constant), Z_STRLEN(class_name->u.constant))) {
				zend_error(E_COMPILE_ERROR, "'\\%s' is an invalid class name", Z_STRVAL(class_name->u.constant));
			}
		} else { 
			if (CG(current_import)) {
				len = compound - Z_STRVAL(class_name->u.constant);
				lcname = zend_str_tolower_dup(Z_STRVAL(class_name->u.constant), len);
				/* Check if first part of compound name is an import name */
				if (zend_hash_find(CG(current_import), lcname, len+1, (void**)&ns) == SUCCESS) {
					/* Substitute import name */
					tmp.op_type = IS_CONST;
					tmp.u.constant = **ns;
					zval_copy_ctor(&tmp.u.constant);
					len += 1;
					Z_STRLEN(class_name->u.constant) -= len;
					memmove(Z_STRVAL(class_name->u.constant), Z_STRVAL(class_name->u.constant)+len, Z_STRLEN(class_name->u.constant)+1);
					zend_do_build_namespace_name(&tmp, &tmp, class_name TSRMLS_CC);
					*class_name = tmp;
					efree(lcname);
					return;
				}
				efree(lcname);
			}
			/* Here name is not prefixed with \ and not imported */
			if (CG(current_namespace)) {
				tmp.op_type = IS_CONST;
				tmp.u.constant = *CG(current_namespace);
				zval_copy_ctor(&tmp.u.constant);
				zend_do_build_namespace_name(&tmp, &tmp, class_name TSRMLS_CC);
				*class_name = tmp;
			}
		}
	} else if (CG(current_import) || CG(current_namespace)) {
		/* this is a plain name (without \) */
		lcname = zend_str_tolower_dup(Z_STRVAL(class_name->u.constant), Z_STRLEN(class_name->u.constant));

		if (CG(current_import) &&
		    zend_hash_find(CG(current_import), lcname, Z_STRLEN(class_name->u.constant)+1, (void**)&ns) == SUCCESS) {
		    /* The given name is an import name. Substitute it. */
			zval_dtor(&class_name->u.constant);
			class_name->u.constant = **ns;
			zval_copy_ctor(&class_name->u.constant);
		} else if (CG(current_namespace)) {
			/* plain name, no import - prepend current namespace to it */
			tmp.op_type = IS_CONST;
			tmp.u.constant = *CG(current_namespace);
			zval_copy_ctor(&tmp.u.constant);
			zend_do_build_namespace_name(&tmp, &tmp, class_name TSRMLS_CC);
			*class_name = tmp;
		}
		efree(lcname);
	}
}
/* }}} */

void zend_do_fetch_class(znode *result, znode *class_name TSRMLS_DC) /* {{{ */
{
	long fetch_class_op_number;
	zend_op *opline;

	if (class_name->op_type == IS_CONST &&
	    Z_TYPE(class_name->u.constant) == IS_STRING &&
	    Z_STRLEN(class_name->u.constant) == 0) {
		/* Usage of namespace as class name not in namespace */
		zval_dtor(&class_name->u.constant);
		zend_error(E_COMPILE_ERROR, "Cannot use 'namespace' as a class name");
		return;
	}

	fetch_class_op_number = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_FETCH_CLASS;
	SET_UNUSED(opline->op1);
	opline->extended_value = ZEND_FETCH_CLASS_GLOBAL;
	CG(catch_begin) = fetch_class_op_number;
	if (class_name->op_type == IS_CONST) {
		int fetch_type;

		fetch_type = zend_get_class_fetch_type(class_name->u.constant.value.str.val, class_name->u.constant.value.str.len);
		switch (fetch_type) {
			case ZEND_FETCH_CLASS_SELF:
			case ZEND_FETCH_CLASS_PARENT:
			case ZEND_FETCH_CLASS_STATIC:
				SET_UNUSED(opline->op2);
				opline->extended_value = fetch_type;
				zval_dtor(&class_name->u.constant);
				break;
			default:
				zend_resolve_class_name(class_name, &opline->extended_value, 0 TSRMLS_CC);
				opline->op2 = *class_name;
				break;
		}
	} else {
		opline->op2 = *class_name;
	}
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.u.EA.type = opline->extended_value;
	opline->result.op_type = IS_VAR; /* FIXME: Hack so that INIT_FCALL_BY_NAME still knows this is a class */
	*result = opline->result;
}
/* }}} */

void zend_do_label(znode *label TSRMLS_DC) /* {{{ */
{
	zend_op_array *oparray = CG(active_op_array);
	zend_label dest;

	if (!CG(labels)) {
		ALLOC_HASHTABLE(CG(labels));
		zend_hash_init(CG(labels), 4, NULL, NULL, 0);
	}

	dest.brk_cont = oparray->current_brk_cont;
	dest.opline_num = get_next_op_number(oparray);

	if (zend_hash_add(CG(labels), Z_STRVAL(label->u.constant), Z_STRLEN(label->u.constant) + 1, (void**)&dest, sizeof(zend_label), NULL) == FAILURE) {
		zend_error(E_COMPILE_ERROR, "Label '%s' already defined", Z_STRVAL(label->u.constant));
	}

	/* Done with label now */
	zval_dtor(&label->u.constant);
}
/* }}} */

void zend_resolve_goto_label(zend_op_array *op_array, zend_op *opline, int pass2 TSRMLS_DC) /* {{{ */
{
	zend_label *dest;
	long current, distance;

	if (CG(labels) == NULL ||
	    zend_hash_find(CG(labels), Z_STRVAL(opline->op2.u.constant), Z_STRLEN(opline->op2.u.constant)+1, (void**)&dest) == FAILURE) {

	    if (pass2) {
	    	CG(in_compilation) = 1;
	    	CG(active_op_array) = op_array;
	    	CG(zend_lineno) = opline->lineno;
			zend_error(E_COMPILE_ERROR, "'goto' to undefined label '%s'", Z_STRVAL(opline->op2.u.constant));
	    } else {
			/* Label is not defined. Delay to pass 2. */
			INC_BPC(op_array);
			return;
		}
	}

	opline->op1.u.opline_num = dest->opline_num;
	zval_dtor(&opline->op2.u.constant);

	/* Check that we are not moving into loop or switch */
	current = opline->extended_value;
	for (distance = 0; current != dest->brk_cont; distance++) {
		if (current == -1) {
		    if (pass2) {
		    	CG(in_compilation) = 1;
	    		CG(active_op_array) = op_array;
	    		CG(zend_lineno) = opline->lineno;
	    	}
			zend_error(E_COMPILE_ERROR, "'goto' into loop or switch statement is disallowed");
		}
		current = op_array->brk_cont_array[current].parent;
	}

	if (distance == 0) {
		/* Nothing to break out of, optimize to ZEND_JMP */
		opline->opcode = ZEND_JMP;
		opline->extended_value = 0;
		SET_UNUSED(opline->op2);
	} else {
		/* Set real break distance */
		ZVAL_LONG(&opline->op2.u.constant, distance);
	}

    if (pass2) {
		DEC_BPC(op_array);
    }
}
/* }}} */

void zend_do_goto(const znode *label TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_GOTO;
	opline->extended_value = CG(active_op_array)->current_brk_cont;
	SET_UNUSED(opline->op1);
	opline->op2 = *label;
	zend_resolve_goto_label(CG(active_op_array), opline, 0 TSRMLS_CC);
}
/* }}} */

void zend_release_labels(TSRMLS_D) /* {{{ */
{
	if (CG(labels)) {
		zend_hash_destroy(CG(labels));
		FREE_HASHTABLE(CG(labels));
	}
	if (!zend_stack_is_empty(&CG(labels_stack))) {
		HashTable **pht;

		zend_stack_top(&CG(labels_stack), (void**)&pht);
		CG(labels) = *pht;
		zend_stack_del_top(&CG(labels_stack));
	} else {
		CG(labels) = NULL;
	}
}
/* }}} */

void zend_do_build_full_name(znode *result, znode *prefix, znode *name, int is_class_member TSRMLS_DC) /* {{{ */
{
	zend_uint length;

	if (!result) {
		result = prefix;
	} else {
		*result = *prefix;
	}

	if (is_class_member) {
		length = sizeof("::")-1 + result->u.constant.value.str.len + name->u.constant.value.str.len;
		result->u.constant.value.str.val = erealloc(result->u.constant.value.str.val, length+1);
		memcpy(&result->u.constant.value.str.val[result->u.constant.value.str.len], "::", sizeof("::")-1);
		memcpy(&result->u.constant.value.str.val[result->u.constant.value.str.len + sizeof("::")-1], name->u.constant.value.str.val, name->u.constant.value.str.len+1);
		STR_FREE(name->u.constant.value.str.val);
		result->u.constant.value.str.len = length;
	} else {
		length = sizeof("\\")-1 + result->u.constant.value.str.len + name->u.constant.value.str.len;
		result->u.constant.value.str.val = erealloc(result->u.constant.value.str.val, length+1);
		memcpy(&result->u.constant.value.str.val[result->u.constant.value.str.len], "\\", sizeof("\\")-1);
		memcpy(&result->u.constant.value.str.val[result->u.constant.value.str.len + sizeof("\\")-1], name->u.constant.value.str.val, name->u.constant.value.str.len+1);
		STR_FREE(name->u.constant.value.str.val);
		result->u.constant.value.str.len = length;
	}
}
/* }}} */

int zend_do_begin_class_member_function_call(znode *class_name, znode *method_name TSRMLS_DC) /* {{{ */
{
	znode class_node;
	unsigned char *ptr = NULL;
	zend_op *opline;
	ulong fetch_type = 0;

	if (method_name->op_type == IS_CONST) {
		char *lcname = zend_str_tolower_dup(Z_STRVAL(method_name->u.constant), Z_STRLEN(method_name->u.constant));
		if ((sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) == Z_STRLEN(method_name->u.constant) &&
		    memcmp(lcname, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) == 0) {
			zval_dtor(&method_name->u.constant);
			SET_UNUSED(*method_name);
		}
		efree(lcname);
	}

	if (class_name->op_type == IS_CONST &&
	    ZEND_FETCH_CLASS_DEFAULT == zend_get_class_fetch_type(Z_STRVAL(class_name->u.constant), Z_STRLEN(class_name->u.constant))) {
		fetch_type = ZEND_FETCH_CLASS_GLOBAL;
		zend_resolve_class_name(class_name, &fetch_type, 1 TSRMLS_CC);
		class_node = *class_name;
	} else {
		zend_do_fetch_class(&class_node, class_name TSRMLS_CC);
	}
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INIT_STATIC_METHOD_CALL;
	opline->op1 = class_node;
	opline->op2 = *method_name;

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
	return 1; /* Dynamic */
}
/* }}} */

void zend_do_end_function_call(znode *function_name, znode *result, const znode *argument_list, int is_method, int is_dynamic_fcall TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if (is_method && function_name && function_name->op_type == IS_UNUSED) {
		/* clone */
		if (Z_LVAL(argument_list->u.constant) != 0) {
			zend_error(E_WARNING, "Clone method does not require arguments");
		}
		opline = &CG(active_op_array)->opcodes[Z_LVAL(function_name->u.constant)];
	} else {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		if (!is_method && !is_dynamic_fcall && function_name->op_type==IS_CONST) {
			opline->opcode = ZEND_DO_FCALL;
			opline->op1 = *function_name;
			ZVAL_LONG(&opline->op2.u.constant, zend_hash_func(Z_STRVAL(function_name->u.constant), Z_STRLEN(function_name->u.constant) + 1));
		} else {
			opline->opcode = ZEND_DO_FCALL_BY_NAME;
			SET_UNUSED(opline->op1);
		}
	}

	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_VAR;
	*result = opline->result;
	SET_UNUSED(opline->op2);

	zend_stack_del_top(&CG(function_call_stack));
	opline->extended_value = Z_LVAL(argument_list->u.constant);
}
/* }}} */

void zend_do_pass_param(znode *param, zend_uchar op, int offset TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	int original_op=op;
	zend_function **function_ptr_ptr, *function_ptr;
	int send_by_reference;
	int send_function = 0;

	zend_stack_top(&CG(function_call_stack), (void **) &function_ptr_ptr);
	function_ptr = *function_ptr_ptr;

	if (original_op == ZEND_SEND_REF && !CG(allow_call_time_pass_reference)) {
		if (function_ptr &&
		    function_ptr->common.function_name &&
		    function_ptr->common.type == ZEND_USER_FUNCTION &&
		    !ARG_SHOULD_BE_SENT_BY_REF(function_ptr, (zend_uint) offset)) {
			zend_error(E_DEPRECATED,
						"Call-time pass-by-reference has been deprecated; "
						"If you would like to pass it by reference, modify the declaration of %s().  "
						"If you would like to enable call-time pass-by-reference, you can set "
						"allow_call_time_pass_reference to true in your INI file", function_ptr->common.function_name);
		} else {
			zend_error(E_DEPRECATED, "Call-time pass-by-reference has been deprecated");
		}
	}

	if (function_ptr) {
		if (ARG_MAY_BE_SENT_BY_REF(function_ptr, (zend_uint) offset)) {
			if (param->op_type & (IS_VAR|IS_CV)) {
				send_by_reference = 1;
				if (op == ZEND_SEND_VAR && zend_is_function_or_method_call(param)) {
					/* Method call */
					op = ZEND_SEND_VAR_NO_REF;
					send_function = ZEND_ARG_SEND_FUNCTION | ZEND_ARG_SEND_SILENT;
				}
			} else {
				op = ZEND_SEND_VAL;
				send_by_reference = 0;
			}
		} else {
			send_by_reference = ARG_SHOULD_BE_SENT_BY_REF(function_ptr, (zend_uint) offset) ? ZEND_ARG_SEND_BY_REF : 0;
		}
	} else {
		send_by_reference = 0;
	}

	if (op == ZEND_SEND_VAR && zend_is_function_or_method_call(param)) {
		/* Method call */
		op = ZEND_SEND_VAR_NO_REF;
		send_function = ZEND_ARG_SEND_FUNCTION;
	} else if (op == ZEND_SEND_VAL && (param->op_type & (IS_VAR|IS_CV))) {
		op = ZEND_SEND_VAR_NO_REF;
	}

	if (op!=ZEND_SEND_VAR_NO_REF && send_by_reference==ZEND_ARG_SEND_BY_REF) {
		/* change to passing by reference */
		switch (param->op_type) {
			case IS_VAR:
			case IS_CV:
				op = ZEND_SEND_REF;
				break;
			default:
				zend_error(E_COMPILE_ERROR, "Only variables can be passed by reference");
				break;
		}
	}

	if (original_op == ZEND_SEND_VAR) {
		switch (op) {
			case ZEND_SEND_VAR_NO_REF:
				zend_do_end_variable_parse(param, BP_VAR_R, 0 TSRMLS_CC);
				break;
			case ZEND_SEND_VAR:
				if (function_ptr) {
					zend_do_end_variable_parse(param, BP_VAR_R, 0 TSRMLS_CC);
				} else {
					zend_do_end_variable_parse(param, BP_VAR_FUNC_ARG, offset TSRMLS_CC);
				}
				break;
			case ZEND_SEND_REF:
				zend_do_end_variable_parse(param, BP_VAR_W, 0 TSRMLS_CC);
				break;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (op == ZEND_SEND_VAR_NO_REF) {
		if (function_ptr) {
			opline->extended_value = ZEND_ARG_COMPILE_TIME_BOUND | send_by_reference | send_function;
		} else {
			opline->extended_value = send_function;
		}
	} else {
		if (function_ptr) {
			opline->extended_value = ZEND_DO_FCALL;
		} else {
			opline->extended_value = ZEND_DO_FCALL_BY_NAME;
		}
	}
	opline->opcode = op;
	opline->op1 = *param;
	opline->op2.u.opline_num = offset;
	SET_UNUSED(opline->op2);
}
/* }}} */

static int generate_free_switch_expr(const zend_switch_entry *switch_entry TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if (switch_entry->cond.op_type != IS_VAR && switch_entry->cond.op_type != IS_TMP_VAR) {
		return (switch_entry->cond.op_type == IS_UNUSED);
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = (switch_entry->cond.op_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
	opline->op1 = switch_entry->cond;
	SET_UNUSED(opline->op2);
	opline->extended_value = 0;
	return 0;
}
/* }}} */

static int generate_free_foreach_copy(const zend_op *foreach_copy TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	/* If we reach the seperator then stop applying the stack */
	if (foreach_copy->result.op_type == IS_UNUSED && foreach_copy->op1.op_type == IS_UNUSED) {
		return 1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = (foreach_copy->result.op_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
	opline->op1 = foreach_copy->result;
	SET_UNUSED(opline->op2);
	opline->extended_value = 1;

	if (foreach_copy->op1.op_type != IS_UNUSED) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = (foreach_copy->op1.op_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
		opline->op1 = foreach_copy->op1;
		SET_UNUSED(opline->op2);
		opline->extended_value = 0;
	}

	return 0;
}
/* }}} */

void zend_do_return(znode *expr, int do_end_vparse TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	int start_op_number, end_op_number;

	if (do_end_vparse) {
		if (CG(active_op_array)->return_reference && !zend_is_function_or_method_call(expr)) {
			zend_do_end_variable_parse(expr, BP_VAR_W, 0 TSRMLS_CC);
		} else {
			zend_do_end_variable_parse(expr, BP_VAR_R, 0 TSRMLS_CC);
		}
	}

	start_op_number = get_next_op_number(CG(active_op_array));

#ifdef ZTS
	zend_stack_apply_with_argument(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_switch_expr TSRMLS_CC);
	zend_stack_apply_with_argument(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_foreach_copy TSRMLS_CC);
#else
	zend_stack_apply(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_switch_expr);
	zend_stack_apply(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_foreach_copy);
#endif

	end_op_number = get_next_op_number(CG(active_op_array));
	while (start_op_number < end_op_number) {
		CG(active_op_array)->opcodes[start_op_number].op1.u.EA.type = EXT_TYPE_FREE_ON_RETURN;
		start_op_number++;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_RETURN;

	if (expr) {
		opline->op1 = *expr;

		if (do_end_vparse && zend_is_function_or_method_call(expr)) {
			opline->extended_value = ZEND_RETURNS_FUNCTION;
		}
	} else {
		opline->op1.op_type = IS_CONST;
		INIT_ZVAL(opline->op1.u.constant);
	}

	SET_UNUSED(opline->op2);
}
/* }}} */

static int zend_add_try_element(zend_uint try_op TSRMLS_DC) /* {{{ */
{
	int try_catch_offset = CG(active_op_array)->last_try_catch++;

	CG(active_op_array)->try_catch_array = erealloc(CG(active_op_array)->try_catch_array, sizeof(zend_try_catch_element)*CG(active_op_array)->last_try_catch);
	CG(active_op_array)->try_catch_array[try_catch_offset].try_op = try_op;
	return try_catch_offset;
}
/* }}} */

static void zend_add_catch_element(int offset, zend_uint catch_op TSRMLS_DC) /* {{{ */
{
	CG(active_op_array)->try_catch_array[offset].catch_op = catch_op;
}
/* }}} */

void zend_do_first_catch(znode *open_parentheses TSRMLS_DC) /* {{{ */
{
	open_parentheses->u.opline_num = get_next_op_number(CG(active_op_array));
}
/* }}} */

void zend_initialize_try_catch_element(const znode *try_token TSRMLS_DC) /* {{{ */
{
	int jmp_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_llist jmp_list;
	zend_llist *jmp_list_ptr;

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	/* save for backpatching */

	zend_llist_init(&jmp_list, sizeof(int), NULL, 0);
	zend_stack_push(&CG(bp_stack), (void *) &jmp_list, sizeof(zend_llist));
	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	zend_llist_add_element(jmp_list_ptr, &jmp_op_number);

	zend_add_catch_element(try_token->u.opline_num, get_next_op_number(CG(active_op_array)) TSRMLS_CC);
}
/* }}} */

void zend_do_mark_last_catch(const znode *first_catch, const znode *last_additional_catch TSRMLS_DC) /* {{{ */
{
	CG(active_op_array)->last--;
	zend_do_if_end(TSRMLS_C);
	if (last_additional_catch->u.opline_num == -1) {
		CG(active_op_array)->opcodes[first_catch->u.opline_num].op1.u.EA.type = 1;
		CG(active_op_array)->opcodes[first_catch->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
	} else {
		CG(active_op_array)->opcodes[last_additional_catch->u.opline_num].op1.u.EA.type = 1;
		CG(active_op_array)->opcodes[last_additional_catch->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
	}
	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_try(znode *try_token TSRMLS_DC) /* {{{ */
{
	try_token->u.opline_num = zend_add_try_element(get_next_op_number(CG(active_op_array)) TSRMLS_CC);
	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_begin_catch(znode *try_token, znode *class_name, const znode *catch_var, znode *first_catch TSRMLS_DC) /* {{{ */
{
	long catch_op_number;
	zend_op *opline;
	znode catch_class;

	zend_do_fetch_class(&catch_class, class_name TSRMLS_CC);

	catch_op_number = get_next_op_number(CG(active_op_array));
	if (catch_op_number > 0) {
		opline = &CG(active_op_array)->opcodes[catch_op_number-1];
		if (opline->opcode == ZEND_FETCH_CLASS) {
			opline->extended_value |= ZEND_FETCH_CLASS_NO_AUTOLOAD;
		}
	}

	if (first_catch) {
		first_catch->u.opline_num = catch_op_number;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_CATCH;
	opline->op1 = catch_class;
/*	SET_UNUSED(opline->op1); */ /* FIXME: Define IS_CLASS or something like that */
	opline->op2.op_type = IS_CV;
	opline->op2.u.var = lookup_cv(CG(active_op_array), catch_var->u.constant.value.str.val, catch_var->u.constant.value.str.len);
	opline->op2.u.EA.type = 0;
	opline->op1.u.EA.type = 0; /* 1 means it's the last catch in the block */

	try_token->u.opline_num = catch_op_number;
}
/* }}} */

void zend_do_end_catch(const znode *try_token TSRMLS_DC) /* {{{ */
{
	int jmp_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_llist *jmp_list_ptr;

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	/* save for backpatching */

	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	zend_llist_add_element(jmp_list_ptr, &jmp_op_number);

	CG(active_op_array)->opcodes[try_token->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
}
/* }}} */

void zend_do_throw(const znode *expr TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_THROW;
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
}
/* }}} */

ZEND_API void function_add_ref(zend_function *function) /* {{{ */
{
	if (function->type == ZEND_USER_FUNCTION) {
		zend_op_array *op_array = &function->op_array;

		(*op_array->refcount)++;
		if (op_array->static_variables) {
			HashTable *static_variables = op_array->static_variables;
			zval *tmp_zval;

			ALLOC_HASHTABLE(op_array->static_variables);
			zend_hash_init(op_array->static_variables, zend_hash_num_elements(static_variables), NULL, ZVAL_PTR_DTOR, 0);
			zend_hash_copy(op_array->static_variables, static_variables, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));
		}
	}
}
/* }}} */

static void do_inherit_parent_constructor(zend_class_entry *ce) /* {{{ */
{
	zend_function *function;

	if (!ce->parent) {
		return;
	}

	/* You cannot change create_object */
	ce->create_object = ce->parent->create_object;

	/* Inherit special functions if needed */
	if (!ce->get_iterator) {
		ce->get_iterator = ce->parent->get_iterator;
	}
	if (!ce->iterator_funcs.funcs) {
		ce->iterator_funcs.funcs = ce->parent->iterator_funcs.funcs;
	}
	if (!ce->__get) {
		ce->__get   = ce->parent->__get;
	}
	if (!ce->__set) {
		ce->__set = ce->parent->__set;
	}
	if (!ce->__unset) {
		ce->__unset = ce->parent->__unset;
	}
	if (!ce->__isset) {
		ce->__isset = ce->parent->__isset;
	}
	if (!ce->__call) {
		ce->__call = ce->parent->__call;
	}
	if (!ce->__callstatic) {
		ce->__callstatic = ce->parent->__callstatic;
	}
	if (!ce->__tostring) {
		ce->__tostring = ce->parent->__tostring;
	}
	if (!ce->clone) {
		ce->clone = ce->parent->clone;
	}
	if(!ce->serialize) {
		ce->serialize = ce->parent->serialize;
	}
	if(!ce->unserialize) {
		ce->unserialize = ce->parent->unserialize;
	}
	if (!ce->destructor) {
		ce->destructor   = ce->parent->destructor;
	}
	if (ce->constructor) {
		if (ce->parent->constructor && ce->parent->constructor->common.fn_flags & ZEND_ACC_FINAL) {
			zend_error(E_ERROR, "Cannot override final %s::%s() with %s::%s()",
				ce->parent->name, ce->parent->constructor->common.function_name,
				ce->name, ce->constructor->common.function_name
				);
		}
		return;
	}

	if (zend_hash_find(&ce->parent->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME), (void **)&function)==SUCCESS) {
		/* inherit parent's constructor */
		zend_hash_update(&ce->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME), function, sizeof(zend_function), NULL);
		function_add_ref(function);
	} else {
		/* Don't inherit the old style constructor if we already have the new style constructor */
		char *lc_class_name;
		char *lc_parent_class_name;

		lc_class_name = zend_str_tolower_dup(ce->name, ce->name_length);
		if (!zend_hash_exists(&ce->function_table, lc_class_name, ce->name_length+1)) {
			lc_parent_class_name = zend_str_tolower_dup(ce->parent->name, ce->parent->name_length);
			if (!zend_hash_exists(&ce->function_table, lc_parent_class_name, ce->parent->name_length+1) && 
					zend_hash_find(&ce->parent->function_table, lc_parent_class_name, ce->parent->name_length+1, (void **)&function)==SUCCESS) {
				if (function->common.fn_flags & ZEND_ACC_CTOR) {
					/* inherit parent's constructor */
					zend_hash_update(&ce->function_table, lc_parent_class_name, ce->parent->name_length+1, function, sizeof(zend_function), NULL);
					function_add_ref(function);
				}
			}
			efree(lc_parent_class_name);
		}
		efree(lc_class_name);
	}
	ce->constructor = ce->parent->constructor;
}
/* }}} */

char *zend_visibility_string(zend_uint fn_flags) /* {{{ */
{
	if (fn_flags & ZEND_ACC_PRIVATE) {
		return "private";
	}
	if (fn_flags & ZEND_ACC_PROTECTED) {
		return "protected";
	}
	if (fn_flags & ZEND_ACC_PUBLIC) {
		return "public";
	}
	return "";
}
/* }}} */

static void do_inherit_method(zend_function *function) /* {{{ */
{
	/* The class entry of the derived function intentionally remains the same
	 * as that of the parent class.  That allows us to know in which context
	 * we're running, and handle private method calls properly.
	 */
	function_add_ref(function);
}
/* }}} */

static zend_bool zend_do_perform_implementation_check(const zend_function *fe, const zend_function *proto) /* {{{ */
{
	zend_uint i;

	/* If it's a user function then arg_info == NULL means we don't have any parameters but
	 * we still need to do the arg number checks.  We are only willing to ignore this for internal
	 * functions because extensions don't always define arg_info.
	 */
	if (!proto || (!proto->common.arg_info && proto->common.type != ZEND_USER_FUNCTION)) {
		return 1;
	}

	/* Checks for constructors only if they are declared in an interface */
	if ((fe->common.fn_flags & ZEND_ACC_CTOR) && (proto->common.scope->ce_flags & ZEND_ACC_INTERFACE) == 0) {
		return 1;
	}

	/* check number of arguments */
	if (proto->common.required_num_args < fe->common.required_num_args
		|| proto->common.num_args > fe->common.num_args) {
		return 0;
	}

	if (proto->common.pass_rest_by_reference
		&& !fe->common.pass_rest_by_reference) {
		return 0;
	}

	if (fe->common.return_reference != proto->common.return_reference) {
		return 0;
	}

	for (i=0; i < proto->common.num_args; i++) {
		if (ZEND_LOG_XOR(fe->common.arg_info[i].class_name, proto->common.arg_info[i].class_name)) {
			/* Only one has a type hint and the other one doesn't */
			return 0;
		}
		if (fe->common.arg_info[i].class_name
			&& strcasecmp(fe->common.arg_info[i].class_name, proto->common.arg_info[i].class_name)!=0) {
			char *colon;

			if (fe->common.type != ZEND_USER_FUNCTION ||
			    strchr(proto->common.arg_info[i].class_name, '\\') != NULL ||
			    (colon = zend_memrchr(fe->common.arg_info[i].class_name, '\\', fe->common.arg_info[i].class_name_len)) == NULL ||
			    strcasecmp(colon+1, proto->common.arg_info[i].class_name) != 0) {
				return 0;
			}
		}
		if (fe->common.arg_info[i].array_type_hint != proto->common.arg_info[i].array_type_hint) {
			/* Only one has an array type hint and the other one doesn't */
			return 0;
		}
		if (fe->common.arg_info[i].pass_by_reference != proto->common.arg_info[i].pass_by_reference) {
			return 0;
		}
	}

	if (proto->common.pass_rest_by_reference) {
		for (i=proto->common.num_args; i < fe->common.num_args; i++) {
			if (!fe->common.arg_info[i].pass_by_reference) {
				return 0;
			}
		}
	}
	return 1;
}
/* }}} */

static zend_bool do_inherit_method_check(HashTable *child_function_table, zend_function *parent, const zend_hash_key *hash_key, zend_class_entry *child_ce) /* {{{ */
{
	zend_uint child_flags;
	zend_uint parent_flags = parent->common.fn_flags;
	zend_function *child;
	TSRMLS_FETCH();

	if (zend_hash_quick_find(child_function_table, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child)==FAILURE) {
		if (parent_flags & (ZEND_ACC_ABSTRACT)) {
			child_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		return 1; /* method doesn't exist in child, copy from parent */
	}

	if (parent->common.fn_flags & ZEND_ACC_ABSTRACT
		&& parent->common.scope != (child->common.prototype ? child->common.prototype->common.scope : child->common.scope)
		&& child->common.fn_flags & (ZEND_ACC_ABSTRACT|ZEND_ACC_IMPLEMENTED_ABSTRACT)) {
		zend_error(E_COMPILE_ERROR, "Can't inherit abstract function %s::%s() (previously declared abstract in %s)", 
			parent->common.scope->name,
			child->common.function_name,
			child->common.prototype ? child->common.prototype->common.scope->name : child->common.scope->name);
	}

	if (parent_flags & ZEND_ACC_FINAL) {
		zend_error(E_COMPILE_ERROR, "Cannot override final method %s::%s()", ZEND_FN_SCOPE_NAME(parent), child->common.function_name);
	}

	child_flags	= child->common.fn_flags;
	/* You cannot change from static to non static and vice versa.
	 */
	if ((child_flags & ZEND_ACC_STATIC) != (parent_flags & ZEND_ACC_STATIC)) {
		if (child->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Cannot make non static method %s::%s() static in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
		} else {
			zend_error(E_COMPILE_ERROR, "Cannot make static method %s::%s() non static in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
		}
	}

	/* Disallow making an inherited method abstract. */
	if ((child_flags & ZEND_ACC_ABSTRACT) && !(parent_flags & ZEND_ACC_ABSTRACT)) {
		zend_error(E_COMPILE_ERROR, "Cannot make non abstract method %s::%s() abstract in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
	}

	if (parent_flags & ZEND_ACC_CHANGED) {
		child->common.fn_flags |= ZEND_ACC_CHANGED;
	} else {
		/* Prevent derived classes from restricting access that was available in parent classes
		 */
		if ((child_flags & ZEND_ACC_PPP_MASK) > (parent_flags & ZEND_ACC_PPP_MASK)) {
			zend_error(E_COMPILE_ERROR, "Access level to %s::%s() must be %s (as in class %s)%s", ZEND_FN_SCOPE_NAME(child), child->common.function_name, zend_visibility_string(parent_flags), ZEND_FN_SCOPE_NAME(parent), (parent_flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if (((child_flags & ZEND_ACC_PPP_MASK) < (parent_flags & ZEND_ACC_PPP_MASK))
			&& ((parent_flags & ZEND_ACC_PPP_MASK) & ZEND_ACC_PRIVATE)) {
			child->common.fn_flags |= ZEND_ACC_CHANGED;
		}
	}

	if (parent_flags & ZEND_ACC_PRIVATE) {
		child->common.prototype = NULL;		
	} else if (parent_flags & ZEND_ACC_ABSTRACT) {
		child->common.fn_flags |= ZEND_ACC_IMPLEMENTED_ABSTRACT;
		child->common.prototype = parent;
	} else if (!(parent->common.fn_flags & ZEND_ACC_CTOR) || (parent->common.prototype && (parent->common.prototype->common.scope->ce_flags & ZEND_ACC_INTERFACE))) {
		/* ctors only have a prototype if it comes from an interface */
		child->common.prototype = parent->common.prototype ? parent->common.prototype : parent;
	}

	if (child->common.prototype && (child->common.prototype->common.fn_flags & ZEND_ACC_ABSTRACT)) {
		if (!zend_do_perform_implementation_check(child, child->common.prototype)) {
			zend_error(E_COMPILE_ERROR, "Declaration of %s::%s() must be compatible with that of %s::%s()", ZEND_FN_SCOPE_NAME(child), child->common.function_name, ZEND_FN_SCOPE_NAME(child->common.prototype), child->common.prototype->common.function_name);
		}
	} else if (EG(error_reporting) & E_STRICT || EG(user_error_handler)) { /* Check E_STRICT (or custom error handler) before the check so that we save some time */
		if (!zend_do_perform_implementation_check(child, parent)) {
			zend_error(E_STRICT, "Declaration of %s::%s() should be compatible with that of %s::%s()", ZEND_FN_SCOPE_NAME(child), child->common.function_name, ZEND_FN_SCOPE_NAME(parent), parent->common.function_name);
		}
	}

	return 0;
}
/* }}} */

static zend_bool do_inherit_property_access_check(HashTable *target_ht, zend_property_info *parent_info, const zend_hash_key *hash_key, zend_class_entry *ce) /* {{{ */
{
	zend_property_info *child_info;
	zend_class_entry *parent_ce = ce->parent;

	if (parent_info->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_SHADOW)) {
		if (zend_hash_quick_find(&ce->properties_info, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child_info)==SUCCESS) {
			child_info->flags |= ZEND_ACC_CHANGED;
		} else {
			zend_hash_quick_update(&ce->properties_info, hash_key->arKey, hash_key->nKeyLength, hash_key->h, parent_info, sizeof(zend_property_info), (void **) &child_info);
			if(ce->type & ZEND_INTERNAL_CLASS) {
				zend_duplicate_property_info_internal(child_info);
			} else {
				zend_duplicate_property_info(child_info);
			}
			child_info->flags &= ~ZEND_ACC_PRIVATE; /* it's not private anymore */
			child_info->flags |= ZEND_ACC_SHADOW; /* but it's a shadow of private */
		}
		return 0; /* don't copy access information to child */
	}

	if (zend_hash_quick_find(&ce->properties_info, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child_info)==SUCCESS) {
		if ((parent_info->flags & ZEND_ACC_STATIC) != (child_info->flags & ZEND_ACC_STATIC)) {
			zend_error(E_COMPILE_ERROR, "Cannot redeclare %s%s::$%s as %s%s::$%s",
				(parent_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", parent_ce->name, hash_key->arKey,
				(child_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", ce->name, hash_key->arKey);
				
		}

		if(parent_info->flags & ZEND_ACC_CHANGED) {
			child_info->flags |= ZEND_ACC_CHANGED;
		}

		if ((child_info->flags & ZEND_ACC_PPP_MASK) > (parent_info->flags & ZEND_ACC_PPP_MASK)) {
			zend_error(E_COMPILE_ERROR, "Access level to %s::$%s must be %s (as in class %s)%s", ce->name, hash_key->arKey, zend_visibility_string(parent_info->flags), parent_ce->name, (parent_info->flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if (child_info->flags & ZEND_ACC_IMPLICIT_PUBLIC) {
			if (!(parent_info->flags & ZEND_ACC_IMPLICIT_PUBLIC)) {
				/* Explicitly copy the default value from the parent (if it has one) */
				zval **pvalue;
	
				if (zend_hash_quick_find(&parent_ce->default_properties, parent_info->name, parent_info->name_length+1, parent_info->h, (void **) &pvalue) == SUCCESS) {
					Z_ADDREF_PP(pvalue);
					zend_hash_quick_del(&ce->default_properties, child_info->name, child_info->name_length+1, parent_info->h);
					zend_hash_quick_update(&ce->default_properties, parent_info->name, parent_info->name_length+1, parent_info->h, pvalue, sizeof(zval *), NULL);
				}
			}
			return 1; /* Inherit from the parent */
		} else if ((child_info->flags & ZEND_ACC_PUBLIC) && (parent_info->flags & ZEND_ACC_PROTECTED)) {
			char *prot_name;
			int prot_name_length;

			zend_mangle_property_name(&prot_name, &prot_name_length, "*", 1, child_info->name, child_info->name_length, ce->type & ZEND_INTERNAL_CLASS);
			if (child_info->flags & ZEND_ACC_STATIC) {
				zval **prop;
				HashTable *ht;

				if (parent_ce->type != ce->type) {
					/* User class extends internal class */
					TSRMLS_FETCH();

					ht = CE_STATIC_MEMBERS(parent_ce);
				} else {
					ht = &parent_ce->default_static_members;
				}
				if (zend_hash_find(ht, prot_name, prot_name_length+1, (void**)&prop) == SUCCESS) {
					zend_hash_del(&ce->default_static_members, prot_name, prot_name_length+1);
				}
			} else {
				zend_hash_del(&ce->default_properties, prot_name, prot_name_length+1);
			}
			pefree(prot_name, ce->type & ZEND_INTERNAL_CLASS);
		}
		return 0;	/* Don't copy from parent */
	} else {
		return 1;	/* Copy from parent */
	}
}
/* }}} */

static inline void do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	if (!(ce->ce_flags & ZEND_ACC_INTERFACE) && iface->interface_gets_implemented && iface->interface_gets_implemented(iface, ce TSRMLS_CC) == FAILURE) {
		zend_error(E_CORE_ERROR, "Class %s could not implement interface %s", ce->name, iface->name);
	}
	if (ce == iface) {
		zend_error(E_ERROR, "Interface %s cannot implement itself", ce->name);
	}
}
/* }}} */

ZEND_API void zend_do_inherit_interfaces(zend_class_entry *ce, const zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	/* expects interface to be contained in ce's interface list already */
	zend_uint i, ce_num, if_num = iface->num_interfaces;
	zend_class_entry *entry;

	if (if_num==0) {
		return;
	}
	ce_num = ce->num_interfaces;

	if (ce->type == ZEND_INTERNAL_CLASS) {
		ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	} else {
		ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	}

	/* Inherit the interfaces, only if they're not already inherited by the class */
	while (if_num--) {
		entry = iface->interfaces[if_num];
		for (i = 0; i < ce_num; i++) {
			if (ce->interfaces[i] == entry) {
				break;
			}
		}
		if (i == ce_num) {
			ce->interfaces[ce->num_interfaces++] = entry;
		}
	}

	/* and now call the implementing handlers */
	while (ce_num < ce->num_interfaces) {
		do_implement_interface(ce, ce->interfaces[ce_num++] TSRMLS_CC);
	}
}
/* }}} */

static int inherit_static_prop(zval **p TSRMLS_DC, int num_args, va_list args, const zend_hash_key *key) /* {{{ */
{
	HashTable *target = va_arg(args, HashTable*);

	if (!zend_hash_quick_exists(target, key->arKey, key->nKeyLength, key->h)) {
		SEPARATE_ZVAL_TO_MAKE_IS_REF(p);
		if (zend_hash_quick_add(target, key->arKey, key->nKeyLength, key->h, p, sizeof(zval*), NULL) == SUCCESS) {
			Z_ADDREF_PP(p);
		}
	}
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

ZEND_API void zend_do_inheritance(zend_class_entry *ce, zend_class_entry *parent_ce TSRMLS_DC) /* {{{ */
{
	if ((ce->ce_flags & ZEND_ACC_INTERFACE)
		&& !(parent_ce->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error(E_COMPILE_ERROR, "Interface %s may not inherit from class (%s)", ce->name, parent_ce->name);
	}
	if (parent_ce->ce_flags & ZEND_ACC_FINAL_CLASS) {
		zend_error(E_COMPILE_ERROR, "Class %s may not inherit from final class (%s)", ce->name, parent_ce->name);
	}

	ce->parent = parent_ce;
	/* Copy serialize/unserialize callbacks */
	if (!ce->serialize) {
		ce->serialize   = parent_ce->serialize;
	}
	if (!ce->unserialize) {
		ce->unserialize = parent_ce->unserialize;
	}

	/* Inherit interfaces */
	zend_do_inherit_interfaces(ce, parent_ce TSRMLS_CC);

	/* Inherit properties */
	zend_hash_merge(&ce->default_properties, &parent_ce->default_properties, (void (*)(void *)) zval_add_ref, NULL, sizeof(zval *), 0);
	if (parent_ce->type != ce->type) {
		/* User class extends internal class */
		zend_update_class_constants(parent_ce  TSRMLS_CC);
		zend_hash_apply_with_arguments(CE_STATIC_MEMBERS(parent_ce) TSRMLS_CC, (apply_func_args_t)inherit_static_prop, 1, &ce->default_static_members);
	} else {
		zend_hash_apply_with_arguments(&parent_ce->default_static_members TSRMLS_CC, (apply_func_args_t)inherit_static_prop, 1, &ce->default_static_members);
	}
	zend_hash_merge_ex(&ce->properties_info, &parent_ce->properties_info, (copy_ctor_func_t) (ce->type & ZEND_INTERNAL_CLASS ? zend_duplicate_property_info_internal : zend_duplicate_property_info), sizeof(zend_property_info), (merge_checker_func_t) do_inherit_property_access_check, ce);

	zend_hash_merge(&ce->constants_table, &parent_ce->constants_table, (void (*)(void *)) zval_add_ref, NULL, sizeof(zval *), 0);
	zend_hash_merge_ex(&ce->function_table, &parent_ce->function_table, (copy_ctor_func_t) do_inherit_method, sizeof(zend_function), (merge_checker_func_t) do_inherit_method_check, ce);
	do_inherit_parent_constructor(ce);

	if (ce->ce_flags & ZEND_ACC_IMPLICIT_ABSTRACT_CLASS && ce->type == ZEND_INTERNAL_CLASS) {
		ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	} else if (!(ce->ce_flags & ZEND_ACC_IMPLEMENT_INTERFACES)) {
		/* The verification will be done in runtime by ZEND_VERIFY_ABSTRACT_CLASS */
		zend_verify_abstract_class(ce TSRMLS_CC);
	}
}
/* }}} */

static zend_bool do_inherit_constant_check(HashTable *child_constants_table, const zval **parent_constant, const zend_hash_key *hash_key, const zend_class_entry *iface) /* {{{ */
{
	zval **old_constant;

	if (zend_hash_quick_find(child_constants_table, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void**)&old_constant) == SUCCESS) {
		if (*old_constant != *parent_constant) {
			zend_error(E_COMPILE_ERROR, "Cannot inherit previously-inherited or override constant %s from interface %s", hash_key->arKey, iface->name);
		}
		return 0;
	}
	return 1;
}
/* }}} */

static int do_interface_constant_check(zval **val TSRMLS_DC, int num_args, va_list args, const zend_hash_key *key) /* {{{ */
{
	zend_class_entry **iface = va_arg(args, zend_class_entry**);

	do_inherit_constant_check(&(*iface)->constants_table, (const zval **) val, key, *iface);

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

ZEND_API void zend_do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	zend_uint i, ignore = 0;
	zend_uint current_iface_num = ce->num_interfaces;
	zend_uint parent_iface_num  = ce->parent ? ce->parent->num_interfaces : 0;

	for (i = 0; i < ce->num_interfaces; i++) {
		if (ce->interfaces[i] == NULL) {
			memmove(ce->interfaces + i, ce->interfaces + i + 1, sizeof(zend_class_entry*) * (--ce->num_interfaces - i));
			i--;
		} else if (ce->interfaces[i] == iface) {
			if (i < parent_iface_num) {
				ignore = 1;
			} else {
				zend_error(E_COMPILE_ERROR, "Class %s cannot implement previously implemented interface %s", ce->name, iface->name);
			}
		}
	}
	if (ignore) {
		/* Check for attempt to redeclare interface constants */
		zend_hash_apply_with_arguments(&ce->constants_table TSRMLS_CC, (apply_func_args_t) do_interface_constant_check, 1, &iface);
	} else {
		if (ce->num_interfaces >= current_iface_num) {
			if (ce->type == ZEND_INTERNAL_CLASS) {
				ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			} else {
				ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			}
		}
		ce->interfaces[ce->num_interfaces++] = iface;
	
		zend_hash_merge_ex(&ce->constants_table, &iface->constants_table, (copy_ctor_func_t) zval_add_ref, sizeof(zval *), (merge_checker_func_t) do_inherit_constant_check, iface);
		zend_hash_merge_ex(&ce->function_table, &iface->function_table, (copy_ctor_func_t) do_inherit_method, sizeof(zend_function), (merge_checker_func_t) do_inherit_method_check, ce);
	
		do_implement_interface(ce, iface TSRMLS_CC);
		zend_do_inherit_interfaces(ce, iface TSRMLS_CC);
	}
}
/* }}} */

ZEND_API int do_bind_function(zend_op *opline, HashTable *function_table, zend_bool compile_time) /* {{{ */
{
	zend_function *function;

	zend_hash_find(function_table, opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, (void *) &function);
	if (zend_hash_add(function_table, opline->op2.u.constant.value.str.val, opline->op2.u.constant.value.str.len+1, function, sizeof(zend_function), NULL)==FAILURE) {
		int error_level = compile_time ? E_COMPILE_ERROR : E_ERROR;
		zend_function *old_function;

		if (zend_hash_find(function_table, opline->op2.u.constant.value.str.val, opline->op2.u.constant.value.str.len+1, (void *) &old_function)==SUCCESS
			&& old_function->type == ZEND_USER_FUNCTION
			&& old_function->op_array.last > 0) {
			zend_error(error_level, "Cannot redeclare %s() (previously declared in %s:%d)",
						function->common.function_name,
						old_function->op_array.filename,
						old_function->op_array.opcodes[0].lineno);
		} else {
			zend_error(error_level, "Cannot redeclare %s()", function->common.function_name);
		}
		return FAILURE;
	} else {
		(*function->op_array.refcount)++;
		function->op_array.static_variables = NULL; /* NULL out the unbound function */
		return SUCCESS;
	}
}
/* }}} */

ZEND_API zend_class_entry *do_bind_class(const zend_op *opline, HashTable *class_table, zend_bool compile_time TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce, **pce;

	if (zend_hash_find(class_table, opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, (void **) &pce)==FAILURE) {
		zend_error(E_COMPILE_ERROR, "Internal Zend error - Missing class information for %s", opline->op1.u.constant.value.str.val);
		return NULL;
	} else {
		ce = *pce;
	}
	ce->refcount++;
	if (zend_hash_add(class_table, opline->op2.u.constant.value.str.val, opline->op2.u.constant.value.str.len+1, &ce, sizeof(zend_class_entry *), NULL)==FAILURE) {
		ce->refcount--;
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error(E_COMPILE_ERROR, "Cannot redeclare class %s", ce->name);
		}
		return NULL;
	} else {
		if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_IMPLEMENT_INTERFACES))) {
			zend_verify_abstract_class(ce TSRMLS_CC);
		}
		return ce;
	}
}
/* }}} */

ZEND_API zend_class_entry *do_bind_inherited_class(const zend_op *opline, HashTable *class_table, zend_class_entry *parent_ce, zend_bool compile_time TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce, **pce;
	int found_ce;

	found_ce = zend_hash_find(class_table, opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, (void **) &pce);

	if (found_ce == FAILURE) {
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error(E_COMPILE_ERROR, "Cannot redeclare class %s", opline->op2.u.constant.value.str.val);
		}
		return NULL;
	} else {
		ce = *pce;
	}

	if (parent_ce->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error(E_COMPILE_ERROR, "Class %s cannot extend from interface %s", ce->name, parent_ce->name);
	}

	zend_do_inheritance(ce, parent_ce TSRMLS_CC);

	ce->refcount++;

	/* Register the derived class */
	if (zend_hash_add(class_table, opline->op2.u.constant.value.str.val, opline->op2.u.constant.value.str.len+1, pce, sizeof(zend_class_entry *), NULL)==FAILURE) {
		zend_error(E_COMPILE_ERROR, "Cannot redeclare class %s", ce->name);
	}
	return ce;
}
/* }}} */

void zend_do_early_binding(TSRMLS_D) /* {{{ */
{
	zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
	HashTable *table;

	while (opline->opcode == ZEND_TICKS && opline > CG(active_op_array)->opcodes) {
		opline--;
	}

	switch (opline->opcode) {
		case ZEND_DECLARE_FUNCTION:
			if (do_bind_function(opline, CG(function_table), 1) == FAILURE) {
				return;
			}
			table = CG(function_table);
			break;
		case ZEND_DECLARE_CLASS:
			if (do_bind_class(opline, CG(class_table), 1 TSRMLS_CC) == NULL) {
				return;
			}
			table = CG(class_table);
			break;
		case ZEND_DECLARE_INHERITED_CLASS:
			{
				zend_op *fetch_class_opline = opline-1;
				zval *parent_name = &fetch_class_opline->op2.u.constant;
				zend_class_entry **pce;

				if ((zend_lookup_class(Z_STRVAL_P(parent_name), Z_STRLEN_P(parent_name), &pce TSRMLS_CC) == FAILURE) ||
				    ((CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_CLASSES) &&
				     ((*pce)->type == ZEND_INTERNAL_CLASS))) {
				    if (CG(compiler_options) & ZEND_COMPILE_DELAYED_BINDING) {
						zend_uint *opline_num = &CG(active_op_array)->early_binding;

						while (*opline_num != -1) {
							opline_num = &CG(active_op_array)->opcodes[*opline_num].result.u.opline_num;
						}
						*opline_num = opline - CG(active_op_array)->opcodes;
						opline->opcode = ZEND_DECLARE_INHERITED_CLASS_DELAYED;
						opline->result.op_type = IS_UNUSED;
						opline->result.u.opline_num = -1;
					}
					return;
				}
				if (do_bind_inherited_class(opline, CG(class_table), *pce, 1 TSRMLS_CC) == NULL) {
					return;
				}
				/* clear unnecessary ZEND_FETCH_CLASS opcode */
				zval_dtor(&fetch_class_opline->op2.u.constant);
				MAKE_NOP(fetch_class_opline);

				table = CG(class_table);
				break;
			}
		case ZEND_VERIFY_ABSTRACT_CLASS:
		case ZEND_ADD_INTERFACE:
			/* We currently don't early-bind classes that implement interfaces */
			return;
		default:
			zend_error(E_COMPILE_ERROR, "Invalid binding type");
			return;
	}

	zend_hash_del(table, opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len);
	zval_dtor(&opline->op1.u.constant);
	zval_dtor(&opline->op2.u.constant);
	MAKE_NOP(opline);
}
/* }}} */

ZEND_API void zend_do_delayed_early_binding(const zend_op_array *op_array TSRMLS_DC) /* {{{ */
{
	if (op_array->early_binding != -1) {
		zend_bool orig_in_compilation = CG(in_compilation);
		zend_uint opline_num = op_array->early_binding;
		zend_class_entry **pce;

		CG(in_compilation) = 1;
		while (opline_num != -1) {
			if (zend_lookup_class(Z_STRVAL(op_array->opcodes[opline_num-1].op2.u.constant), Z_STRLEN(op_array->opcodes[opline_num-1].op2.u.constant), &pce TSRMLS_CC) == SUCCESS) {
				do_bind_inherited_class(&op_array->opcodes[opline_num], EG(class_table), *pce, 1 TSRMLS_CC);
			}
			opline_num = op_array->opcodes[opline_num].result.u.opline_num;
		}
		CG(in_compilation) = orig_in_compilation;
	}
}
/* }}} */

void zend_do_boolean_or_begin(znode *expr1, znode *op_token TSRMLS_DC) /* {{{ */
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPNZ_EX;
	if (expr1->op_type == IS_TMP_VAR) {
		opline->result = *expr1;
	} else {
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->result.op_type = IS_TMP_VAR;
	}
	opline->op1 = *expr1;
	SET_UNUSED(opline->op2);

	op_token->u.opline_num = next_op_number;

	*expr1 = opline->result;
}
/* }}} */

void zend_do_boolean_or_end(znode *result, const znode *expr1, const znode *expr2, znode *op_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	*result = *expr1; /* we saved the original result in expr1 */
	opline->opcode = ZEND_BOOL;
	opline->result = *result;
	opline->op1 = *expr2;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[op_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
}
/* }}} */

void zend_do_boolean_and_begin(znode *expr1, znode *op_token TSRMLS_DC) /* {{{ */
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ_EX;
	if (expr1->op_type == IS_TMP_VAR) {
		opline->result = *expr1;
	} else {
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->result.op_type = IS_TMP_VAR;
	}
	opline->op1 = *expr1;
	SET_UNUSED(opline->op2);

	op_token->u.opline_num = next_op_number;

	*expr1 = opline->result;
}
/* }}} */

void zend_do_boolean_and_end(znode *result, const znode *expr1, const znode *expr2, const znode *op_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	*result = *expr1; /* we saved the original result in expr1 */
	opline->opcode = ZEND_BOOL;
	opline->result = *result;
	opline->op1 = *expr2;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[op_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
}
/* }}} */

void zend_do_do_while_begin(TSRMLS_D) /* {{{ */
{
	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_do_while_end(const znode *do_token, const znode *expr_open_bracket, const znode *expr TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPNZ;
	opline->op1 = *expr;
	opline->op2.u.opline_num = do_token->u.opline_num;
	SET_UNUSED(opline->op2);

	do_end_loop(expr_open_bracket->u.opline_num, 0 TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_brk_cont(zend_uchar op, const znode *expr TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->op1.u.opline_num = CG(active_op_array)->current_brk_cont;
	SET_UNUSED(opline->op1);
	if (expr) {
		opline->op2 = *expr;
	} else {
		Z_TYPE(opline->op2.u.constant) = IS_LONG;
		Z_LVAL(opline->op2.u.constant) = 1;
		INIT_PZVAL(&opline->op2.u.constant);
		opline->op2.op_type = IS_CONST;
	}
}
/* }}} */

void zend_do_switch_cond(const znode *cond TSRMLS_DC) /* {{{ */
{
	zend_switch_entry switch_entry;

	switch_entry.cond = *cond;
	switch_entry.default_case = -1;
	switch_entry.control_var = -1;
	zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry, sizeof(switch_entry));

	do_begin_loop(TSRMLS_C);

	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_switch_end(const znode *case_list TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	zend_switch_entry *switch_entry_ptr;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	/* add code to jmp to default case */
	if (switch_entry_ptr->default_case != -1) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_JMP;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
		opline->op1.u.opline_num = switch_entry_ptr->default_case;
	}

	if (case_list->op_type != IS_UNUSED) { /* non-empty switch */
		int next_op_number = get_next_op_number(CG(active_op_array));

		CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
	}

	/* remember break/continue loop information */
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].brk = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].parent;

	if (switch_entry_ptr->cond.op_type==IS_VAR || switch_entry_ptr->cond.op_type==IS_TMP_VAR) {
		/* emit free for the switch condition*/
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = (switch_entry_ptr->cond.op_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
		opline->op1 = switch_entry_ptr->cond;
		SET_UNUSED(opline->op2);
	}
	if (switch_entry_ptr->cond.op_type == IS_CONST) {
		zval_dtor(&switch_entry_ptr->cond.u.constant);
	}

	zend_stack_del_top(&CG(switch_cond_stack));

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_case_before_statement(const znode *case_list, znode *case_token, const znode *case_expr TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	int next_op_number;
	zend_switch_entry *switch_entry_ptr;
	znode result;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	if (switch_entry_ptr->control_var == -1) {
		switch_entry_ptr->control_var = get_temporary_variable(CG(active_op_array));
	}
	opline->opcode = ZEND_CASE;
	opline->result.u.var = switch_entry_ptr->control_var;
	opline->result.op_type = IS_TMP_VAR;
	opline->op1 = switch_entry_ptr->cond;
	opline->op2 = *case_expr;
	if (opline->op1.op_type == IS_CONST) {
		zval_copy_ctor(&opline->op1.u.constant);
	}
	result = opline->result;

	next_op_number = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_JMPZ;
	opline->op1 = result;
	SET_UNUSED(opline->op2);
	case_token->u.opline_num = next_op_number;

	if (case_list->op_type==IS_UNUSED) {
		return;
	}
	next_op_number = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
}
/* }}} */

void zend_do_case_after_statement(znode *result, const znode *case_token TSRMLS_DC) /* {{{ */
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	result->u.opline_num = next_op_number;

	switch (CG(active_op_array)->opcodes[case_token->u.opline_num].opcode) {
		case ZEND_JMP:
			CG(active_op_array)->opcodes[case_token->u.opline_num].op1.u.opline_num = get_next_op_number(CG(active_op_array));
			break;
		case ZEND_JMPZ:
			CG(active_op_array)->opcodes[case_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
			break;
	}
}
/* }}} */

void zend_do_default_before_statement(const znode *case_list, znode *default_token TSRMLS_DC) /* {{{ */
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_switch_entry *switch_entry_ptr;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	default_token->u.opline_num = next_op_number;

	next_op_number = get_next_op_number(CG(active_op_array));
	switch_entry_ptr->default_case = next_op_number;

	if (case_list->op_type==IS_UNUSED) {
		return;
	}
	CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
}
/* }}} */

void zend_do_begin_class_declaration(const znode *class_token, znode *class_name, const znode *parent_class_name TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	int doing_inheritance = 0;
	zend_class_entry *new_class_entry;
	char *lcname;
	int error = 0;
	zval **ns_name;

	if (CG(active_class_entry)) {
		zend_error(E_COMPILE_ERROR, "Class declarations may not be nested");
		return;
	}

	lcname = zend_str_tolower_dup(class_name->u.constant.value.str.val, class_name->u.constant.value.str.len);

	if (!(strcmp(lcname, "self") && strcmp(lcname, "parent"))) {
		efree(lcname);
		zend_error(E_COMPILE_ERROR, "Cannot use '%s' as class name as it is reserved", class_name->u.constant.value.str.val);
	}

	/* Class name must not conflict with import names */
	if (CG(current_import) &&
	    zend_hash_find(CG(current_import), lcname, Z_STRLEN(class_name->u.constant)+1, (void**)&ns_name) == SUCCESS) {
		error = 1;
	}

	if (CG(current_namespace)) {
		/* Prefix class name with name of current namespace */
		znode tmp;

		tmp.u.constant = *CG(current_namespace);
		zval_copy_ctor(&tmp.u.constant);
		zend_do_build_namespace_name(&tmp, &tmp, class_name TSRMLS_CC);
		class_name = &tmp;
		efree(lcname);
		lcname = zend_str_tolower_dup(Z_STRVAL(class_name->u.constant), Z_STRLEN(class_name->u.constant));
	}

	if (error) {
		char *tmp = zend_str_tolower_dup(Z_STRVAL_PP(ns_name), Z_STRLEN_PP(ns_name));

		if (Z_STRLEN_PP(ns_name) != Z_STRLEN(class_name->u.constant) ||
			memcmp(tmp, lcname, Z_STRLEN(class_name->u.constant))) {
			zend_error(E_COMPILE_ERROR, "Cannot declare class %s because the name is already in use", Z_STRVAL(class_name->u.constant));
		}
		efree(tmp);
	}

	new_class_entry = emalloc(sizeof(zend_class_entry));
	new_class_entry->type = ZEND_USER_CLASS;
	new_class_entry->name = class_name->u.constant.value.str.val;
	new_class_entry->name_length = class_name->u.constant.value.str.len;

	zend_initialize_class_data(new_class_entry, 1 TSRMLS_CC);
	new_class_entry->filename = zend_get_compiled_filename(TSRMLS_C);
	new_class_entry->line_start = class_token->u.opline_num;
	new_class_entry->ce_flags |= class_token->u.EA.type;

	if (parent_class_name && parent_class_name->op_type != IS_UNUSED) {
		switch (parent_class_name->u.EA.type) {
			case ZEND_FETCH_CLASS_SELF:
				zend_error(E_COMPILE_ERROR, "Cannot use 'self' as class name as it is reserved");
				break;
			case ZEND_FETCH_CLASS_PARENT:
				zend_error(E_COMPILE_ERROR, "Cannot use 'parent' as class name as it is reserved");
				break;
			case ZEND_FETCH_CLASS_STATIC:
				zend_error(E_COMPILE_ERROR, "Cannot use 'static' as class name as it is reserved");
				break;
			default:
				break;
		}
		doing_inheritance = 1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->op1.op_type = IS_CONST;
	build_runtime_defined_function_key(&opline->op1.u.constant, lcname, new_class_entry->name_length TSRMLS_CC);
	
	opline->op2.op_type = IS_CONST;
	opline->op2.u.constant.type = IS_STRING;
	Z_SET_REFCOUNT(opline->op2.u.constant, 1);

	if (doing_inheritance) {
		opline->extended_value = parent_class_name->u.var;
		opline->opcode = ZEND_DECLARE_INHERITED_CLASS;
	} else {
		opline->opcode = ZEND_DECLARE_CLASS;
	}

	opline->op2.u.constant.value.str.val = lcname;
	opline->op2.u.constant.value.str.len = new_class_entry->name_length;
	
	zend_hash_update(CG(class_table), opline->op1.u.constant.value.str.val, opline->op1.u.constant.value.str.len, &new_class_entry, sizeof(zend_class_entry *), NULL);
	CG(active_class_entry) = new_class_entry;

	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_VAR;
	CG(implementing_class) = opline->result;

	if (CG(doc_comment)) {
		CG(active_class_entry)->doc_comment = CG(doc_comment);
		CG(active_class_entry)->doc_comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}
}
/* }}} */

static void do_verify_abstract_class(TSRMLS_D) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_VERIFY_ABSTRACT_CLASS;
	opline->op1 = CG(implementing_class);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_end_class_declaration(const znode *class_token, const znode *parent_token TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = CG(active_class_entry);

	if (ce->constructor) {
		ce->constructor->common.fn_flags |= ZEND_ACC_CTOR;
		if (ce->constructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Constructor %s::%s() cannot be static", ce->name, ce->constructor->common.function_name);
		}
	}
	if (ce->destructor) {
		ce->destructor->common.fn_flags |= ZEND_ACC_DTOR;
		if (ce->destructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Destructor %s::%s() cannot be static", ce->name, ce->destructor->common.function_name);
		}
	}
	if (ce->clone) {
		ce->clone->common.fn_flags |= ZEND_ACC_CLONE;
		if (ce->clone->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Clone method %s::%s() cannot be static", ce->name, ce->clone->common.function_name);
		}
	}

	ce->line_end = zend_get_compiled_lineno(TSRMLS_C);

	if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS))
		&& ((parent_token->op_type != IS_UNUSED) || (ce->num_interfaces > 0))) {
		zend_verify_abstract_class(ce TSRMLS_CC);
		if (ce->num_interfaces) {
			do_verify_abstract_class(TSRMLS_C);
		}
	}
	/* Inherit interfaces; reset number to zero, we need it for above check and
	 * will restore it during actual implementation. 
	 * The ZEND_ACC_IMPLEMENT_INTERFACES flag disables double call to
	 * zend_verify_abstract_class() */
	if (ce->num_interfaces > 0) {
		ce->interfaces = NULL;
		ce->num_interfaces = 0;
		ce->ce_flags |= ZEND_ACC_IMPLEMENT_INTERFACES;
	}
	CG(active_class_entry) = NULL;
}
/* }}} */

void zend_do_implements_interface(znode *interface_name TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	switch (zend_get_class_fetch_type(Z_STRVAL(interface_name->u.constant), Z_STRLEN(interface_name->u.constant))) {
		case ZEND_FETCH_CLASS_SELF:
		case ZEND_FETCH_CLASS_PARENT:
		case ZEND_FETCH_CLASS_STATIC:
			zend_error(E_COMPILE_ERROR, "Cannot use '%s' as interface name as it is reserved", Z_STRVAL(interface_name->u.constant));
			break;
		default:
			break;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_ADD_INTERFACE;
	opline->op1 = CG(implementing_class);
	zend_resolve_class_name(interface_name, &opline->extended_value, 0 TSRMLS_CC);
	opline->extended_value = (opline->extended_value & ~ZEND_FETCH_CLASS_MASK) | ZEND_FETCH_CLASS_INTERFACE;
	opline->op2 = *interface_name;
	CG(active_class_entry)->num_interfaces++;
}
/* }}} */

ZEND_API void zend_mangle_property_name(char **dest, int *dest_length, const char *src1, int src1_length, const char *src2, int src2_length, int internal) /* {{{ */
{
	char *prop_name;
	int prop_name_length;

	prop_name_length = 1 + src1_length + 1 + src2_length;
	prop_name = pemalloc(prop_name_length + 1, internal);
	prop_name[0] = '\0';
	memcpy(prop_name + 1, src1, src1_length+1);
	memcpy(prop_name + 1 + src1_length + 1, src2, src2_length+1);

	*dest = prop_name;
	*dest_length = prop_name_length;
}
/* }}} */

static int zend_strnlen(const char* s, int maxlen) /* {{{ */
{
	int len = 0;
	while (*s++ && maxlen--) len++;
	return len;
}
/* }}} */

ZEND_API int zend_unmangle_property_name(char *mangled_property, int len, char **class_name, char **prop_name) /* {{{ */
{
	int class_name_len;

	*class_name = NULL;

	if (mangled_property[0]!=0) {
		*prop_name = mangled_property;
		return SUCCESS;
	}
	if (len < 3 || mangled_property[1]==0) {
		zend_error(E_NOTICE, "Illegal member variable name");
		*prop_name = mangled_property;
		return FAILURE;
	}

	class_name_len = zend_strnlen(mangled_property+1, --len - 1) + 1;
	if (class_name_len >= len || mangled_property[class_name_len]!=0) {
		zend_error(E_NOTICE, "Corrupt member variable name");
		*prop_name = mangled_property;
		return FAILURE;
	}
	*class_name = mangled_property+1;
	*prop_name = (*class_name)+class_name_len;
	return SUCCESS;
}
/* }}} */

void zend_do_declare_property(const znode *var_name, const znode *value, zend_uint access_type TSRMLS_DC) /* {{{ */
{
	zval *property;
	zend_property_info *existing_property_info;
	char *comment = NULL;
	int comment_len = 0;

	if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error(E_COMPILE_ERROR, "Interfaces may not include member variables");
	}

	if (access_type & ZEND_ACC_ABSTRACT) {
		zend_error(E_COMPILE_ERROR, "Properties cannot be declared abstract");
	}

	if (access_type & ZEND_ACC_FINAL) {
		zend_error(E_COMPILE_ERROR, "Cannot declare property %s::$%s final, the final modifier is allowed only for methods and classes",
				   CG(active_class_entry)->name, var_name->u.constant.value.str.val);
	}

	if (zend_hash_find(&CG(active_class_entry)->properties_info, var_name->u.constant.value.str.val, var_name->u.constant.value.str.len+1, (void **) &existing_property_info)==SUCCESS) {
		if (!(existing_property_info->flags & ZEND_ACC_IMPLICIT_PUBLIC)) {
			zend_error(E_COMPILE_ERROR, "Cannot redeclare %s::$%s", CG(active_class_entry)->name, var_name->u.constant.value.str.val);
		}
	}
	ALLOC_ZVAL(property);

	if (value) {
		*property = value->u.constant;
	} else {
		INIT_PZVAL(property);
		Z_TYPE_P(property) = IS_NULL;
	}

	if (CG(doc_comment)) {
		comment = CG(doc_comment);
		comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}

	zend_declare_property_ex(CG(active_class_entry), var_name->u.constant.value.str.val, var_name->u.constant.value.str.len, property, access_type, comment, comment_len TSRMLS_CC);
	efree(var_name->u.constant.value.str.val);
}
/* }}} */

void zend_do_declare_class_constant(znode *var_name, const znode *value TSRMLS_DC) /* {{{ */
{
	zval *property;

	if(Z_TYPE(value->u.constant) == IS_CONSTANT_ARRAY) {
		zend_error(E_COMPILE_ERROR, "Arrays are not allowed in class constants");
	}

	ALLOC_ZVAL(property);
	*property = value->u.constant;

	if (zend_hash_add(&CG(active_class_entry)->constants_table, var_name->u.constant.value.str.val, var_name->u.constant.value.str.len+1, &property, sizeof(zval *), NULL)==FAILURE) {
		FREE_ZVAL(property);
		zend_error(E_COMPILE_ERROR, "Cannot redefine class constant %s::%s", CG(active_class_entry)->name, var_name->u.constant.value.str.val);
	}
	FREE_PNODE(var_name);
	
	if (CG(doc_comment)) {
		efree(CG(doc_comment));
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}
}
/* }}} */

void zend_do_fetch_property(znode *result, znode *object, const znode *property TSRMLS_DC) /* {{{ */
{
	zend_op opline;
	zend_llist *fetch_list_ptr;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	if (object->op_type == IS_CV) {
		if (object->u.var == CG(active_op_array)->this_var) {
		    SET_UNUSED(*object); /* this means $this for objects */
	    }
	} else if (fetch_list_ptr->count == 1) {
		zend_llist_element *le = fetch_list_ptr->head;
		zend_op *opline_ptr = (zend_op *) le->data;

		if (opline_is_fetch_this(opline_ptr TSRMLS_CC)) {
			efree(Z_STRVAL(opline_ptr->op1.u.constant));
			SET_UNUSED(opline_ptr->op1); /* this means $this for objects */
			opline_ptr->op2 = *property;
			/* if it was usual fetch, we change it to object fetch */
			switch (opline_ptr->opcode) {
				case ZEND_FETCH_W:
					opline_ptr->opcode = ZEND_FETCH_OBJ_W;
					break;
				case ZEND_FETCH_R:
					opline_ptr->opcode = ZEND_FETCH_OBJ_R;
					break;
				case ZEND_FETCH_RW:
					opline_ptr->opcode = ZEND_FETCH_OBJ_RW;
					break;
				case ZEND_FETCH_IS:
					opline_ptr->opcode = ZEND_FETCH_OBJ_IS;
					break;
				case ZEND_FETCH_UNSET:
					opline_ptr->opcode = ZEND_FETCH_OBJ_UNSET;
					break;
				case ZEND_FETCH_FUNC_ARG:
					opline_ptr->opcode = ZEND_FETCH_OBJ_FUNC_ARG;
					break;
			}
			*result = opline_ptr->result;
			return;
		}
	}

	init_op(&opline TSRMLS_CC);
	opline.opcode = ZEND_FETCH_OBJ_W;	/* the backpatching routine assumes W */
	opline.result.op_type = IS_VAR;
	opline.result.u.EA.type = 0;
	opline.result.u.var = get_temporary_variable(CG(active_op_array));
	opline.op1 = *object;
	opline.op2 = *property;
	*result = opline.result;

	zend_llist_add_element(fetch_list_ptr, &opline);
}
/* }}} */

void zend_do_halt_compiler_register(TSRMLS_D) /* {{{ */
{
	char *name, *cfilename;
	char haltoff[] = "__COMPILER_HALT_OFFSET__";
	int len, clen;
	cfilename = zend_get_compiled_filename(TSRMLS_C);
	clen = strlen(cfilename);
	zend_mangle_property_name(&name, &len, haltoff, sizeof(haltoff) - 1, cfilename, clen, 0);
	zend_register_long_constant(name, len+1, zend_get_scanned_file_offset(TSRMLS_C), CONST_CS, 0 TSRMLS_CC);
	pefree(name, 0);
}
/* }}} */

void zend_do_declare_implicit_property(TSRMLS_D) /* {{{ */
{
/* Fixes bug #26182. Not sure why we needed to do this in the first place.
   Has to be checked with Zeev.
*/
#if ANDI_0
	zend_op *opline_ptr;
	zend_llist_element *le;
	zend_llist *fetch_list_ptr;


	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	if (fetch_list_ptr->count != 1) {
		return;
	}

	le = fetch_list_ptr->head;
	opline_ptr = (zend_op *) le->data;

	if (opline_ptr->op1.op_type == IS_UNUSED
		&& CG(active_class_entry)
		&& opline_ptr->op2.op_type == IS_CONST
		&& !zend_hash_exists(&CG(active_class_entry)->properties_info, opline_ptr->op2.u.constant.value.str.val, opline_ptr->op2.u.constant.value.str.len+1)) {
		znode property;

		property = opline_ptr->op2;
		property.u.constant.value.str.val = estrndup(opline_ptr->op2.u.constant.value.str.val, opline_ptr->op2.u.constant.value.str.len);
		zend_do_declare_property(&property, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_IMPLICIT_PUBLIC TSRMLS_CC);
	}
#endif
}
/* }}} */

void zend_do_push_object(const znode *object TSRMLS_DC) /* {{{ */
{
	zend_stack_push(&CG(object_stack), object, sizeof(znode));
}
/* }}} */

void zend_do_pop_object(znode *object TSRMLS_DC) /* {{{ */
{
	if (object) {
		znode *tmp;

		zend_stack_top(&CG(object_stack), (void **) &tmp);
		*object = *tmp;
	}
	zend_stack_del_top(&CG(object_stack));
}
/* }}} */

void zend_do_begin_new_object(znode *new_token, znode *class_type TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	unsigned char *ptr = NULL;

	new_token->u.opline_num = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_NEW;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *class_type;
	SET_UNUSED(opline->op2);

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(unsigned char *));
}
/* }}} */

void zend_do_end_new_object(znode *result, const znode *new_token, const znode *argument_list TSRMLS_DC) /* {{{ */
{
	znode ctor_result;

	zend_do_end_function_call(NULL, &ctor_result, argument_list, 1, 0 TSRMLS_CC);
	zend_do_free(&ctor_result TSRMLS_CC);

	CG(active_op_array)->opcodes[new_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
	*result = CG(active_op_array)->opcodes[new_token->u.opline_num].result;
}
/* }}} */

static zend_constant* zend_get_ct_const(const zval *const_name, int all_internal_constants_substitution TSRMLS_DC) /* {{{ */
{
	zend_constant *c = NULL;

	if (Z_STRVAL_P(const_name)[0] == '\\') {
		if (zend_hash_find(EG(zend_constants), Z_STRVAL_P(const_name)+1, Z_STRLEN_P(const_name), (void **) &c) == FAILURE) {
			char *lookup_name = zend_str_tolower_dup(Z_STRVAL_P(const_name)+1, Z_STRLEN_P(const_name)-1);

			if (zend_hash_find(EG(zend_constants), lookup_name, Z_STRLEN_P(const_name), (void **) &c)==SUCCESS) {
				if ((c->flags & CONST_CT_SUBST) && !(c->flags & CONST_CS)) {
					efree(lookup_name);
					return c;
				}
			}
			efree(lookup_name);
			return NULL;
		}
	} else if (zend_hash_find(EG(zend_constants), Z_STRVAL_P(const_name), Z_STRLEN_P(const_name)+1, (void **) &c) == FAILURE) {
		char *lookup_name = zend_str_tolower_dup(Z_STRVAL_P(const_name), Z_STRLEN_P(const_name));
		 
		if (zend_hash_find(EG(zend_constants), lookup_name, Z_STRLEN_P(const_name)+1, (void **) &c)==SUCCESS) {
			if ((c->flags & CONST_CT_SUBST) && !(c->flags & CONST_CS)) {
				efree(lookup_name);
				return c;
			}
		}
		efree(lookup_name);
		return NULL;
	}
	if (c->flags & CONST_CT_SUBST) {
		return c;
	}
	if (all_internal_constants_substitution &&
	    (c->flags & CONST_PERSISTENT) &&
	    !(CG(compiler_options) & ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION) &&
	    Z_TYPE(c->value) != IS_CONSTANT &&
	    Z_TYPE(c->value) != IS_CONSTANT_ARRAY) {
		return c;
	}
	return NULL;
}
/* }}} */

static int zend_constant_ct_subst(znode *result, zval *const_name, int all_internal_constants_substitution TSRMLS_DC) /* {{{ */
{
	zend_constant *c = zend_get_ct_const(const_name, all_internal_constants_substitution TSRMLS_CC);

	if (c) {
		zval_dtor(const_name);
		result->op_type = IS_CONST;
		result->u.constant = c->value;
		zval_copy_ctor(&result->u.constant);
		INIT_PZVAL(&result->u.constant);
		return 1;
	}
	return 0;
}
/* }}} */

void zend_do_fetch_constant(znode *result, znode *constant_container, znode *constant_name, int mode, zend_bool check_namespace TSRMLS_DC) /* {{{ */
{
	znode tmp;
	zend_op *opline;
	int type;
	char *compound;
	ulong fetch_type = 0;

	if (constant_container) {
		switch (mode) {
			case ZEND_CT:
				/* this is a class constant */
				type = zend_get_class_fetch_type(Z_STRVAL(constant_container->u.constant), Z_STRLEN(constant_container->u.constant));
	
				if (ZEND_FETCH_CLASS_STATIC == type) {
					zend_error(E_ERROR, "\"static::\" is not allowed in compile-time constants");
				} else if (ZEND_FETCH_CLASS_DEFAULT == type) {
					zend_resolve_class_name(constant_container, &fetch_type, 1 TSRMLS_CC);
				}
				zend_do_build_full_name(NULL, constant_container, constant_name, 1 TSRMLS_CC);
				*result = *constant_container;
				result->u.constant.type = IS_CONSTANT | fetch_type;
				break;
			case ZEND_RT:
				if (constant_container->op_type == IS_CONST &&
				ZEND_FETCH_CLASS_DEFAULT == zend_get_class_fetch_type(Z_STRVAL(constant_container->u.constant), Z_STRLEN(constant_container->u.constant))) {
					zend_resolve_class_name(constant_container, &fetch_type, 1 TSRMLS_CC);
				} else {
					zend_do_fetch_class(&tmp, constant_container TSRMLS_CC);
					constant_container = &tmp;
				}
				opline = get_next_op(CG(active_op_array) TSRMLS_CC);
				opline->opcode = ZEND_FETCH_CONSTANT;
				opline->result.op_type = IS_TMP_VAR;
				opline->result.u.var = get_temporary_variable(CG(active_op_array));
				opline->op1 = *constant_container;
				opline->op2 = *constant_name;
				*result = opline->result;
				break;
		}
		return;
	}
	/* namespace constant */
	/* only one that did not contain \ from the start can be converted to string if unknown */
	switch (mode) {
		case ZEND_CT:
			compound = memchr(Z_STRVAL(constant_name->u.constant), '\\', Z_STRLEN(constant_name->u.constant));
			/* this is a namespace constant, or an unprefixed constant */

			if (zend_constant_ct_subst(result, &constant_name->u.constant, 0 TSRMLS_CC)) {
				break;
			}

			zend_resolve_non_class_name(constant_name, check_namespace TSRMLS_CC);

			if(!compound) {
				fetch_type |= IS_CONSTANT_UNQUALIFIED;
			}

			*result = *constant_name;
			result->u.constant.type = IS_CONSTANT | fetch_type;
			break;
		case ZEND_RT:
			compound = memchr(Z_STRVAL(constant_name->u.constant), '\\', Z_STRLEN(constant_name->u.constant));

			zend_resolve_non_class_name(constant_name, check_namespace TSRMLS_CC);
			
			if(zend_constant_ct_subst(result, &constant_name->u.constant, 1 TSRMLS_CC)) {
				break;
			}

			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			opline->opcode = ZEND_FETCH_CONSTANT;
			opline->result.op_type = IS_TMP_VAR;
			opline->result.u.var = get_temporary_variable(CG(active_op_array));
			*result = opline->result;
			SET_UNUSED(opline->op1);
			if(compound) {
				/* the name is unambiguous */
				opline->extended_value = 0;
			} else {
				opline->extended_value = IS_CONSTANT_UNQUALIFIED;
			}
			opline->op2 = *constant_name;
			break;
	}
}
/* }}} */

void zend_do_shell_exec(znode *result, const znode *cmd TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	switch (cmd->op_type) {
		case IS_CONST:
		case IS_TMP_VAR:
			opline->opcode = ZEND_SEND_VAL;
			break;
		default:
			opline->opcode = ZEND_SEND_VAR;
			break;
	}
	opline->op1 = *cmd;
	opline->op2.u.opline_num = 0;
	opline->extended_value = ZEND_DO_FCALL;
	SET_UNUSED(opline->op2);

	/* FIXME: exception support not added to this op2 */
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_DO_FCALL;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_VAR;
	Z_STRVAL(opline->op1.u.constant) = estrndup("shell_exec", sizeof("shell_exec")-1);
	Z_STRLEN(opline->op1.u.constant) = sizeof("shell_exec")-1;
	INIT_PZVAL(&opline->op1.u.constant);
	Z_TYPE(opline->op1.u.constant) = IS_STRING;
	opline->op1.op_type = IS_CONST;
	opline->extended_value = 1;
	SET_UNUSED(opline->op2);
	ZVAL_LONG(&opline->op2.u.constant, zend_hash_func("shell_exec", sizeof("shell_exec")));
	*result = opline->result;
}
/* }}} */

void zend_do_init_array(znode *result, const znode *expr, const znode *offset, zend_bool is_ref TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_INIT_ARRAY;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_TMP_VAR;
	*result = opline->result;
	if (expr) {
		opline->op1 = *expr;
		if (offset) {
			opline->op2 = *offset;
		} else {
			SET_UNUSED(opline->op2);
		}
	} else {
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
	}
	opline->extended_value = is_ref;
}
/* }}} */

void zend_do_add_array_element(znode *result, const znode *expr, const znode *offset, zend_bool is_ref TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ADD_ARRAY_ELEMENT;
	opline->result = *result;
	opline->op1 = *expr;
	if (offset) {
		opline->op2 = *offset;
	} else {
		SET_UNUSED(opline->op2);
	}
	opline->extended_value = is_ref;
}
/* }}} */

void zend_do_add_static_array_element(znode *result, znode *offset, const znode *expr) /* {{{ */
{
	zval *element;

	ALLOC_ZVAL(element);
	*element = expr->u.constant;
	if (offset) {
		switch (offset->u.constant.type & IS_CONSTANT_TYPE_MASK) {
			case IS_CONSTANT:
				/* Ugly hack to denote that this value has a constant index */
				Z_TYPE_P(element) |= IS_CONSTANT_INDEX;
				Z_STRVAL(offset->u.constant) = erealloc(Z_STRVAL(offset->u.constant), Z_STRLEN(offset->u.constant)+3);
				Z_STRVAL(offset->u.constant)[Z_STRLEN(offset->u.constant)+1] = Z_TYPE(offset->u.constant);
				Z_STRVAL(offset->u.constant)[Z_STRLEN(offset->u.constant)+2] = 0;
				zend_symtable_update(result->u.constant.value.ht, Z_STRVAL(offset->u.constant), Z_STRLEN(offset->u.constant)+3, &element, sizeof(zval *), NULL);
				zval_dtor(&offset->u.constant);
				break;
			case IS_STRING:
				zend_symtable_update(result->u.constant.value.ht, offset->u.constant.value.str.val, offset->u.constant.value.str.len+1, &element, sizeof(zval *), NULL);
				zval_dtor(&offset->u.constant);
				break;
			case IS_NULL:
				zend_symtable_update(Z_ARRVAL(result->u.constant), "", 1, &element, sizeof(zval *), NULL);
				break;
			case IS_LONG:
			case IS_BOOL:
				zend_hash_index_update(Z_ARRVAL(result->u.constant), Z_LVAL(offset->u.constant), &element, sizeof(zval *), NULL);
				break;
			case IS_DOUBLE:
				zend_hash_index_update(Z_ARRVAL(result->u.constant), zend_dval_to_lval(Z_DVAL(offset->u.constant)), &element, sizeof(zval *), NULL);
				break;
			case IS_CONSTANT_ARRAY:
				zend_error(E_ERROR, "Illegal offset type");
				break;
		}
	} else {
		zend_hash_next_index_insert(Z_ARRVAL(result->u.constant), &element, sizeof(zval *), NULL);
	}
}
/* }}} */

void zend_do_add_list_element(const znode *element TSRMLS_DC) /* {{{ */
{
	list_llist_element lle;

	if (element) {
		zend_check_writable_variable(element);

		lle.var = *element;
		zend_llist_copy(&lle.dimensions, &CG(dimension_llist));
		zend_llist_prepend_element(&CG(list_llist), &lle);
	}
	(*((int *)CG(dimension_llist).tail->data))++;
}
/* }}} */

void zend_do_new_list_begin(TSRMLS_D) /* {{{ */
{
	int current_dimension = 0;
	zend_llist_add_element(&CG(dimension_llist), &current_dimension);
}
/* }}} */

void zend_do_new_list_end(TSRMLS_D) /* {{{ */
{
	zend_llist_remove_tail(&CG(dimension_llist));
	(*((int *)CG(dimension_llist).tail->data))++;
}
/* }}} */

void zend_do_list_init(TSRMLS_D) /* {{{ */
{
	zend_stack_push(&CG(list_stack), &CG(list_llist), sizeof(zend_llist));
	zend_stack_push(&CG(list_stack), &CG(dimension_llist), sizeof(zend_llist));
	zend_llist_init(&CG(list_llist), sizeof(list_llist_element), NULL, 0);
	zend_llist_init(&CG(dimension_llist), sizeof(int), NULL, 0);
	zend_do_new_list_begin(TSRMLS_C);
}
/* }}} */

void zend_do_list_end(znode *result, znode *expr TSRMLS_DC) /* {{{ */
{
	zend_llist_element *le;
	zend_llist_element *dimension;
	zend_op *opline;
	znode last_container;

	le = CG(list_llist).head;
	while (le) {
		zend_llist *tmp_dimension_llist = &((list_llist_element *)le->data)->dimensions;
		dimension = tmp_dimension_llist->head;
		while (dimension) {
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			if (dimension == tmp_dimension_llist->head) { /* first */
				last_container = *expr;
				switch (expr->op_type) {
					case IS_VAR:
					case IS_CV:
						opline->opcode = ZEND_FETCH_DIM_R;
						break;
					case IS_TMP_VAR:
						opline->opcode = ZEND_FETCH_DIM_TMP_VAR;
						break;
					case IS_CONST: /* fetch_dim_tmp_var will handle this bogus fetch */
						zval_copy_ctor(&expr->u.constant);
						opline->opcode = ZEND_FETCH_DIM_TMP_VAR;
						break;
				}
				opline->extended_value = ZEND_FETCH_ADD_LOCK;
			} else {
				opline->opcode = ZEND_FETCH_DIM_R;
			}
			opline->result.op_type = IS_VAR;
			opline->result.u.EA.type = 0;
			opline->result.u.var = get_temporary_variable(CG(active_op_array));
			opline->op1 = last_container;
			opline->op2.op_type = IS_CONST;
			Z_TYPE(opline->op2.u.constant) = IS_LONG;
			Z_LVAL(opline->op2.u.constant) = *((int *) dimension->data);
			INIT_PZVAL(&opline->op2.u.constant);
			last_container = opline->result;
			dimension = dimension->next;
		}
		((list_llist_element *) le->data)->value = last_container;
		zend_llist_destroy(&((list_llist_element *) le->data)->dimensions);
		zend_do_assign(result, &((list_llist_element *) le->data)->var, &((list_llist_element *) le->data)->value TSRMLS_CC);
		zend_do_free(result TSRMLS_CC);
		le = le->next;
	}
	zend_llist_destroy(&CG(dimension_llist));
	zend_llist_destroy(&CG(list_llist));
	*result = *expr;
	{
		zend_llist *p;

		/* restore previous lists */
		zend_stack_top(&CG(list_stack), (void **) &p);
		CG(dimension_llist) = *p;
		zend_stack_del_top(&CG(list_stack));
		zend_stack_top(&CG(list_stack), (void **) &p);
		CG(list_llist) = *p;
		zend_stack_del_top(&CG(list_stack));
	}
}
/* }}} */

void zend_do_fetch_static_variable(znode *varname, const znode *static_assignment, int fetch_type TSRMLS_DC) /* {{{ */
{
	zval *tmp;
	zend_op *opline;
	znode lval;
	znode result;

	ALLOC_ZVAL(tmp);

	if (static_assignment) {
		*tmp = static_assignment->u.constant;
	} else {
		INIT_ZVAL(*tmp);
	}
	if (!CG(active_op_array)->static_variables) {
		ALLOC_HASHTABLE(CG(active_op_array)->static_variables);
		zend_hash_init(CG(active_op_array)->static_variables, 2, NULL, ZVAL_PTR_DTOR, 0);
	}
	zend_hash_update(CG(active_op_array)->static_variables, varname->u.constant.value.str.val, varname->u.constant.value.str.len+1, &tmp, sizeof(zval *), NULL);

	if (varname->op_type == IS_CONST) {
		if (Z_TYPE(varname->u.constant) != IS_STRING) {
			convert_to_string(&varname->u.constant);
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = (fetch_type == ZEND_FETCH_LEXICAL) ? ZEND_FETCH_R : ZEND_FETCH_W;		/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *varname;
	SET_UNUSED(opline->op2);
	opline->op2.u.EA.type = ZEND_FETCH_STATIC;
	result = opline->result;

	if (varname->op_type == IS_CONST) {
		zval_copy_ctor(&varname->u.constant);
	}
	fetch_simple_variable(&lval, varname, 0 TSRMLS_CC); /* Relies on the fact that the default fetch is BP_VAR_W */

	if (fetch_type == ZEND_FETCH_LEXICAL) {
		znode dummy;

		zend_do_begin_variable_parse(TSRMLS_C);
		zend_do_assign(&dummy, &lval, &result TSRMLS_CC);
		zend_do_free(&dummy TSRMLS_CC);
	} else {
		zend_do_assign_ref(NULL, &lval, &result TSRMLS_CC);
	}
	CG(active_op_array)->opcodes[CG(active_op_array)->last-1].result.u.EA.type |= EXT_TYPE_UNUSED;

/*	zval_dtor(&varname->u.constant); */
}
/* }}} */

void zend_do_fetch_lexical_variable(znode *varname, zend_bool is_ref TSRMLS_DC) /* {{{ */
{
	znode value;

	if (Z_STRLEN(varname->u.constant) == sizeof("this") - 1 &&
	    memcmp(Z_STRVAL(varname->u.constant), "this", sizeof("this") - 1) == 0) {
		zend_error(E_COMPILE_ERROR, "Cannot use $this as lexical variable");
		return;
	}

	value.op_type = IS_CONST;
	ZVAL_NULL(&value.u.constant);
	Z_TYPE(value.u.constant) |= is_ref ? IS_LEXICAL_REF : IS_LEXICAL_VAR;
	Z_SET_REFCOUNT_P(&value.u.constant, 1);
	Z_UNSET_ISREF_P(&value.u.constant);
	
	zend_do_fetch_static_variable(varname, &value, is_ref ? ZEND_FETCH_STATIC : ZEND_FETCH_LEXICAL TSRMLS_CC);
}
/* }}} */

void zend_do_fetch_global_variable(znode *varname, const znode *static_assignment, int fetch_type TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	znode lval;
	znode result;

	if (varname->op_type == IS_CONST) {
		if (Z_TYPE(varname->u.constant) != IS_STRING) {
			convert_to_string(&varname->u.constant);
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_FETCH_W;		/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *varname;
	SET_UNUSED(opline->op2);
	opline->op2.u.EA.type = fetch_type;
	result = opline->result;

	if (varname->op_type == IS_CONST) {
		zval_copy_ctor(&varname->u.constant);
	}
	fetch_simple_variable(&lval, varname, 0 TSRMLS_CC); /* Relies on the fact that the default fetch is BP_VAR_W */

	zend_do_assign_ref(NULL, &lval, &result TSRMLS_CC);
	CG(active_op_array)->opcodes[CG(active_op_array)->last-1].result.u.EA.type |= EXT_TYPE_UNUSED;
}
/* }}} */

void zend_do_cast(znode *result, const znode *expr, int type TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_CAST;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
	opline->extended_value = type;
	*result = opline->result;
}
/* }}} */

void zend_do_include_or_eval(int type, znode *result, const znode *op1 TSRMLS_DC) /* {{{ */
{
	zend_do_extended_fcall_begin(TSRMLS_C);
	{
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_INCLUDE_OR_EVAL;
		opline->result.op_type = IS_VAR;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->op1 = *op1;
		SET_UNUSED(opline->op2);
		Z_LVAL(opline->op2.u.constant) = type;
		*result = opline->result;
	}
	zend_do_extended_fcall_end(TSRMLS_C);
}
/* }}} */

void zend_do_indirect_references(znode *result, const znode *num_references, znode *variable TSRMLS_DC) /* {{{ */
{
	int i;

	zend_do_end_variable_parse(variable, BP_VAR_R, 0 TSRMLS_CC);
	for (i=1; i<num_references->u.constant.value.lval; i++) {
		fetch_simple_variable_ex(result, variable, 0, ZEND_FETCH_R TSRMLS_CC);
		*variable = *result;
	}
	zend_do_begin_variable_parse(TSRMLS_C);
	fetch_simple_variable(result, variable, 1 TSRMLS_CC);
}
/* }}} */

void zend_do_unset(const znode *variable TSRMLS_DC) /* {{{ */
{
	zend_op *last_op;

	zend_check_writable_variable(variable);

	if (variable->op_type == IS_CV) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_UNSET_VAR;
		opline->op1 = *variable;
		SET_UNUSED(opline->op2);
		opline->op2.u.EA.type = ZEND_FETCH_LOCAL;
		SET_UNUSED(opline->result);
		opline->extended_value = ZEND_QUICK_SET;
	} else {
		last_op = &CG(active_op_array)->opcodes[get_next_op_number(CG(active_op_array))-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_UNSET:
				last_op->opcode = ZEND_UNSET_VAR;
				break;
			case ZEND_FETCH_DIM_UNSET:
				last_op->opcode = ZEND_UNSET_DIM;
				break;
			case ZEND_FETCH_OBJ_UNSET:
				last_op->opcode = ZEND_UNSET_OBJ;
				break;

		}
	}
}
/* }}} */

void zend_do_isset_or_isempty(int type, znode *result, znode *variable TSRMLS_DC) /* {{{ */
{
	zend_op *last_op;

	zend_do_end_variable_parse(variable, BP_VAR_IS, 0 TSRMLS_CC);

	zend_check_writable_variable(variable);

	if (variable->op_type == IS_CV) {
		last_op = get_next_op(CG(active_op_array) TSRMLS_CC);
		last_op->opcode = ZEND_ISSET_ISEMPTY_VAR;
		last_op->op1 = *variable;
		SET_UNUSED(last_op->op2);
		last_op->op2.u.EA.type = ZEND_FETCH_LOCAL;
		last_op->result.u.var = get_temporary_variable(CG(active_op_array));
		last_op->extended_value = ZEND_QUICK_SET;
	} else {
		last_op = &CG(active_op_array)->opcodes[get_next_op_number(CG(active_op_array))-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_VAR;
				break;
			case ZEND_FETCH_DIM_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_DIM_OBJ;
				break;
			case ZEND_FETCH_OBJ_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_PROP_OBJ;
				break;
		}
		last_op->extended_value = 0;
	}
	last_op->result.op_type = IS_TMP_VAR;
	last_op->extended_value |= type;

	*result = last_op->result;
}
/* }}} */

void zend_do_instanceof(znode *result, const znode *expr, const znode *class_znode, int type TSRMLS_DC) /* {{{ */
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		opline = &CG(active_op_array)->opcodes[last_op_number-1];
		if (opline->opcode == ZEND_FETCH_CLASS) {
			opline->extended_value |= ZEND_FETCH_CLASS_NO_AUTOLOAD;
		}
	}

	if (expr->op_type == IS_CONST) {
		zend_error(E_COMPILE_ERROR, "instanceof expects an object instance, constant given");
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INSTANCEOF;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *expr;

	opline->op2 = *class_znode;

	*result = opline->result;
}
/* }}} */

void zend_do_foreach_begin(znode *foreach_token, znode *open_brackets_token, znode *array, znode *as_token, int variable TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	zend_bool is_variable;
	zend_bool push_container = 0;
	zend_op dummy_opline;

	if (variable) {
		if (zend_is_function_or_method_call(array)) {
			is_variable = 0;
		} else {
			is_variable = 1;
		}
		/* save the location of FETCH_W instruction(s) */
		open_brackets_token->u.opline_num = get_next_op_number(CG(active_op_array));
		zend_do_end_variable_parse(array, BP_VAR_W, 0 TSRMLS_CC);
		if (CG(active_op_array)->last > 0 &&
		    CG(active_op_array)->opcodes[CG(active_op_array)->last-1].opcode == ZEND_FETCH_OBJ_W) {
			/* Only lock the container if we are fetching from a real container and not $this */
			if (CG(active_op_array)->opcodes[CG(active_op_array)->last-1].op1.op_type == IS_VAR) {
				CG(active_op_array)->opcodes[CG(active_op_array)->last-1].extended_value |= ZEND_FETCH_ADD_LOCK;
				push_container = 1;
			}
		}
	} else {
		is_variable = 0;
		open_brackets_token->u.opline_num = get_next_op_number(CG(active_op_array));
	}

	/* save the location of FE_RESET */
	foreach_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	/* Preform array reset */
	opline->opcode = ZEND_FE_RESET;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *array;
	SET_UNUSED(opline->op2);
	opline->extended_value = is_variable ? ZEND_FE_RESET_VARIABLE : 0;

	dummy_opline.result = opline->result;
	if (push_container) {
		dummy_opline.op1 = CG(active_op_array)->opcodes[CG(active_op_array)->last-2].op1;
	} else {
		znode tmp;

		tmp.op_type = IS_UNUSED;
		dummy_opline.op1 = tmp;
	}
	zend_stack_push(&CG(foreach_copy_stack), (void *) &dummy_opline, sizeof(zend_op));

	/* save the location of FE_FETCH */
	as_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_FE_FETCH;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = dummy_opline.result;
	opline->extended_value = 0;
	SET_UNUSED(opline->op2);

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_OP_DATA;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	SET_UNUSED(opline->result);
}
/* }}} */

void zend_do_foreach_cont(znode *foreach_token, const znode *open_brackets_token, const znode *as_token, znode *value, znode *key TSRMLS_DC) /* {{{ */
{
	zend_op *opline;
	znode dummy, value_node;
	zend_bool assign_by_ref=0;

	opline = &CG(active_op_array)->opcodes[as_token->u.opline_num];
	if (key->op_type != IS_UNUSED) {
		znode *tmp;

		/* switch between the key and value... */
		tmp = key;
		key = value;
		value = tmp;

		/* Mark extended_value in case both key and value are being used */
		opline->extended_value |= ZEND_FE_FETCH_WITH_KEY;
	}

	if ((key->op_type != IS_UNUSED) && (key->u.EA.type & ZEND_PARSED_REFERENCE_VARIABLE)) {
		zend_error(E_COMPILE_ERROR, "Key element cannot be a reference");
	}

	if (value->u.EA.type & ZEND_PARSED_REFERENCE_VARIABLE) {
		assign_by_ref = 1;
		if (!(opline-1)->extended_value) {
			zend_error(E_COMPILE_ERROR, "Cannot create references to elements of a temporary array expression");
		}
		/* Mark extended_value for assign-by-reference */
		opline->extended_value |= ZEND_FE_FETCH_BYREF;
		CG(active_op_array)->opcodes[foreach_token->u.opline_num].extended_value |= ZEND_FE_RESET_REFERENCE;
	} else {
		zend_op *foreach_copy;
		zend_op *fetch = &CG(active_op_array)->opcodes[foreach_token->u.opline_num];
		zend_op	*end = &CG(active_op_array)->opcodes[open_brackets_token->u.opline_num];

		/* Change "write context" into "read context" */
		fetch->extended_value = 0;  /* reset ZEND_FE_RESET_VARIABLE */
		while (fetch != end) {
			--fetch;
			if (fetch->opcode == ZEND_FETCH_DIM_W && fetch->op2.op_type == IS_UNUSED) {
				zend_error(E_COMPILE_ERROR, "Cannot use [] for reading");
			}
			fetch->opcode -= 3; /* FETCH_W -> FETCH_R */
		}
		/* prevent double SWITCH_FREE */
		zend_stack_top(&CG(foreach_copy_stack), (void **) &foreach_copy);
		foreach_copy->op1.op_type = IS_UNUSED;
	}

	value_node = opline->result;

	if (assign_by_ref) {
		zend_do_end_variable_parse(value, BP_VAR_W, 0 TSRMLS_CC);
		/* Mark FE_FETCH as IS_VAR as it holds the data directly as a value */
		zend_do_assign_ref(NULL, value, &value_node TSRMLS_CC);
	} else {
		zend_do_assign(&dummy, value, &value_node TSRMLS_CC);
		zend_do_free(&dummy TSRMLS_CC);
	}

	if (key->op_type != IS_UNUSED) {
		znode key_node;

		opline = &CG(active_op_array)->opcodes[as_token->u.opline_num+1];
		opline->result.op_type = IS_TMP_VAR;
		opline->result.u.EA.type = 0;
		opline->result.u.opline_num = get_temporary_variable(CG(active_op_array));
		key_node = opline->result;

		zend_do_assign(&dummy, key, &key_node TSRMLS_CC);
		zend_do_free(&dummy TSRMLS_CC);
	}

	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_foreach_end(const znode *foreach_token, const znode *as_token TSRMLS_DC) /* {{{ */
{
	zend_op *container_ptr;
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = as_token->u.opline_num;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[foreach_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array)); /* FE_RESET */
	CG(active_op_array)->opcodes[as_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array)); /* FE_FETCH */

	do_end_loop(as_token->u.opline_num, 1 TSRMLS_CC);

	zend_stack_top(&CG(foreach_copy_stack), (void **) &container_ptr);
	generate_free_foreach_copy(container_ptr TSRMLS_CC);
	zend_stack_del_top(&CG(foreach_copy_stack));

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_declare_begin(TSRMLS_D) /* {{{ */
{
	zend_stack_push(&CG(declare_stack), &CG(declarables), sizeof(zend_declarables));
}
/* }}} */

void zend_do_declare_stmt(znode *var, znode *val TSRMLS_DC) /* {{{ */
{
	if (!zend_binary_strcasecmp(var->u.constant.value.str.val, var->u.constant.value.str.len, "ticks", sizeof("ticks")-1)) {
		convert_to_long(&val->u.constant);
		CG(declarables).ticks = val->u.constant;
#ifdef ZEND_MULTIBYTE
	} else if (!zend_binary_strcasecmp(var->u.constant.value.str.val, var->u.constant.value.str.len, "encoding", sizeof("encoding")-1)) {
		zend_encoding *new_encoding, *old_encoding;
		zend_encoding_filter old_input_filter;

		if ((Z_TYPE(val->u.constant) & IS_CONSTANT_TYPE_MASK) == IS_CONSTANT) {
			zend_error(E_COMPILE_ERROR, "Cannot use constants as encoding");
		}

		/*
		 * Check that the pragma comes before any opcodes. If the compilation
		 * got as far as this, the previous portion of the script must have been
		 * parseable according to the .ini script_encoding setting. We still
		 * want to tell them to put declare() at the top.
		 */
		{
			int num = CG(active_op_array)->last;
			/* ignore ZEND_EXT_STMT and ZEND_TICKS */
			while (num > 0 &&
			       (CG(active_op_array)->opcodes[num-1].opcode == ZEND_EXT_STMT ||
			        CG(active_op_array)->opcodes[num-1].opcode == ZEND_TICKS)) {
				--num;
			}

			if (num > 0) {
				zend_error(E_COMPILE_ERROR, "Encoding declaration pragma must be the very first statement in the script");
			}
		}
		CG(encoding_declared) = 1;

		convert_to_string(&val->u.constant);
		new_encoding = zend_multibyte_fetch_encoding(val->u.constant.value.str.val);
		if (!new_encoding) {
			zend_error(E_COMPILE_WARNING, "Unsupported encoding [%s]", val->u.constant.value.str.val);
		} else {
			old_input_filter = LANG_SCNG(input_filter);
			old_encoding = LANG_SCNG(script_encoding);
			zend_multibyte_set_filter(new_encoding TSRMLS_CC);

			/* need to re-scan if input filter changed */
			if (old_input_filter != LANG_SCNG(input_filter) ||
				((old_input_filter == zend_multibyte_script_encoding_filter) &&
				 (new_encoding != old_encoding))) {
				zend_multibyte_yyinput_again(old_input_filter, old_encoding TSRMLS_CC);
			}
		}
		efree(val->u.constant.value.str.val);
#else  /* !ZEND_MULTIBYTE */
	} else if (!zend_binary_strcasecmp(var->u.constant.value.str.val, var->u.constant.value.str.len, "encoding", sizeof("encoding")-1)) {
		/* Do not generate any kind of warning for encoding declares */
		/* zend_error(E_COMPILE_WARNING, "Declare encoding [%s] not supported", val->u.constant.value.str.val); */
		zval_dtor(&val->u.constant);
#endif /* ZEND_MULTIBYTE */
	} else {
		zend_error(E_COMPILE_WARNING, "Unsupported declare '%s'", var->u.constant.value.str.val);
		zval_dtor(&val->u.constant);
	}
	zval_dtor(&var->u.constant);
}
/* }}} */

void zend_do_declare_end(const znode *declare_token TSRMLS_DC) /* {{{ */
{
	zend_declarables *declarables;

	zend_stack_top(&CG(declare_stack), (void **) &declarables);
	/* We should restore if there was more than (current - start) - (ticks?1:0) opcodes */
	if ((get_next_op_number(CG(active_op_array)) - declare_token->u.opline_num) - ((Z_LVAL(CG(declarables).ticks))?1:0)) {
		CG(declarables) = *declarables;
	}
}
/* }}} */

void zend_do_exit(znode *result, const znode *message TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXIT;
	opline->op1 = *message;
	SET_UNUSED(opline->op2);

	result->op_type = IS_CONST;
	Z_TYPE(result->u.constant) = IS_BOOL;
	Z_LVAL(result->u.constant) = 1;
}
/* }}} */

void zend_do_begin_silence(znode *strudel_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_BEGIN_SILENCE;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	*strudel_token = opline->result;
}
/* }}} */

void zend_do_end_silence(const znode *strudel_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_END_SILENCE;
	opline->op1 = *strudel_token;
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_jmp_set(const znode *value, znode *jmp_token, znode *colon_token TSRMLS_DC) /* {{{ */
{
	int op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP_SET;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *value;
	SET_UNUSED(opline->op2);
	
	*colon_token = opline->result;

	jmp_token->u.opline_num = op_number;

	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_jmp_set_else(znode *result, const znode *false_value, const znode *jmp_token, const znode *colon_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_QM_ASSIGN;
	opline->extended_value = 0;
	opline->result = *colon_token;
	opline->op1 = *false_value;
	SET_UNUSED(opline->op2);
	
	*result = opline->result;

	CG(active_op_array)->opcodes[jmp_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
	
	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_begin_qm_op(const znode *cond, znode *qm_token TSRMLS_DC) /* {{{ */
{
	int jmpz_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *cond;
	SET_UNUSED(opline->op2);
	opline->op2.u.opline_num = jmpz_op_number;
	*qm_token = opline->op2;

	INC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_qm_true(const znode *true_value, znode *qm_token, znode *colon_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	CG(active_op_array)->opcodes[qm_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array))+1; /* jmp over the ZEND_JMP */

	opline->opcode = ZEND_QM_ASSIGN;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *true_value;
	SET_UNUSED(opline->op2);

	*qm_token = opline->result;
	colon_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_qm_false(znode *result, const znode *false_value, const znode *qm_token, const znode *colon_token TSRMLS_DC) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_QM_ASSIGN;
	opline->result = *qm_token;
	opline->op1 = *false_value;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[colon_token->u.opline_num].op1.u.opline_num = get_next_op_number(CG(active_op_array));

	*result = opline->result;

	DEC_BPC(CG(active_op_array));
}
/* }}} */

void zend_do_extended_info(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_STMT;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_extended_fcall_begin(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_BEGIN;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_extended_fcall_end(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_END;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_ticks(TSRMLS_D) /* {{{ */
{
	if (Z_LVAL(CG(declarables).ticks)) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_TICKS;
		opline->op1.u.constant = CG(declarables).ticks;
		opline->op1.op_type = IS_CONST;
		SET_UNUSED(opline->op2);
	}
}
/* }}} */

void zend_auto_global_dtor(zend_auto_global *auto_global) /* {{{ */
{
	free(auto_global->name);
}
/* }}} */

zend_bool zend_is_auto_global(const char *name, uint name_len TSRMLS_DC) /* {{{ */
{
	zend_auto_global *auto_global;

	if (zend_hash_find(CG(auto_globals), name, name_len+1, (void **) &auto_global)==SUCCESS) {
		if (auto_global->armed) {
			auto_global->armed = auto_global->auto_global_callback(auto_global->name, auto_global->name_len TSRMLS_CC);
		}
		return 1;
	}
	return 0;
}
/* }}} */

int zend_register_auto_global(const char *name, uint name_len, zend_auto_global_callback auto_global_callback TSRMLS_DC) /* {{{ */
{
	zend_auto_global auto_global;

	auto_global.name = zend_strndup(name, name_len);
	auto_global.name_len = name_len;
	auto_global.auto_global_callback = auto_global_callback;

	return zend_hash_add(CG(auto_globals), name, name_len+1, &auto_global, sizeof(zend_auto_global), NULL);
}
/* }}} */

int zendlex(znode *zendlval TSRMLS_DC) /* {{{ */
{
	int retval;

	if (CG(increment_lineno)) {
		CG(zend_lineno)++;
		CG(increment_lineno) = 0;
	}

again:
	Z_TYPE(zendlval->u.constant) = IS_LONG;
	retval = lex_scan(&zendlval->u.constant TSRMLS_CC);
	switch (retval) {
		case T_COMMENT:
		case T_DOC_COMMENT:
		case T_OPEN_TAG:
		case T_WHITESPACE:
			goto again;

		case T_CLOSE_TAG:
			if (LANG_SCNG(yy_text)[LANG_SCNG(yy_leng)-1] != '>') {
				CG(increment_lineno) = 1;
			}
			if (CG(has_bracketed_namespaces) && !CG(in_namespace)) {
				goto again;				
			}
			retval = ';'; /* implicit ; */
			break;
		case T_OPEN_TAG_WITH_ECHO:
			retval = T_ECHO;
			break;
		case T_END_HEREDOC:
			efree(Z_STRVAL(zendlval->u.constant));
			break;
	}

	INIT_PZVAL(&zendlval->u.constant);
	zendlval->op_type = IS_CONST;
	return retval;
}
/* }}} */

ZEND_API void zend_initialize_class_data(zend_class_entry *ce, zend_bool nullify_handlers TSRMLS_DC) /* {{{ */
{
	zend_bool persistent_hashes = (ce->type == ZEND_INTERNAL_CLASS) ? 1 : 0;
	dtor_func_t zval_ptr_dtor_func = ((persistent_hashes) ? ZVAL_INTERNAL_PTR_DTOR : ZVAL_PTR_DTOR);

	ce->refcount = 1;
	ce->constants_updated = 0;
	ce->ce_flags = 0;

	ce->doc_comment = NULL;
	ce->doc_comment_len = 0;

	zend_hash_init_ex(&ce->default_properties, 0, NULL, zval_ptr_dtor_func, persistent_hashes, 0);
	zend_hash_init_ex(&ce->properties_info, 0, NULL, (dtor_func_t) (persistent_hashes ? zend_destroy_property_info_internal : zend_destroy_property_info), persistent_hashes, 0);
	zend_hash_init_ex(&ce->default_static_members, 0, NULL, zval_ptr_dtor_func, persistent_hashes, 0);
	zend_hash_init_ex(&ce->constants_table, 0, NULL, zval_ptr_dtor_func, persistent_hashes, 0);
	zend_hash_init_ex(&ce->function_table, 0, NULL, ZEND_FUNCTION_DTOR, persistent_hashes, 0);

	if (ce->type == ZEND_INTERNAL_CLASS) {
#ifdef ZTS
		int n = zend_hash_num_elements(CG(class_table));

		if (CG(static_members) && n >= CG(last_static_member)) {
			/* Support for run-time declaration: dl() */
			CG(last_static_member) = n+1;
			CG(static_members) = realloc(CG(static_members), (n+1)*sizeof(HashTable*));
			CG(static_members)[n] = NULL;
		}
		ce->static_members = (HashTable*)(zend_intptr_t)n;
#else
		ce->static_members = NULL;
#endif
	} else {
		ce->static_members = &ce->default_static_members;
	}

	if (nullify_handlers) {
		ce->constructor = NULL;
		ce->destructor = NULL;
		ce->clone = NULL;
		ce->__get = NULL;
		ce->__set = NULL;
		ce->__unset = NULL;
		ce->__isset = NULL;
		ce->__call = NULL;
		ce->__callstatic = NULL;
		ce->__tostring = NULL;
		ce->create_object = NULL;
		ce->get_iterator = NULL;
		ce->iterator_funcs.funcs = NULL;
		ce->interface_gets_implemented = NULL;
		ce->get_static_method = NULL;
		ce->parent = NULL;
		ce->num_interfaces = 0;
		ce->interfaces = NULL;
		ce->module = NULL;
		ce->serialize = NULL;
		ce->unserialize = NULL;
		ce->serialize_func = NULL;
		ce->unserialize_func = NULL;
		ce->builtin_functions = NULL;
	}
}
/* }}} */

int zend_get_class_fetch_type(const char *class_name, uint class_name_len) /* {{{ */
{
	if ((class_name_len == sizeof("self")-1) &&
		!memcmp(class_name, "self", sizeof("self")-1)) {
		return ZEND_FETCH_CLASS_SELF;		
	} else if ((class_name_len == sizeof("parent")-1) &&
		!memcmp(class_name, "parent", sizeof("parent")-1)) {
		return ZEND_FETCH_CLASS_PARENT;
	} else if ((class_name_len == sizeof("static")-1) &&
		!memcmp(class_name, "static", sizeof("static")-1)) {
		return ZEND_FETCH_CLASS_STATIC;
	} else {
		return ZEND_FETCH_CLASS_DEFAULT;
	}
}
/* }}} */

ZEND_API char* zend_get_compiled_variable_name(const zend_op_array *op_array, zend_uint var, int* name_len) /* {{{ */
{
	if (name_len) {
		*name_len = op_array->vars[var].name_len;
	}
	return op_array->vars[var].name;
}
/* }}} */

void zend_do_build_namespace_name(znode *result, znode *prefix, znode *name TSRMLS_DC) /* {{{ */
{
	if (prefix) {
		*result = *prefix;
		if (Z_TYPE(result->u.constant) == IS_STRING &&
		    Z_STRLEN(result->u.constant) == 0) {
			/* namespace\ */
			if (CG(current_namespace)) {
				znode tmp;

				zval_dtor(&result->u.constant);
				tmp.op_type = IS_CONST;
				tmp.u.constant = *CG(current_namespace);
				zval_copy_ctor(&tmp.u.constant);
				zend_do_build_namespace_name(result, NULL, &tmp TSRMLS_CC);
			}
		}
	} else {
		result->op_type = IS_CONST;
		Z_TYPE(result->u.constant) = IS_STRING;
		Z_STRVAL(result->u.constant) = NULL;
		Z_STRLEN(result->u.constant) = 0;
	}
	/* prefix = result */
	zend_do_build_full_name(NULL, result, name, 0 TSRMLS_CC);
}
/* }}} */

void zend_do_begin_namespace(const znode *name, zend_bool with_bracket TSRMLS_DC) /* {{{ */
{
	char *lcname;

	/* handle mixed syntax declaration or nested namespaces */
	if (!CG(has_bracketed_namespaces)) {
		if (CG(current_namespace)) {
			/* previous namespace declarations were unbracketed */
			if (with_bracket) {
				zend_error(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations with unbracketed namespace declarations");
			}
		}
	} else {
		/* previous namespace declarations were bracketed */
		if (!with_bracket) {
			zend_error(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations with unbracketed namespace declarations");
		} else if (CG(current_namespace) || CG(in_namespace)) {
			zend_error(E_COMPILE_ERROR, "Namespace declarations cannot be nested");
		}
	}

	if (((!with_bracket && !CG(current_namespace)) || (with_bracket && !CG(has_bracketed_namespaces))) && CG(active_op_array)->last > 0) {
		/* ignore ZEND_EXT_STMT and ZEND_TICKS */
		int num = CG(active_op_array)->last;
		while (num > 0 &&
		       (CG(active_op_array)->opcodes[num-1].opcode == ZEND_EXT_STMT ||
		        CG(active_op_array)->opcodes[num-1].opcode == ZEND_TICKS)) {
			--num;
		}
		if (num > 0) {
			zend_error(E_COMPILE_ERROR, "Namespace declaration statement has to be the very first statement in the script");
		}
	}

	CG(in_namespace) = 1;
	if (with_bracket) {
		CG(has_bracketed_namespaces) = 1;
	}

	if (name) {
		lcname = zend_str_tolower_dup(Z_STRVAL(name->u.constant), Z_STRLEN(name->u.constant));
		if (((Z_STRLEN(name->u.constant) == sizeof("self")-1) &&
		      !memcmp(lcname, "self", sizeof("self")-1)) ||
		    ((Z_STRLEN(name->u.constant) == sizeof("parent")-1) &&
	          !memcmp(lcname, "parent", sizeof("parent")-1))) {
			zend_error(E_COMPILE_ERROR, "Cannot use '%s' as namespace name", Z_STRVAL(name->u.constant));
		}
		efree(lcname);

		if (CG(current_namespace)) {
			zval_dtor(CG(current_namespace));
		} else {
			ALLOC_ZVAL(CG(current_namespace));
		}
		*CG(current_namespace) = name->u.constant;
	} else {
		if (CG(current_namespace)) {
			zval_dtor(CG(current_namespace));
			FREE_ZVAL(CG(current_namespace));
			CG(current_namespace) = NULL;
		}
	}

	if (CG(current_import)) {
		zend_hash_destroy(CG(current_import));
		efree(CG(current_import));
		CG(current_import) = NULL;
	}
}
/* }}} */

void zend_do_use(znode *ns_name, znode *new_name, int is_global TSRMLS_DC) /* {{{ */
{
	char *lcname;
	zval *name, *ns, tmp;
	zend_bool warn = 0;
	zend_class_entry **pce;

	if (!CG(current_import)) {
		CG(current_import) = emalloc(sizeof(HashTable));
		zend_hash_init(CG(current_import), 0, NULL, ZVAL_PTR_DTOR, 0);
	}

	ALLOC_ZVAL(ns);
	*ns = ns_name->u.constant;
	if (new_name) {
		name = &new_name->u.constant;
	} else {
		char *p;

		/* The form "use A\B" is eqivalent to "use A\B as B".
		   So we extract the last part of compound name to use as a new_name */
		name = &tmp;
		p = zend_memrchr(Z_STRVAL_P(ns), '\\', Z_STRLEN_P(ns));
		if (p) {
			ZVAL_STRING(name, p+1, 1);
		} else {
			*name = *ns;
			zval_copy_ctor(name);
			warn = !is_global && !CG(current_namespace);
		}
	}

	lcname = zend_str_tolower_dup(Z_STRVAL_P(name), Z_STRLEN_P(name));

	if (((Z_STRLEN_P(name) == sizeof("self")-1) &&
          !memcmp(lcname, "self", sizeof("self")-1)) ||
	    ((Z_STRLEN_P(name) == sizeof("parent")-1) &&
          !memcmp(lcname, "parent", sizeof("parent")-1))) {
		zend_error(E_COMPILE_ERROR, "Cannot use %s as %s because '%s' is a special class name", Z_STRVAL_P(ns), Z_STRVAL_P(name), Z_STRVAL_P(name));
	}

	if (CG(current_namespace)) {
		/* Prefix import name with current namespace name to avoid conflicts with classes */
		char *c_ns_name = emalloc(Z_STRLEN_P(CG(current_namespace)) + 1 + Z_STRLEN_P(name) + 1);

		zend_str_tolower_copy(c_ns_name, Z_STRVAL_P(CG(current_namespace)), Z_STRLEN_P(CG(current_namespace)));
		c_ns_name[Z_STRLEN_P(CG(current_namespace))] = '\\';
		memcpy(c_ns_name+Z_STRLEN_P(CG(current_namespace))+1, lcname, Z_STRLEN_P(name)+1);
		if (zend_hash_exists(CG(class_table), c_ns_name, Z_STRLEN_P(CG(current_namespace)) + 1 + Z_STRLEN_P(name)+1)) {
			char *tmp = zend_str_tolower_dup(Z_STRVAL_P(ns), Z_STRLEN_P(ns));

			if (Z_STRLEN_P(ns) != Z_STRLEN_P(CG(current_namespace)) + 1 + Z_STRLEN_P(name) ||
				memcmp(tmp, c_ns_name, Z_STRLEN_P(ns))) {
				zend_error(E_COMPILE_ERROR, "Cannot use %s as %s because the name is already in use", Z_STRVAL_P(ns), Z_STRVAL_P(name));
			}
			efree(tmp);
		}
		efree(c_ns_name);
	} else if (zend_hash_find(CG(class_table), lcname, Z_STRLEN_P(name)+1, (void**)&pce) == SUCCESS &&
	           (*pce)->type == ZEND_USER_CLASS &&
	           (*pce)->filename == CG(compiled_filename)) {
		char *c_tmp = zend_str_tolower_dup(Z_STRVAL_P(ns), Z_STRLEN_P(ns));

		if (Z_STRLEN_P(ns) != Z_STRLEN_P(name) ||
			memcmp(c_tmp, lcname, Z_STRLEN_P(ns))) {
			zend_error(E_COMPILE_ERROR, "Cannot use %s as %s because the name is already in use", Z_STRVAL_P(ns), Z_STRVAL_P(name));
		}
		efree(c_tmp);
	}

	if (zend_hash_add(CG(current_import), lcname, Z_STRLEN_P(name)+1, &ns, sizeof(zval*), NULL) != SUCCESS) {
		zend_error(E_COMPILE_ERROR, "Cannot use %s as %s because the name is already in use", Z_STRVAL_P(ns), Z_STRVAL_P(name));
	}
	if (warn) {
		zend_error(E_WARNING, "The use statement with non-compound name '%s' has no effect", Z_STRVAL_P(name));
	}
	efree(lcname);
	zval_dtor(name);
}
/* }}} */

void zend_do_declare_constant(znode *name, znode *value TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if(Z_TYPE(value->u.constant) == IS_CONSTANT_ARRAY) {
		zend_error(E_COMPILE_ERROR, "Arrays are not allowed as constants");
	}

	if (zend_get_ct_const(&name->u.constant, 0 TSRMLS_CC)) {
		zend_error(E_COMPILE_ERROR, "Cannot redeclare constant '%s'", Z_STRVAL(name->u.constant));
	}

	if (CG(current_namespace)) {
		/* Prefix constant name with name of current namespace, lowercased */
		znode tmp;

		tmp.op_type = IS_CONST;
		tmp.u.constant = *CG(current_namespace);
		Z_STRVAL(tmp.u.constant) = zend_str_tolower_dup(Z_STRVAL(tmp.u.constant), Z_STRLEN(tmp.u.constant));
		zend_do_build_namespace_name(&tmp, &tmp, name TSRMLS_CC);
		*name = tmp;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_DECLARE_CONST;
	SET_UNUSED(opline->result);
	opline->op1 = *name;
	opline->op2 = *value;
}
/* }}} */

void zend_verify_namespace(TSRMLS_D) /* {{{ */
{
	if (CG(has_bracketed_namespaces) && !CG(in_namespace)) {
		zend_error(E_COMPILE_ERROR, "No code may exist outside of namespace {}");
	}
}
/* }}} */

void zend_do_end_namespace(TSRMLS_D) /* {{{ */
{
	CG(in_namespace) = 0;
	if (CG(current_namespace)) {
		zval_dtor(CG(current_namespace));
		FREE_ZVAL(CG(current_namespace));
		CG(current_namespace) = NULL;
	}
	if (CG(current_import)) {
		zend_hash_destroy(CG(current_import));
		efree(CG(current_import));
		CG(current_import) = NULL;
	}
}
/* }}} */

void zend_do_end_compilation(TSRMLS_D) /* {{{ */
{
	CG(has_bracketed_namespaces) = 0;
	zend_do_end_namespace(TSRMLS_C);
}
/* }}} */

/* {{{ zend_dirname
   Returns directory name component of path */
ZEND_API size_t zend_dirname(char *path, size_t len)
{
	register char *end = path + len - 1;
	unsigned int len_adjust = 0;

#ifdef PHP_WIN32
	/* Note that on Win32 CWD is per drive (heritage from CP/M).
	 * This means dirname("c:foo") maps to "c:." or "c:" - which means CWD on C: drive.
	 */
	if ((2 <= len) && isalpha((int)((unsigned char *)path)[0]) && (':' == path[1])) {
		/* Skip over the drive spec (if any) so as not to change */
		path += 2;
		len_adjust += 2;
		if (2 == len) {
			/* Return "c:" on Win32 for dirname("c:").
			 * It would be more consistent to return "c:." 
			 * but that would require making the string *longer*.
			 */
			return len;
		}
	}
#elif defined(NETWARE)
	/*
	 * Find the first occurence of : from the left 
	 * move the path pointer to the position just after :
	 * increment the len_adjust to the length of path till colon character(inclusive)
	 * If there is no character beyond : simple return len
	 */
	char *colonpos = NULL;
	colonpos = strchr(path, ':');
	if (colonpos != NULL) {
		len_adjust = ((colonpos - path) + 1);
		path += len_adjust;
		if (len_adjust == len) {
			return len;
		}
    }
#endif

	if (len == 0) {
		/* Illegal use of this function */
		return 0;
	}

	/* Strip trailing slashes */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* The path only contained slashes */
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}

	/* Strip filename */
	while (end >= path && !IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* No slash found, therefore return '.' */
#ifdef NETWARE
		if (len_adjust == 0) {
			path[0] = '.';
			path[1] = '\0';
			return 1; /* only one character */
		} else {
			path[0] = '\0';
			return len_adjust;
		}
#else
		path[0] = '.';
		path[1] = '\0';
		return 1 + len_adjust;
#endif
	}

	/* Strip slashes which came before the file name */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}
	*(end+1) = '\0';

	return (size_t)(end + 1 - path) + len_adjust;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
