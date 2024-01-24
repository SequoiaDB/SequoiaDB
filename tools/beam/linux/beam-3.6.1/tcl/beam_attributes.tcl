# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2003-2010 IBM Corporation. All rights reserved.
#
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp
#
# The source code for this program is not published or otherwise divested
# of its trade secrets, irrespective of what has been deposited within
# the U.S. Copyright Office.
#
#
#    AUTHOR:
#
#        Frank Wallingford
#
#    DESCRIPTION:
#
#        This Tcl file defines function properties but does NOT associate
#        them with any specific function. It only provides the raw materials.
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        12/17/03  fwalling Switched to lang-specific files
#        06/09/03  brand    Added longjmp
#        04/22/03  brand    Added allocators, etc.
#        04/21/03  fwalling Created
#

namespace eval beam::attribute {


# This is just an example of all available atributes


  beam::resource_create {               # resource representing heap allocated memory
    name = "heap_memory",               # identified as "heap_memory" to attributes
    display = "memory",                 # in complaints call it "memory"
    allocating_verb = "allocating",     # use these verbs when talking about memory
    allocated_verb = "allocated",
    freeing_verb = "freeing",
    freed_verb = "freed",
    use_after_free = "error"
  }

  beam::resource_create {               # resource representing memory on the stack
    name = "stack_memory",              # identified as "stack_memory" to attributes
    display = "stack memory",           # in complaints call it "stack memory"
    allocating_verb = "allocating",     # use these verbs when talking about memory
    allocated_verb = "allocated",
    freeing_verb = "deallocating",
    freed_verb = "deallocated",
    use_after_free = "error"
  }

  beam::resource_create {
    name = "file",
    display = "file",
    allocating_verb = "opening",
    allocated_verb = "opened",
    freeing_verb = "closing",
    freed_verb = "closed",
    use_after_free = "error"
  }

  beam::resource_create {
    name = "directory",
    display = "directory",
    allocating_verb = "opening",
    allocated_verb = "opened",
    freeing_verb = "closing",
    freed_verb = "closed",
    use_after_free = "error"
  }

  beam::propinfo_create {               
      name = "memory allocation source", # property saying how a location was allocated
      invariance = "none",               # only address at byte 0 has the property
      dependence = "none"                # nothing can change the property of an address
  }
    

  beam::propinfo_create {               
      name = "file source", # property saying how a file was opened
      invariance = "none",  # the address returned by fopen-like has the property
      dependence = "none"   # nothing can change the property of a file
  }


  beam::propinfo_create {               
      name = "file state",  # opened or closed
      invariance = "none",  # only the number returned by open() has the property
      dependence = "calls", # only some calls can change the state of a file descriptor
      resource   = "file"
  }

  beam::propinfo_create {               
      name = "directory state", # opened or closed
      invariance = "none",      # only the dir handle returned by opendir() has the property
      dependence = "calls",     # only some calls can change the state of a dir handle
      resource   = "directory"
  }
  


# Builtin understanding of locking

# This is the resource that would leak if a lock
# was initialized, but not uninitialized

    beam::resource_create {
        name            = "lock",
        display         = "lock",
        allocating_verb = "initializing",
        allocated_verb  = "initialized",
        freeing_verb    = "uninitializing",
        freed_verb      = "uninitialized",
        use_after_free  = "error"
    }

# This is the resource that would leak if a lock
# was acquired, but not released.
# Its name starts with "_" meaning that it is a 
# built-in name interpreted by Beam
# At this time we have only recursive locks

    beam::resource_create {
        name            = "_recursive_lock",
        display         = "lock",
        allocating_verb = "acquiring",
        allocated_verb  = "acquired",
        freeing_verb    = "releasing",
        freed_verb      = "released",
        use_after_free  = "ok"
    }

    beam::propinfo_create {
        name       = "_lock_state",
        invariance = "none",
        dependence = "calls",
        resource   = "lock",
        domain     = "symbol"   # "initialized", "uninitialized"
    }

