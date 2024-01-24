# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2006-2010 IBM Corporation. All rights reserved.
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
#        Daniel Brand
#
#    DESCRIPTION:
#
#        Attributes for pthread functions
#
#    MODIFICATIONS:
#
#        See svn log for more recent modifications.
#


# Below are variable assigments to make the attributes more readable

  set require     {property ( property_type = requires, type  = input,  }
  set provide     {property ( property_type = provides, type  = output,  }
  set if          {if       ( type  = input,     }
  set and         {and      ( type  = input,     }


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

# The pthread_mutex_init() function initialises the mutex whose address is parameter 1.
# Upon successful initialisation, the state of the mutex becomes initialised and unlocked.
# Attempting to initialise an already initialised mutex results in undefined behaviour.

    beam::function_attribute "$require $uninitialized"                  -names "pthread_mutex_init"
    beam::function_attribute "$provide $initialized   if $return_0"     -names "pthread_mutex_init"
    beam::function_attribute "$provide $uninitialized if $return_non_0" -names "pthread_mutex_init"
    beam::function_attribute "$provide $unlocked_excls"                 -names "pthread_mutex_init"

# Programmers cannot rely on pthread_mutex_init() always succeeding because that success is
# system state dependent.
# FIXME: should we provide the force_test attribute ?
    beam::function_attribute {
        force_test ( test_index = return,
                     test_type = not_equal,
                     value = 0 ),

    } -names "pthread_mutex_init"


# The pthread_mutex_destroy() function destroys the mutex whose address is parameter 1;
# the mutex object becomes, in effect, uninitialised.
# A destroyed mutex object can be re-initialised using pthread_mutex_init();
# the results of otherwise referencing the object after it has been destroyed are undefined.

# It is safe to destroy an initialised mutex that is unlocked.
# Attempting to destroy a locked mutex results in undefined behaviour.

    beam::function_attribute "$require $initialized"    -names "pthread_mutex_destroy"
    beam::function_attribute "$provide $uninitialized"  -names "pthread_mutex_destroy"


# The pthread_mutex_lock, pthread_mutex_trylock and pthread_mutex_unlock functions
# will fail if:
#The value specified by mutex does not refer to an initialised mutex object.

    beam::function_attribute "$require $initialized"    -names "pthread_mutex_lock"
    beam::function_attribute "$require $initialized"    -names "pthread_mutex_trylock"
    beam::function_attribute "$require $initialized"    -names "pthread_mutex_unlock"

# The mutex object referenced by mutex is locked by calling pthread_mutex_lock.
# If the mutex is already locked, the calling thread blocks until the mutex
# becomes available.
# This operation returns with the mutex object referenced by mutex in
# the locked state with the calling thread as its owner.

    beam::function_attribute "$provide $lock_excls"     -names "pthread_mutex_lock"

# The pthread_mutex_unlock() function attempts to unlock the specified mutex.

    beam::function_attribute "$provide $unlock_excls"   -names "pthread_mutex_unlock"

# Attempting to destroy a locked mutex results in undefined behavior.

    beam::function_attribute "$require $unlocked_excls" -names "pthread_mutex_destroy"


# The functions do not do anything else
# FIXME: add pthread_mutex_trylock when all its effects are expressed

    beam::function_attribute "no_other_side_effects"  -names "pthread_mutex_init"
    beam::function_attribute "no_other_side_effects"  -names "pthread_mutex_destroy"
    beam::function_attribute "no_other_side_effects"  -names "pthread_mutex_lock"
    beam::function_attribute "no_other_side_effects"  -names "pthread_mutex_unlock"


# The pthread_exit() function terminates the calling thread

  beam::function_attribute "noreturn"  -names "pthread_exit"

# Functions locking and unlocking mutexes may be run in parallel 
# (It may be obvious, but need to be specified)

 beam::function_attribute "thread_safe"  -names "pthread_mutex_lock" \
                                                "pthread_mutex_unlock" \
                                                "pthread_mutex_trylock" 
}


