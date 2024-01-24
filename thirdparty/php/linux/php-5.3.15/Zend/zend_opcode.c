/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2012 Zend Technologies Ltd. (http://www.zend.com) |
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

/* $Id$ */

#include <stdio.h>

#include "zend.h"
#include "zend_alloc.h"
#include "zend_compile.h"
#include "zend_extensions.h"
#include "zend_API.h"

#include "zend_vm.h"

static void zend_extension_op_array_ctor_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->op_array_ctor) {
		extension->op_array_ctor(op_array);
	}
}

static void zend_extension_op_array_dtor_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->op_array_dtor) {
		extension->op_array_dtor(op_array);
	}
}

static void op_array_alloc_ops(zend_op_array *op_array)
{
	op_array->opcodes = erealloc(op_array->opcodes, (op_array->size)*sizeof(zend_op));
}

void init_op_array(zend_op_array *op_array, zend_uchar type, int initial_ops_size TSRMLS_DC)
{
	op_array->type = type;

	op_array->backpatch_count = 0;
	if (CG(interactive)) {
		/* We must avoid a realloc() on the op_array in interactive mode, since pointers to constants
		 * will become invalid
		 */
		initial_ops_size = 8192;
	}

	op_array->refcount = (zend_uint *) emalloc(sizeof(zend_uint));
	*op_array->refcount = 1;
	op_array->size = initial_ops_size;
	op_array->last = 0;
	op_array->opcodes = NULL;
	op_array_alloc_ops(op_array);

	op_array->size_var = 0; /* FIXME:??? */
	op_array->last_var = 0;
	op_array->vars = NULL;

	op_array->T = 0;

	op_array->function_name = NULL;
	op_array->filename = zend_get_compiled_filename(TSRMLS_C);
	op_array->doc_comment = NULL;
	op_array->doc_comment_len = 0;

	op_array->arg_info = NULL;
	op_array->num_args = 0;
	op_array->required_num_args = 0;

	op_array->scope = NULL;

	op_array->brk_cont_array = NULL;
	op_array->try_catch_array = NULL;
	op_array->last_brk_cont = 0;
	op_array->current_brk_cont = -1;

	op_array->static_variables = NULL;
	op_array->last_try_catch = 0;

	op_array->return_reference = 0;
	op_array->done_pass_two = 0;

	op_array->this_var = -1;

	op_array->start_op = NULL;

	op_array->fn_flags = CG(interactive)?ZEND_ACC_INTERACTIVE:0;

	op_array->early_binding = -1;

	memset(op_array->reserved, 0, ZEND_MAX_RESERVED_RESOURCES * sizeof(void*));

	zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_ctor_handler, op_array TSRMLS_CC);
}

ZEND_API void destroy_zend_function(zend_function *function TSRMLS_DC)
{
	switch (function->type) {
		case ZEND_USER_FUNCTION:
			destroy_op_array((zend_op_array *) function TSRMLS_CC);
			break;
		case ZEND_INTERNAL_FUNCTION:
			/* do nothing */
			break;
	}
}

ZEND_API void zend_function_dtor(zend_function *function)
{
	TSRMLS_FETCH();

	destroy_zend_function(function TSRMLS_CC);
}

static void zend_cleanup_op_array_data(zend_op_array *op_array)
{
	if (op_array->static_variables) {
		zend_hash_clean(op_array->static_variables);
	}
}

ZEND_API int zend_cleanup_function_data(zend_function *function TSRMLS_DC)
{
	if (function->type == ZEND_USER_FUNCTION) {
		zend_cleanup_op_array_data((zend_op_array *) function);
		return ZEND_HASH_APPLY_KEEP;
	} else {
		return ZEND_HASH_APPLY_STOP;
	}
}

ZEND_API int zend_cleanup_function_data_full(zend_function *function TSRMLS_DC)
{
	if (function->type == ZEND_USER_FUNCTION) {
		zend_cleanup_op_array_data((zend_op_array *) function);
	}
	return 0;
}