    beam::propinfo_create {
        name       = "_lock_num_exclusive",
        invariance = "none",
        dependence = "calls",
        resource   = "_recursive_lock",
        domain     = "int"      # by how many threads locked for writing
    }

    beam::propinfo_create {
        name       = "_lock_num_shared",
        invariance = "none",
        dependence = "calls",
        resource   = "_recursive_lock",
        domain     = "int"      # by how many threads locked for reading
    }


  set all_attributes {
    pure,                                         # no side-effect
    const,                                        # no side-effect and return value for fixed arguments is always the same
    thread_safe,                                  # may run in parallel with any other
    unused,
    no_other_side_effects,                        # no effect other than what specified
    no_other_anchoring,                           # no anchoring other than what specified
    no_stats,                                     # don't include in stats at all, even in total count

    noreturn,                                     # function does not return
    format ( kind = printf,                       # printf, or scanf 
             string_index = 1,                    # where pattern
             first_to_check = 2 ),                # where arguments start
    aligned ( alignment = 4 ),
    allocator ( size_index = 1,                   # parameter specifying size to allocate
                multiplier_index = unset,         # size multiplied by this
		return_index = return,            # where is allocated memory returned 
		anchored,                         # if not freed will it leak?
		if_out_of_memory = return_null,   # if runs out of mem returns null
		initial_state = uninitialized,    # how is allocated memory initialized?
		if_size_is_0 = error,             # what is alsed to allocate 0 bytes?
		if_size_is_negative = error,      # what is alsed to allocate negative number 
                resource = "heap_memory"),        # what words to use in complaints
    return_overlap ( return_index = return,       # parameter where pointer is returned (0 = return value)
                     points_into_index = 1, ),    # parameter into which returned pointer points
                     fate = may) 
                     
    
    # Chained attributes - any can contain conditions of this form:

    chained_attribute (
      keywords...
    ) if (
      index = 1,                                  # The index of the condition
      num_dereference = 0,                        # dereferenced this many times
      type = output,                              # condition applies to input or output parameter
      
      range_min = 0,                              # condition is true if value at index is
      range_max = 10,                             # within this range
      
      test_type = equal,                          # condition is true if value compares to test_value
      test_value = 0,                             # according to the test_type (equal, not_equal, etc)
      
      increment = 2,                              # The call increments the parameter by 2
      
      property_name = "name",                     # condition is true if value contains this
      property_value = "value"                    # property name and value
    ) and (
      ...                                         # any number of "and (...)" may follow, with same keywords
    ),
    
    
    assert if ( index = 1,                        # issue no complaint after called with 
             type = input,                        # input value of index
             num_dereference = 0,                 # dereferenced this many times, in a range:
             range_min = 0,                       # Range [range_min, range_max]
             range_max = 1,                       # (Not used if invalid range)
             test_type = unset,                   # Comparison ( equal, not_equal, etc )
             test_value = 0 ),
             
    deallocator ( pointer_index = 1,              # parameter with pointer to be deallocated
                  num_dereference = 1,            # How many times is the parameter dereferenced?
                  allow_null,                     # The pointer may be null
                  resource = "heap_memory"),      # refer to it as freeing memory

### (possible future attribute)
#    forbid_overlap ( index1 = 1,                  # one of two parameters that may never overlap in memory
#                     index2 = 2 ),                # the other of two parameters that may never overlap in memory

    buffer ( buffer_index = 1,                    # Which parameter has the buffer that is read/written
             num_dereference = 0,                 # How many times is the parameter dereferenced?
             type = read,                         # read or write?

             string_index = 2,                    # Optional: a parameter whose string length adds to "given_string_length"
	     string2_index = unset,               # Optional: a second string parameter like the above
	     string3_index = unset,               # Optional: a third string parameter like the above
	     padding = 0 ,                        # Optional: a number to always add to  "given_string_length"
	     size_index = 4,                      # Optional: the index of a size for specifing "given_size_length"
	     multiplier_index = 0,                # Optional: the index of a number to multiply by size

             # Equivalent to the above, except that the size of all
             # of the bound strings + bound padding is bound by
             # the value of bound_size * bound_multiplier

             bound_string_index = 2,
	     bound_string2_index = unset,
	     bound_string3_index = unset,
	     bound_padding = 0 ,
	     bound_size_index = 4,
	     bound_multiplier_index = 0 )     # Bytes accessed = min(given_string_length, given_size_length)
             
# (memmod is an internal attribute)

