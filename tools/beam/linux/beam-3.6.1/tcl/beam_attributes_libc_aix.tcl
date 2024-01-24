# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2005-2010 IBM Corporation. All rights reserved.
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
#        Attributes for non C-standard functions that are found in
#        libc on AIX.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        06/03/05  florian  Carved out of beam_attributes_libc.tcl
#

# This file contains attributes for the following functions.
# None of them have any other side effects.

 beam::function_attribute  no_other_side_effects  -signatures \
    "mlalloc"    \
    "clalloc"    \
    "cfree"      \
    "relalloc"   \
    "xmalloc"    \
    "xmfree"     \
    "__assert"   \
    "simple_lock"   \
    "simple_unlock" \



# Below are variable assigments to make the attributes more readable

  set require     {property ( property_type = requires, type = input, }
  set provide     {property ( property_type = provides, type = output, }
  set if          {if       ( type  = input,     }
  set and         {and      ( type  = input,     }
  set A           {beam::function_attribute}

  set initialized   {  index = 1,
                       property_name = "_lock_state",
                       property_value = "initialized")
  }

  set uninitialized {  index = 1,
                       property_name = "_lock_state",
                       property_value = "uninitialized")
  }

# Return value is 0
  set return_0    {(index = return,
                    type = output,
                    test_type = equal,
                    test_value = 0)
  }

# Return value is not 0
  set return_non_0 {(index = return,
                     type = output,
                     test_type = not_equal,
                     test_value = 0)
  }

# Lock in exclusive mode
  set lock_excls   { index = 1,
		     property_name = "_lock_num_exclusive",
                     increment = 1)
  }

# Unlock in exclusive mode
  set unlock_excls { index = 1,
		     property_name = "_lock_num_exclusive",
                     increment = -1)
  }

# Locked in exclusive mode
  set locked_excls { index = 1,
		     property_name = "_lock_num_exclusive",
                     test_type = greater_than,
                     test_value = 0)
  }

# Unlocked in exclusive mode
  set unlocked_excls { index = 1,
                       property_name = "_lock_num_exclusive",
                       test_type = equal,
                       test_value = 0)
  }



namespace eval beam::attribute {

  beam::function_attribute $malloc_like  -names "mlalloc"
  beam::function_attribute $calloc_like  -names "clalloc"
  beam::function_attribute $free_like    -names "cfree"
  beam::function_attribute $realloc_like -names "relalloc"
  beam::function_attribute $malloc_like  -names "xmalloc"
  beam::function_attribute $free_like    -names "xmfree"

  beam::function_attribute $abort_like   -names "__assert"

# xmalloc and xmfree are a paired allocator/deallocator

  beam::function_attribute {
    property (index = return,       # Which parameter has he property
	      property_type = provides,
	      type = output,
              property_name = "memory allocation source",
	      property_value = "from xmalloc")
  } -signatures "xmalloc"

  beam::function_attribute {
    property (index = 1,            # Which parameter has he property
	      property_type = requires,
	      type = input,
	      property_name = "memory allocation source",
	      property_value = "from xmalloc")
  } -signatures "xmfree"

# allocators and matching deallocators

  beam::function_attribute {
      property (index = return,      # Which parameter has he property
                property_type = provides,
                type = output,
                property_name = "memory allocation source",
                property_value = "from mlalloc")
  } -signatures "mlalloc" "clalloc" "relalloc"

  beam::function_attribute {
      property (index = 1,           # Which parameter has he property
                property_type = requires,
                type = input,
                property_name = "memory allocation source",
                property_value = "from mlalloc")
  } -signatures "relalloc"


    "$A" "$provide $initialized"    -names "lock_init"

# The lock address referenced by the arg is locked by calling simple_lock. If
# the lock is already locked, the calling thread blocks until the lock becomes
# available. This operation returns with the lock referenced by the argument in
# the locked state with the calling thread as its owner.

    "$A" "$provide $lock_excls"     -names "simple_lock"

    "$A" "$require $locked_excls"   -names "simple_unlock"
    "$A" "$provide $unlock_excls"   -names "simple_unlock"


# Functions locking and unlocking may be run in parallel 
# (It may be obvious, but need to be specified)

  "$A" "thread_safe"  -names "simple_lock" \
                             "simple_unlock" 

}