ZEND_API int zend_cleanup_class_data(zend_class_entry **pce TSRMLS_DC)
{
	if ((*pce)->type == ZEND_USER_CLASS) {
		/* Clean all parts that can contain run-time data */
		/* Note that only run-time accessed data need to be cleaned up, pre-defined data can
		   not contain objects and thus are not probelmatic */
		zend_hash_apply(&(*pce)->function_table, (apply_func_t) zend_cleanup_function_data_full TSRMLS_CC);
		if ((*pce)->static_members) {
			zend_hash_clean((*pce)->static_members);
			(*pce)->static_members = NULL;
		}
	} else if (CE_STATIC_MEMBERS(*pce)) {
		zend_hash_destroy(CE_STATIC_MEMBERS(*pce));
		FREE_HASHTABLE(CE_STATIC_MEMBERS(*pce));
#ifdef ZTS
		CG(static_members)[(zend_intptr_t)((*pce)->static_members)] = NULL;
#else
		(*pce)->static_members = NULL;
#endif
	}
	return 0;
}

ZEND_API void destroy_zend_class(zend_class_entry **pce)
{
	zend_class_entry *ce = *pce;
	
	if (--ce->refcount > 0) {
		return;
	}
	switch (ce->type) {
		case ZEND_USER_CLASS:
			zend_hash_destroy(&ce->default_properties);
			zend_hash_destroy(&ce->properties_info);
			zend_hash_destroy(&ce->default_static_members);
			efree(ce->name);
			zend_hash_destroy(&ce->function_table);
			zend_hash_destroy(&ce->constants_table);
			if (ce->num_interfaces > 0 && ce->interfaces) {
				efree(ce->interfaces);
			}
			if (ce->doc_comment) {
				efree(ce->doc_comment);
			}
			efree(ce);
			break;
		case ZEND_INTERNAL_CLASS:
			zend_hash_destroy(&ce->default_properties);
			zend_hash_destroy(&ce->properties_info);
			zend_hash_destroy(&ce->default_static_members);
			free(ce->name);
			zend_hash_destroy(&ce->function_table);
			zend_hash_destroy(&ce->constants_table);
			if (ce->num_interfaces > 0) {
				free(ce->interfaces);
			}
			if (ce->doc_comment) {
				free(ce->doc_comment);
			}
			free(ce);
			break;
	}
}

void zend_class_add_ref(zend_class_entry **ce)
{
	(*ce)->refcount++;
}

ZEND_API void destroy_op_array(zend_op_array *op_array TSRMLS_DC)
{
	zend_op *opline = op_array->opcodes;
	zend_op *end = op_array->opcodes+op_array->last;
	zend_uint i;

	if (op_array->static_variables) {
		zend_hash_destroy(op_array->static_variables);
		FREE_HASHTABLE(op_array->static_variables);
	}

	if (--(*op_array->refcount)>0) {
		return;
	}

	efree(op_array->refcount);

	if (op_array->vars) {
		i = op_array->last_var;
		while (i > 0) {
			i--;
			efree(op_array->vars[i].name);
		}
		efree(op_array->vars);
	}

	while (opline<end) {
		if (opline->op1.op_type==IS_CONST) {
#if DEBUG_ZEND>2
			printf("Reducing refcount for %x 1=>0 (destroying)\n", &opline->op1.u.constant);
#endif
			zval_dtor(&opline->op1.u.constant);
		}
		if (opline->op2.op_type==IS_CONST) {
#if DEBUG_ZEND>2
			printf("Reducing refcount for %x 1=>0 (destroying)\n", &opline->op2.u.constant);
#endif
			zval_dtor(&opline->op2.u.constant);
		}
		opline++;
	}
	efree(op_array->opcodes);

	if (op_array->function_name) {
		efree(op_array->function_name);
	}
	if (op_array->doc_comment) {
		efree(op_array->doc_comment);
	}
	if (op_array->brk_cont_array) {
		efree(op_array->brk_cont_array);
	}
	if (op_array->try_catch_array) {
		efree(op_array->try_catch_array);
	}
	if (op_array->done_pass_two) {
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_dtor_handler, op_array TSRMLS_CC);
	}
	if (op_array->arg_info) {
		for (i=0; i<op_array->num_args; i++) {
			efree((char*)op_array->arg_info[i].name);
			if (op_array->arg_info[i].class_name) {
				efree((char*)op_array->arg_info[i].class_name);
			}
		}
		efree(op_array->arg_info);
	}
}