    memmod ( global_variable = "foo",             # A global variable is modified by the function
             index           = 2,                 # A parameter is modified by the function
	     allow_null,                          # the arg may be be null
             num_dereference = 1,                 # 0 or 1 for now, level of pointer dereference
	     first_bit       = 8,                 # First bit modified (or -1)
	     num_bits        = 32 ),              # Number of bits modified (or -1)


    force_test ( test_index = return,             # Which parameter is the value returned in?
                 num_dereference = 0,             # How many times is the parameter dereferenced?
                 # then, one of:
                 min_test_value = 0,              # Test required if in range [min_test_value, max_test_value]
                 max_test_value = 0,              # (Not used if invalid range)
                 # OR
                 test_type = unset,               # What test is required? (not_equal, greater_than, greater_than_or_equal, less_than, less_than_or_equal)
		 value = 0 ),                     # What is the value to compare to?
      
      property (index = 1,                        # Which parameter has the property
                num_dereference = 0,              # How many time to dereference the parameter
                property_type = requires,         # Function requires the property
                type = input,                     # The property is on input value of parameter
                range_min = 0,                    # The property can be a numerical range
                range_max = 1,                    # just like conditions
                test_type = equal,                # or it can be a test
                test_value = -1,                  # and value to test against
                property_name = "memory allocation source",     # or the original name of a text property
                property_value = "from malloc"),                # and the vaue of the text property
                
      advisory ( explanation = "Explanation text", # This text is printed when the function is called if
                 category    = "information" ),    # this category is in the enabling_policy
                 
      vararg ( count_index = 2,        # Optional index where to find the number of expected arguments
               count_multiplier = 3,   # Optional multiplier to multiply by the number at count_index
               count_always = 0,       # Optional amount to always add to the total number of expected arguments
               first_to_check = 4 ),   # The first argument to start the required argument count at
                 
  }



#     printf_like

# check validity of pattern.
# pattern is in arg 1 and the rest follows

  set printf_like {
    format ( kind = printf, string_index = 1, first_to_check = 2 )
  }


#     fprintf_like

# check validity of pattern.
# pattern is in arg 2 and the rest follows

  set fprintf_like {
    format ( kind = printf, string_index = 2, first_to_check = 3 )
  }


#     scanf_like

# check validity of pattern.
# pattern is in arg 1 and the rest follows

  set scanf_like {
    format ( kind = scanf, string_index = 1, first_to_check = 2 )
  }
  
#     fscanf_like

# check validity of pattern.
# pattern is in arg 2 and the rest follows

  set fscanf_like {
    format ( kind = scanf, string_index = 2, first_to_check = 3 )
  }


  set can_return_null_like {
      force_test ( test_index = return,             # The return value can be null
		   num_dereference = 0,             # The return value itself can be null
		   test_type = equal,               # The exception case is when equal to
		   value = 0 )                      # NULL
  }




#     malloc_like

# Allocator behaving like malloc

  
  set malloc_like {
      allocator ( size_index = 1,              # size is in parm 1
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = uninitialized, # new memory is uninitialized
		if_size_is_0 = return_null,    # if size = 0, get null
		if_size_is_negative = error ,  # never want to ask for negative size
                resource = "heap_memory"),
    no_other_side_effects                      # does nothing else besides allocation
  }
    
