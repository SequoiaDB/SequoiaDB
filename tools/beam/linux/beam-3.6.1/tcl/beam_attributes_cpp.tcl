# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2004-2010 IBM Corporation. All rights reserved.
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
#        Attributes that relate to C++.
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ----------------------------------------------------
#        04/13/04  fwalling Broken out from beam_c_attributes.tcl
#

namespace eval beam::attribute {

  ###################################################
  # Attributes for C++
  ###################################################
  
  set new_sig               {operator new(size_t)}
  set array_new_sig         {operator new [](size_t)}
  set new_nothrow_sig       {operator new (size_t, const std::nothrow_t &)}
  set array_new_nothrow_sig {operator new[] (size_t, const std::nothrow_t &)}
    
  set del_sig               {operator delete (void *)}
  set array_del_sig         {operator delete [](void *)}
  set del_nothrow_sig       {operator delete (void *, const std::nothrow_t &)}
  set array_del_nothrow_sig {operator delete[] (void *, const std::nothrow_t &)}


# The functions given attributes in this file have no other side-effects

 beam::function_attribute  no_other_side_effects  -signatures \
	$new_sig               \
        $array_new_sig         \
        $new_nothrow_sig       \
        $array_new_nothrow_sig \
        $del_sig               \
        $array_del_sig         \
        $del_nothrow_sig       \
        $array_del_nothrow_sig
        
  
  # Since older compilers didn't throw an exception for "new", but instead 
  # return NULL, we provide a command line option to change the behavior of 
  # "new" and "new[]". If that option was seen, we treat "new" like 
  # "new(nothrow)" to avoid complaints about unneeded tests for NULL.

  if { [::info exists beam::new_returns_null] && $beam::new_returns_null == "yes" } {
    beam::function_attribute $new_nothrow_like -signatures $new_sig
    beam::function_attribute $new_nothrow_like -signatures $array_new_sig
  } else {
    beam::function_attribute $new_like -signatures $new_sig
    beam::function_attribute $new_like -signatures $array_new_sig
  }
  
  beam::function_attribute $new_nothrow_like -signatures $new_nothrow_sig
  beam::function_attribute $new_nothrow_like -signatures $array_new_nothrow_sig
  
  beam::function_attribute $free_like -signatures $del_sig
  beam::function_attribute $free_like -signatures $del_nothrow_sig
  beam::function_attribute $free_like -signatures $array_del_sig
  beam::function_attribute $free_like -signatures $array_del_nothrow_sig

  beam::function_attribute {
      property (index = return,       # Which parameter has the property
                property_type = provides,                  
                type = output,                  
                property_name = "memory allocation source",
                property_value = "from new") 
  } -signatures $new_sig $new_nothrow_sig
    
  beam::function_attribute {
      property (index = 1,           # Which parameter has he property
                property_type = requires,                  
                type = input,                  
                property_name = "memory allocation source",
                property_value = "from new") 
  } -signatures $del_sig $del_nothrow_sig
    
  beam::function_attribute {
      property (index = return,      # Which parameter has he property
                property_type = provides,                  
                type = output,                  
                property_name = "memory allocation source",
                property_value = "from new array") 
  } -signatures $array_new_sig $array_new_nothrow_sig
    
  beam::function_attribute {
      property (index = 1,           # Which parameter has he property
                property_type = requires,                  
                type = input,                  
                property_name = "memory allocation source",
                property_value = "from new array")
  } -signatures $array_del_sig $array_del_nothrow_sig

  ################################################################
  # STL Properties
  ################################################################
  
  # STL String: The operators that take in 'const char *' may not be passed NULL
  # (C++ Standard, section 21.3.1)
  #
  # This covers:
  #  std::basic_string<>::basic_string(const char *)
  #  std::basic_string<>::basic_string(const char *, size_t)
  #  std::basic_string<>::operator=(const char *)

  beam::function_attribute {
    buffer (buffer_index = 1,
            type = read,
            string_index = 1),
  } -signatures \
  {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>::basic_string
                     (const char *, 
                      size_t, 
                      const std::allocator<char>&)
  } \
  {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>::basic_string
                     (const char *, 
                      const std::allocator<char>&)
  } \
  {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>::operator =(const char *)
  }

}