void init_op(zend_op *op TSRMLS_DC)
{
	memset(op, 0, sizeof(zend_op));
	op->lineno = CG(zend_lineno);
	SET_UNUSED(op->result);
}

zend_op *get_next_op(zend_op_array *op_array TSRMLS_DC)
{
	zend_uint next_op_num = op_array->last++;
	zend_op *next_op;

	if (next_op_num >= op_array->size) {
		if (op_array->fn_flags & ZEND_ACC_INTERACTIVE) {
			/* we messed up */
			zend_printf("Ran out of opcode space!\n"
						"You should probably consider writing this huge script into a file!\n");
			zend_bailout();
		}
		op_array->size *= 4;
		op_array_alloc_ops(op_array);
	}
	
	next_op = &(op_array->opcodes[next_op_num]);
	
	init_op(next_op TSRMLS_CC);

	return next_op;
}

int get_next_op_number(zend_op_array *op_array)
{
	return op_array->last;
}

zend_brk_cont_element *get_next_brk_cont_element(zend_op_array *op_array)
{
	op_array->last_brk_cont++;
	op_array->brk_cont_array = erealloc(op_array->brk_cont_array, sizeof(zend_brk_cont_element)*op_array->last_brk_cont);
	return &op_array->brk_cont_array[op_array->last_brk_cont-1];
}

static void zend_update_extended_info(zend_op_array *op_array TSRMLS_DC)
{
	zend_op *opline = op_array->opcodes, *end=opline+op_array->last;

	while (opline<end) {
		if (opline->opcode == ZEND_EXT_STMT) {
			if (opline+1<end) {
				if ((opline+1)->opcode == ZEND_EXT_STMT) {
					opline->opcode = ZEND_NOP;
					opline++;
					continue;
				}
				if (opline+1<end) {
					opline->lineno = (opline+1)->lineno;
				}
			} else {
				opline->opcode = ZEND_NOP;
			}
		}
		opline++;
	}
}

static void zend_extension_op_array_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->op_array_handler) {
		extension->op_array_handler(op_array);
	}
}

ZEND_API int pass_two(zend_op_array *op_array TSRMLS_DC)
{
	zend_op *opline, *end;

	if (op_array->type!=ZEND_USER_FUNCTION && op_array->type!=ZEND_EVAL_CODE) {
		return 0;
	}
	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) {
		zend_update_extended_info(op_array TSRMLS_CC);
	}
	if (CG(compiler_options) & ZEND_COMPILE_HANDLE_OP_ARRAY) {
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_handler, op_array TSRMLS_CC);
	}

	if (!(op_array->fn_flags & ZEND_ACC_INTERACTIVE) && op_array->size != op_array->last) {
		op_array->opcodes = (zend_op *) erealloc(op_array->opcodes, sizeof(zend_op)*op_array->last);
		op_array->size = op_array->last;
	}

	opline = op_array->opcodes;
	end = opline + op_array->last;
	while (opline < end) {
		if (opline->op1.op_type == IS_CONST) {
			Z_SET_ISREF(opline->op1.u.constant);
			Z_SET_REFCOUNT(opline->op1.u.constant, 2); /* Make sure is_ref won't be reset */
		}
		if (opline->op2.op_type == IS_CONST) {
			Z_SET_ISREF(opline->op2.u.constant);
			Z_SET_REFCOUNT(opline->op2.u.constant, 2);
		}
		switch (opline->opcode) {
			case ZEND_GOTO:
				if (Z_TYPE(opline->op2.u.constant) != IS_LONG) {
					zend_resolve_goto_label(op_array, opline, 1 TSRMLS_CC);
				}
				/* break omitted intentionally */
			case ZEND_JMP:
				opline->op1.u.jmp_addr = &op_array->opcodes[opline->op1.u.opline_num];
				break;
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
				opline->op2.u.jmp_addr = &op_array->opcodes[opline->op2.u.opline_num];
				break;
		}
		ZEND_VM_SET_OPCODE_HANDLER(opline);
		opline++;
	}
	
	op_array->done_pass_two = 1;
	return 0;
}