#     alloca_like

  # We do not handle alloca exactly;
  # specifically we do not free the results of alloca
  # at the end of a function.
  # It is represented as a new anchored memory,
  
  set alloca_like {
      allocator ( size_index = 1,              # size is in parm 1
		return_index = return,         # allocated memory is returned
		anchored,                      # caller need not free it
		if_out_of_memory = ok,         # doesn't run out of mem
		initial_state = uninitialized, # new memory is uninitialized
		if_size_is_0 = ok,             # if size = 0, returns memory of size 0
		if_size_is_negative = error ,  # never want to ask for negative size
                resource = "stack_memory"),
      # the next property will catch attemps at freeing it
      property (index = return,                   # Which parameter has the property
                num_dereference = 0,              # How many time to dereference the parameter
                property_type = provides,                  
                type = output,                  
                property_name = "memory allocation source", 
                property_value = "from alloca"), 
    no_other_side_effects                      # does nothing else besides allocation
  }

#     new_like

# Allocator behaving like new
# Differs from malloc in that it aborts in case of not enough memory, and
# if size = 0 does not retun NULL, retuns a pointer to memory of size 0

  set new_like {
      allocator ( size_index = 1,              # size is in parm 1
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = ok,         # if runs out of mem then do not return
		initial_state = uninitialized, # new memory is uninitialized
		if_size_is_0 = ok,             # if size = 0, returns memory of size 0
		if_size_is_negative = error ,  # never want to ask for negative size
                resource = "heap_memory"),     # refer to it as "allocation memory"
    no_other_side_effects                      # does nothing else besides allocation
  }
    

#     new_nothrow_like

# Allocator behaving like new (nothrow)
# Differs from new in that it returns NULL in case of not enough memory

  set new_nothrow_like {
      allocator ( size_index = 1,              # size is in parm 1
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = uninitialized, # new memory is uninitialized
		if_size_is_0 = ok,             # if size = 0, returns memory of size 0
		if_size_is_negative = error ,  # never want to ask for negative size
                resource = "heap_memory"),     # refer to it as "allocation memory"
    no_other_side_effects                      # does nothing else besides allocation
  }
    

#     calloc_like

# Allocator behaving like calloc

  set calloc_like {
      allocator ( size_index = 2,              # element size is in parm 2
                multiplier_index = 1,          # number of elements is in parm 1
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_zero, # new memory is all 0
		if_size_is_0 = return_null,    # if size = 0, get null
		if_size_is_negative = error ,  # never want to ask for negative size
                resource = "heap_memory"),          # refer to it as "allocation memory"
    no_other_side_effects                      # does nothing else besides allocation
  }
  

#     data_base_allocator_like

# Allocator behaving like a typical function allocation
# a new object in a data-base package
# The allocated memory is anchored inside the data-base package
# so that it need not be freed

  beam::resource_create {
    name = "object",
    display = "object",
    allocating_verb = "creating",
    allocated_verb = "created",
    freeing_verb = "deleting",
    freed_verb = "deleted",
    use_after_free = "error"
  }
  
  set data_base_allocator_like {
      allocator ( 
                size_index = unset,            # size unknown
		return_index = return,         # allocated memory is returned
		anchored,                      # pointer to new memory save in static storage
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, # initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error,  # irrelevant 
                resource = "object"            # refer to it as "creating object"
                  ),
    no_other_side_effects                     # does nothing else besides allocation
  }
  
  
#     unknown_allocator_like

# A function returning a something that needs to be freed
# The contents of the memory is unknown

  set unknown_allocator_like {
      allocator (
                size_index = unset,            # size unknown
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, # initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error   # irrelevant
                resource = "heap_memory"       # refer to it as "allocating memory"
                  ),
    no_other_side_effects                      # does nothing else besides allocation
  }
  
  
#     fopen_like

# Allocator behaving fopen


  set fopen_like {
      allocator (
                size_index = unset,            # size is irrelevant
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# there are more important reason for null
		initial_state = initialized_to_unknown, # initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error,  # irrelevant
                resource = "file"              # refer to it as "opening file"
                  )  
  }
    
#     cursor_allocator_parm1