int print_class(zend_class_entry *class_entry TSRMLS_DC)
{
	printf("Class %s:\n", class_entry->name);
	zend_hash_apply(&class_entry->function_table, (apply_func_t) pass_two TSRMLS_CC);
	printf("End of class %s.\n\n", class_entry->name);
	return 0;
}

ZEND_API unary_op_type get_unary_op(int opcode)
{
	switch (opcode) {
		case ZEND_BW_NOT:
			return (unary_op_type) bitwise_not_function;
			break;
		case ZEND_BOOL_NOT:
			return (unary_op_type) boolean_not_function;
			break;
		default:
			return (unary_op_type) NULL;
			break;
	}
}

ZEND_API binary_op_type get_binary_op(int opcode)
{
	switch (opcode) {
		case ZEND_ADD:
		case ZEND_ASSIGN_ADD:
			return (binary_op_type) add_function;
			break;
		case ZEND_SUB:
		case ZEND_ASSIGN_SUB:
			return (binary_op_type) sub_function;
			break;
		case ZEND_MUL:
		case ZEND_ASSIGN_MUL:
			return (binary_op_type) mul_function;
			break;
		case ZEND_DIV:
		case ZEND_ASSIGN_DIV:
			return (binary_op_type) div_function;
			break;
		case ZEND_MOD:
		case ZEND_ASSIGN_MOD:
			return (binary_op_type) mod_function;
			break;
		case ZEND_SL:
		case ZEND_ASSIGN_SL:
			return (binary_op_type) shift_left_function;
			break;
		case ZEND_SR:
		case ZEND_ASSIGN_SR:
			return (binary_op_type) shift_right_function;
			break;
		case ZEND_CONCAT:
		case ZEND_ASSIGN_CONCAT:
			return (binary_op_type) concat_function;
			break;
		case ZEND_IS_IDENTICAL:
			return (binary_op_type) is_identical_function;
			break;
		case ZEND_IS_NOT_IDENTICAL:
			return (binary_op_type) is_not_identical_function;
			break;
		case ZEND_IS_EQUAL:
			return (binary_op_type) is_equal_function;
			break;
		case ZEND_IS_NOT_EQUAL:
			return (binary_op_type) is_not_equal_function;
			break;
		case ZEND_IS_SMALLER:
			return (binary_op_type) is_smaller_function;
			break;
		case ZEND_IS_SMALLER_OR_EQUAL:
			return (binary_op_type) is_smaller_or_equal_function;
			break;
		case ZEND_BW_OR:
		case ZEND_ASSIGN_BW_OR:
			return (binary_op_type) bitwise_or_function;
			break;
		case ZEND_BW_AND:
		case ZEND_ASSIGN_BW_AND:
			return (binary_op_type) bitwise_and_function;
			break;
		case ZEND_BW_XOR:
		case ZEND_ASSIGN_BW_XOR:
			return (binary_op_type) bitwise_xor_function;
			break;
		case ZEND_BOOL_XOR:
			return (binary_op_type) boolean_xor_function;
			break;
		default:
			return (binary_op_type) NULL;
			break;
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