# A function (typically in a data-base package)
# returns a new memory through parameter 1, like this
#    A = first_object(&Cursor);
# Most common occurrence of such functions are allocators 
# of cursors for iteration

  beam::resource_create {
    name = "cursor",
    display = "cursor",
    allocating_verb = "creating",
    allocated_verb = "created",
    freeing_verb = "deleting",
    freed_verb = "deleted",
    use_after_free = "error"
  }
  
  set cursor_allocator_parm1 {
      allocator ( 
                size_index = unset,            # size unknown
		return_index = 1,              # allocated memory is returned through parm 1
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, #  initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error   # irrelevant
                resource = "cursor"            # refer to it as "creating cursor"
                  )
  }
  
  
#     cursor_allocator_parm2

# A function (typically in a data-base package)
# returns a new memory through parameter 2, like this
#    A = first_object(Object, &Cursor);
# Most common occurrence of such functions are allocators 
# of cursors for iteration

  set cursor_allocator_parm2 {
      allocator ( 
                size_index = unset,            # size unknown
		return_index = 2,              # allocated memory is returned through parm 2
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, # initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error   # irrelevant
                resource = "cursor"            # refer to it as "creating cursor"
                  )
  }
  
  
#     cursor_allocator_parm3

# A function (typically in a data-base package)
# returns a new memory through parameter 3, like this
#    A = first_object(Object, Object, &Cursor);
# Most common occurrence of such functions are allocators 
# of cursors for iteration

  set cursor_allocator_parm3 {
      allocator ( 
                size_index = unset,            # size unknown
		return_index = 3,              # allocated memory is returned through parm 3
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, # initialized to something
		#if_size_is_0 = return_null,   # irrelevant
		#if_size_is_negative = error   # irrelevant
                resource = "cursor"            # refer to it as "creating cursor"
                  )
  }
  
  
#     realloc_like

# Deallocate first parameter and 
# return newly allocated memory

  set realloc_like {
      deallocator ( pointer_index = 1 ,        # deallocate parm 1
                  resource = "heap_memory")    # refer to it as freeing memory
      if (index = return,                      # Only if returned value non null
          type = output,
          test_type = not_equal,
          test_value = 0),           
    
      allocator ( size_index = 2,              # size in parm 2
		return_index = return,         # allocated memory is returned
		#anchored,                     # caller gets the only pointer to new memory
		if_out_of_memory = return_null,# if runs out of mem returns null
		initial_state = initialized_to_unknown, # partially initialized
		if_size_is_0 = return_null,
		if_size_is_negative = error ,
                resource = "heap_memory"       # refer to it as "allocating memory"
                ),
      anchor ( index = 1,                      # input may be returned
               fate = may),
    no_other_side_effects                      # does nothing else besides reallocation
  }
  
  
  
#     free_like

# Deallocate first parameter

  set free_like {
    deallocator ( pointer_index = 1 ,        # deallocate parm 1
                  resource = "heap_memory"), # refer to it as freeing memory
    no_other_side_effects                    # does nothing else besides deallocation
  }
  
  
#     fclose_like

# close first parameter

  set fclose_like {
    deallocator ( pointer_index = 1 ,       # deallocate parm 1
                  resource = "file"),       # refer to it as closing file
    no_other_side_effects                   # does nothing else besides deallocation
  }
  
  
  
#     exit_like

# Terminate execution

  set exit_like {
    noreturn,
    no_other_side_effects
  }
  
  
  
  
#     abort_like

# Terminates execution and it is an error to do so
  
  set abort_like {
    noreturn,
    assert,  #unconditional assert
    no_other_side_effects
  }
  
  
  
#     panic_like

# Prints some nasty message so no complaints
# should be issued following this

  set  panic_like {
    assert,  #unconditional assert
    no_other_side_effects
  }
  


#     strstr_overlap_like

# Returns a pointer into its first argument

  set strstr_overlap_like {
      return_overlap ( return_index = return,       # it comes out as return value
                       points_into_index = 1 )     # it points into the same location as arg 1
  }   

#     forbid_overlap_like

# The first two arguments may not point to memory that overlaps

### (possible future attribute)
#  set forbid_overlap_like {
#      forbid_overlap ( index1 = 1,
#                       index2 = 2 )
#  }


#     buffer1_size2_write_like

# A buffer in parameter 1 requires a size that's in parameter 2

  set buffer1_size2_write_like {
      buffer ( buffer_index = 1, type = write, size_index = 2 )
  }


#     buffer1_size3_read_like

# A buffer in parameter 1 requires a size that's in parameter 3

  set buffer1_size3_read_like {
      buffer ( buffer_index = 1, type = read, size_index = 3 )
  }

#     buffer1_size3_write_like

# A buffer in parameter 1 requires a size that's in parameter 3

  set buffer1_size3_write_like {
      buffer ( buffer_index = 1, type = write, size_index = 3 )
  }



#     buffer2_size3_read_like

# A buffer in parameter 2 requires a size that's in parameter 3

  set buffer2_size3_read_like {
      buffer ( buffer_index = 2, type = read, size_index = 3 )
  }

#     buffer2_size3_write_like

# A buffer in parameter 2 requires a size that's in parameter 3

  set buffer2_size3_write_like {
      buffer ( buffer_index = 2, type = write, size_index = 3 )
  }

#     buffer1_string_read_like

# Parameter 1 is a string being read

  set buffer1_string_read_like {
      buffer ( buffer_index = 1, type = read, string_index = 1)
  }


   set side_effect_on_environment {
      memmod ( global_variable = __BEAM_dummy_global_variable_representing_environment,
	       first_bit       = 0,                 # assigns buffer from the beginning
	       num_bits        = -1 )              # assigns the whole buffer
  } 

   set side_effect_on_some_accessible_variables {
      memmod ( global_variable = __BEAM_dummy_global_variable_representing_environment,
	       num_dereference = 1,                 # assigns to some memory pointed at
	       first_bit       = 0,                 # assigns buffer from the beginning
	       num_bits        = -1 ),              # assigns the whole buffer
      no_other_side_effects
  } 
  
  set return_non_0_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
                 test_type = not_equal, 
                 test_value = 0)
  }
  
  set return_int_ge_0_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
		 range_min = 0,
		 range_max = 2147483647)
  }

  set return_short_ge_0_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
		 range_min = 0,
		 range_max = 32767)
  }
  
  set return_ge_0_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
                 test_type = greater_than_or_equal, 
                 test_value = 0)
  }

  set return_gt_0_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
                 test_type = greater_than, 
                 test_value = 0)
  }
  
  set return_ge_minus_1_like { 
      property ( index = return, 
                 property_type = provides, 
                 type = output, 
                 test_type = greater_than_or_equal, 
                 test_value = -1)
  }
  
  set requires_arg_1_non_null_like { 
      property ( index = 1, 
                 property_type = requires, 
                 type = input, 
                 test_type = not_equal, 
                 test_value = 0)
  }
  
  set requires_arg_2_non_null_like { 
      property ( index = 2, 
                 property_type = requires, 
                 type = input, 
                 test_type = not_equal, 
                 test_value = 0)
  }
  
  set requires_arg_3_non_null_like { 
      property ( index = 3, 
                 property_type = requires, 
                 type = input, 
                 test_type = not_equal, 
                 test_value = 0)
  }

  set requires_arg_4_non_null_like { 
      property ( index = 4, 
                 property_type = requires, 
                 type = input, 
                 test_type = not_equal, 
                 test_value = 0)
  }

  set requires_arg_5_non_null_like { 
      property ( index = 5, 
                 property_type = requires, 
                 type = input, 
                 test_type = not_equal, 
                 test_value = 0)
  }

  # Writes to arg 1 which is required to be non-null
  set writes_non_null_arg_1 {
    memmod ( index = 1, first_bit = -1, num_bits = -1 )
  }

  # Writes to arg 2 which is required to be non-null
  set writes_non_null_arg_2 {
    memmod ( index = 2, first_bit = -1, num_bits = -1 )
  }

  # Writes to arg 3 which is required to be non-null
  set writes_non_null_arg_3 {
    memmod ( index = 3, first_bit = -1, num_bits = -1 )
  }

}
