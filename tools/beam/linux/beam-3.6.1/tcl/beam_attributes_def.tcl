# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2007-2010 IBM Corporation. All rights reserved.
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
#        Frank Wallingford, Florian Krohm, Nate Daly, Dan Brand
#
#        
#    DESCRIPTION:
#
#        Definition of function attributes. These are used for modelling
#        libc functions. 
#
#    NOTE:
#
#        There are several places where a function has the
#        requires_arg_1_non_null_like  attribute. But it should better
#        have a buffer attribute there. The reason is backward compatibility.
#        Buffer attribute cause different complaints. So we have an issue
#        in case people have suppressed a complaint with an innocence code.
#        This will be fixed when we roll out Marcio's stuff in which case
#        innocence codes will be broken again.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        11/26/07  florian  Carved out of beam_attributes_libc.tcl
#

namespace eval beam::attribute {

#####################################################################
# Convenience definitions
#####################################################################
  set returns_malloc_like {
    property ( index = return,                 
               property_type = provides,                  
               type = output,                  
               property_name = "memory allocation source", 
               property_value = "from malloc" )
  }

  set requires_malloc_like_arg_1 {
    property ( index = 1,
               property_type = requires,                  
               type = input,                  
               property_name = "memory allocation source",
               property_value = "from malloc" )
  }
              
  set returns_static_like {
    property ( index = return,                 
               property_type = provides,                  
               type = output,                  
               property_name = "memory allocation source", 
               property_value = "statically allocated" )
  }


#####################################################################
# A function that is "const" and nothing else.
#####################################################################
  set attr(const_function) "const, no_other_side_effects"

#####################################################################
# A function that is "pure" and nothing else.
#####################################################################
  set attr(pure_function)  "pure, no_other_side_effects"

#####################################################################
# frexp and friends
#####################################################################
  set attr(frexp) "buffer ( buffer_index = 2,
                            type = write,        
                            units = elements,
                            size = 1 ),
                            no_other_side_effects"

#####################################################################
# modf and friends
#####################################################################
  set attr(modf) "buffer ( buffer_index = 2,
                            type = write,        
                            units = elements,
                            size = 1 ),
                            no_other_side_effects"

#####################################################################
# remquo and friends
#####################################################################
  set attr(remquo) "buffer ( buffer_index = 3,
                             type = write,        
                             units = elements,
                             size = 1 ),
                             no_other_side_effects"

#####################################################################
# nan and friends
#####################################################################
  set attr(nan) "pure,
                 buffer ( buffer_index = 1,
                          string_index = 1,
                          type = read,
                          padding = 1 ),
                 no_other_side_effects"

#####################################################################
# feclearexcept
#####################################################################
  set attr(feclearexcept) "$side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# imaxabs
#####################################################################
  set attr(imaxabs) "const,
                     $return_ge_0_like,
                     no_other_side_effects"

#####################################################################
# setlocale
#####################################################################
  set attr(setlocale) "$side_effect_on_environment,
                       $returns_static_like,
                       no_other_side_effects"

#####################################################################
# localeconv
#####################################################################
  set attr(localeconv)  "pure, 
                         $returns_static_like,
                         $return_non_0_like,
                         no_other_side_effects" 

#####################################################################
# setjmp
#####################################################################
  set attr(setjmp) "$side_effect_on_environment,
                    assert if (index=return,        # to suppress any
                               type=output,         # complaint after
                               test_type=not_equal, # targetted  by a
                               test_value=0),       # longjmp
                    no_other_side_effects"

#####################################################################
# longjmp
#####################################################################
  set attr(longjmp) "$exit_like, no_other_side_effects"

#####################################################################
# signal
#####################################################################
  set attr(signal) "$side_effect_on_environment,
                    anchor ( index = 2 ),
                    no_other_side_effects"

#####################################################################
# raise
# We assume that the signal handler returns.
#####################################################################
  set attr(raise) "$side_effect_on_environment,
                   no_other_side_effects"

#####################################################################
# fread
#####################################################################
  set attr(fread) "$side_effect_on_environment,
                   buffer ( buffer_index = 1, 
                            type = write, 
                            size_index = 2, 
                            multiplier_index = 3 ),
                   buffer ( buffer_index = 4,
                            type  = read,
                            size  = 1,
                            units = elements ),
                   memmod ( index     =  4,
                            first_bit =  0,
                            num_bits  = -1 ),
                   no_other_side_effects" 

#####################################################################
# fwrite
#####################################################################
  set attr(fwrite) "$side_effect_on_environment,
                    buffer ( buffer_index = 1, 
                             type = read, 
                             size_index = 2, 
                             multiplier_index = 3 ),
                    buffer ( buffer_index = 4,
                             type = read,
                             size = 1,
                             units = elements ),
                    memmod ( index     =  4,
                             first_bit =  0,
                             num_bits  = -1 ),
                    no_other_side_effects" 

#####################################################################
# fgetpos
#####################################################################
  set attr(fgetpos)        "memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            $requires_arg_2_non_null_like,
                            no_other_side_effects" 

#####################################################################
# fseek
#####################################################################
  set attr(fseek) "memmod ( index     =  1,
                            first_bit =  0,
                            num_bits  = -1 ),
                   $side_effect_on_environment,
                   no_other_side_effects"

#####################################################################
# rewind
#####################################################################
  set attr(rewind) "memmod ( index     =  1,
                             first_bit =  0,
                             num_bits  = -1 ),
                    $side_effect_on_environment,
                    no_other_side_effects"

#####################################################################
# fsetpos
#####################################################################
  set attr(fsetpos) "memmod ( index     =  1,
                             first_bit =  0,
                             num_bits  = -1 ),
                     $requires_arg_2_non_null_like,
                     $side_effect_on_environment,
                     no_other_side_effects" 

#####################################################################
# ftell
#####################################################################
  set attr(ftell) "memmod ( index     =  1,
                            first_bit =  0,
                            num_bits  = -1 ),
                   no_other_side_effects" 

#####################################################################
# clearerr
#####################################################################
  set attr(clearerr) "memmod ( index     =  1,
                               first_bit =  0,
                               num_bits  = -1 ),
                      $side_effect_on_environment,
                      no_other_side_effects"

#####################################################################
# feof
#####################################################################
  set attr(feof) "$requires_arg_1_non_null_like,
                  pure,
                  no_other_side_effects"

#####################################################################
# perror
#####################################################################
  set attr(perror) "$side_effect_on_environment,
                    buffer ( buffer_index = 1,
                             type = read,
                             string_index = 1,
                             padding = 1 )
                        if ( index = 1,
                             type  = input,
                             test_type = not_equal,
                             test_value = 0 ),
                    no_other_side_effects"

#####################################################################
# remove
#####################################################################
  set attr(remove) "$requires_arg_1_non_null_like,
                    $side_effect_on_environment,
                    no_other_side_effects"

  set attr(rename) "$requires_arg_1_non_null_like,
                    $requires_arg_2_non_null_like,
                    $side_effect_on_environment,
                    no_other_side_effects"

  set attr(tmpfile) "$fopen_like, 
                     $side_effect_on_environment,
                     no_other_side_effects"

#####################################################################
# tmpnam
# FIXME: Non-obvious: return overlaps arg 1 iff arg 1 is not null _and_ 
# the return value is not null.
#####################################################################
  set attr(tmpnam) "$side_effect_on_environment,
                    $returns_static_like if ( index = 1,
                                              type = input,
                                              test_type = not_equal,
                                              test_value = 0 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                    buffer ( buffer_index = 1, 
                             type = write,
                             padding = 1 ) if ( index = 1,
                                                type = input,
                                                test_type = not_equal,
                                                test_value = 0 ),
                    no_other_side_effects"

#####################################################################
# fclose
#####################################################################
  set attr(fclose) "$fclose_like,
                    memmod ( index     =  1,
                             first_bit =  0,
                             num_bits  = -1 ),
                    property ( index = 1,
                               property_type = requires,                  
                               type = input,                  
                               property_name = \"file source\",
                               property_value = \"from fopen_like\" ),
                    $side_effect_on_environment,
                    no_other_side_effects"

#####################################################################
# fflush
#####################################################################
  # Requires arg 1 be from fopen_like function unless it's null.
  set attr(fflush) "property ( index = 1,
                               property_type = requires,                  
                               type = input,                  
                               property_name = \"file source\",
                               property_value = \"from fopen_like\" ),
                    buffer ( buffer_index = 1, 
                             type = write,
                             size = 1,
                             units = elements ) if ( index = 1,
                                                     type = input,
                                                     test_type = not_equal,
                                                     test_value = 0 ),
                    $side_effect_on_environment,
                    no_other_side_effects"

#####################################################################
# fopen
#####################################################################
  set attr(fopen) "$requires_arg_1_non_null_like,
                   $requires_arg_2_non_null_like,
                   $fopen_like,
                   property ( index = return,
                              property_type = provides,                  
                              type = output,                  
                              property_name = \"file source\",
                              property_value = \"from fopen_like\" ), 
                   $side_effect_on_environment,
                   no_other_side_effects"

#####################################################################
# freopen
#####################################################################
  set attr(freopen)        "$requires_arg_2_non_null_like,
                            memmod ( index     =  3,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            property ( index = 3,
                                       property_type = requires,                  
                                       type = input,                  
                                       property_name = \"file source\",
                                       property_value = \"from fopen_like\" ),
                            property ( index = return, 
                                       property_type = provides,                  
                                       type = output,                  
                                       property_name = \"file source\",
                                       property_value = \"from fopen_like\" ),
                            deallocator ( pointer_index = 3,
                                          resource = \"file\" )
                                        if (index=return,
                                            type=output,
                                            test_type=equal,
                                            test_value=0),
                            return_overlap ( return_index = return,
                                             points_into_index = 3,
                                             fate = may  ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# setbuf
#####################################################################
  set attr(setbuf)         "buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            property ( index = 1,
                                       property_type = requires,                  
                                       type = input,                  
                                       property_name = \"file source\",
                                       property_value = \"from fopen_like\" ),
                            anchor ( index = 2 ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# fprintf
#####################################################################
  set attr(fprintf)        "format ( kind = printf, 
                                     string_index = 2,
                                     first_to_check = 3 ),
                            memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            $requires_arg_2_non_null_like,
                            $side_effect_on_environment,
                            no_other_side_effects"

  # NOTE: The scanf family of functions in this section are among the few 
  #       that have side effects which we can't express. Don't use 
  #       no_other_side_effects with them.

#####################################################################
# fscanf
#####################################################################
  set attr(fscanf)         "format ( kind = scanf,
                                     string_index = 2,
                                     first_to_check = 3 ),
                            memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            no_other_anchoring,
                            $requires_arg_2_non_null_like"

#####################################################################
# printf
#####################################################################
  set attr(printf)         "format ( kind = printf,
                                     string_index = 1,
                                     first_to_check = 2 ),
                            $requires_arg_1_non_null_like,
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# scanf
#####################################################################
  set attr(scanf)          "format ( kind = scanf,
                                     string_index = 1,
                                     first_to_check = 2 ),
                            no_other_anchoring,
                            $requires_arg_1_non_null_like"

#####################################################################
# snprintf
#####################################################################
  set attr(snprintf)       "format ( kind = printf,
                                     string_index = 3,
                                     first_to_check = 4 ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2,
                                     units = elements ),
                            $requires_arg_3_non_null_like,
                            no_other_side_effects"

#####################################################################
# sprintf
# The buffer attribute is used in magical ways.
#####################################################################
  set attr(sprintf)        "format ( kind = printf,
                                     string_index = 2,
                                     first_to_check = 3 ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     string_index = 2,
                                     padding = 1 ),
                            no_other_side_effects"

#####################################################################
# sscanf
#####################################################################
  set attr(sscanf)         "format ( kind = scanf,
                                     string_index = 2,
                                     first_to_check = 3 ),
                            no_other_anchoring,
                            $requires_arg_1_non_null_like,
                            $requires_arg_2_non_null_like"

#####################################################################
# vfprintf
#####################################################################
  set attr(vfprintf)       "memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            $requires_arg_2_non_null_like,
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# vfscanf
#####################################################################
  set attr(vfscanf)        "memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            no_other_anchoring,
                            $requires_arg_2_non_null_like"

#####################################################################
# vprintf
#####################################################################
  set attr(vprintf)        "$requires_arg_1_non_null_like,
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# vscanf
#####################################################################
  set attr(vscanf)         "no_other_anchoring,
                            $requires_arg_1_non_null_like"

#####################################################################
# vsnprintf
#####################################################################
  set attr(vsnprintf)      "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2,
                                     units = elements ),
                            $requires_arg_3_non_null_like,
                            no_other_side_effects"

#####################################################################
# vsprintf
#####################################################################
  set attr(vsprintf)       "memmod ( index     =  1,
                                     first_bit =  0,
                                     num_bits  = -1 ),
                            $requires_arg_2_non_null_like,
                            no_other_side_effects"

#####################################################################
# vsscanf
#####################################################################
  set attr(vsscanf)        "no_other_anchoring,
                            $requires_arg_1_non_null_like,
                            $requires_arg_2_non_null_like"

#####################################################################
# fgetc
#####################################################################
  set attr(fgetc)          "buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# fgets
#####################################################################
  set attr(fgets)          "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2,
                                     units = elements ),
                            buffer ( buffer_index = 3,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 3,
                                     type = write,
                                     size = 1,
                                     units = elements),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# fputc
#####################################################################
  set attr(fputc)          "buffer ( buffer_index = 2,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# fputs
#####################################################################
  set attr(fputs)          "buffer ( buffer_index = 1,
                                     type = read,
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# getc
#####################################################################
  set attr(getc)           "buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# getchar
#####################################################################
  set attr(getchar)        "$side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# gets
#####################################################################
  set attr(gets)           "memmod ( global_variable = NULL,
                                     index           = 1,
                                     first_bit       = 0,
                                     num_bits        = -1 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# putc
#####################################################################
  set attr(putc)           "buffer ( buffer_index = 2,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# putchar
#####################################################################
  set attr(putchar)        "$side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# puts
#####################################################################
  set attr(puts)           "buffer ( buffer_index = 1,
                                     type = read,
                                     string_index = 1,
                                     padding = 1 ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# ungetc
#####################################################################
  set attr(ungetc)         "buffer ( buffer_index = 2,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            $side_effect_on_environment,
                            no_other_side_effects"


#####################################################################
# atoi and friends
#####################################################################
  set attr(atoi)           "$requires_arg_1_non_null_like,
                            pure,
                            no_other_side_effects"

#####################################################################
# strtod and friends
#####################################################################
  set attr(strtod) {
# The 1st argument is the string to be converted. So we read it.
      buffer ( buffer_index = 1,
               type = read,
               string_index = 1,
               padding = 1,
               units = elements ),
# The 2nd argument is being written to, if it is not NULL.
# The 2nd argument points into the memory pointed to by the 1st argument
# FIXME: this ought to be conditional but return_overlap does not accept
# FIXME: conditions
    buffer ( buffer_index = 2, 
             type  = write,
             size  = 1,
             units = elements )
        if ( index = 2,
             type  = input,
             test_type = not_equal,
             test_value = 0 ),
      return_overlap ( return_index = 2,
                       points_into_index = 1,
                       fate = must ),
    no_other_side_effects
  }

#####################################################################
# strtol and friends
#####################################################################
  set attr(strtol) {
# The 1st argument is the string to be converted. So we read it.
      buffer ( buffer_index = 1,
               type = read,
               string_index = 1,
               padding = 1,
               units = elements ),
# The 2nd argument is being written to, if it is not NULL.
# The 2nd argument points into the memory pointed to by the 1st argument
# FIXME: this ought to be conditional but return_overlap does not accept
# FIXME: conditions
    buffer ( buffer_index = 2, 
             type  = write,
             size  = 1,
             units = elements )
        if ( index = 2,
             type  = input,
             test_type = not_equal,
             test_value = 0 ),
      return_overlap ( return_index = 2,
                       points_into_index = 1,
                       fate = must  ),
# The 3rd argument is the base with the following constraint:
# base >= 0 && base <= 36 && base != 1
    property ( index = 3,
               property_type = requires,
               type = input,
               test_type = greater_than_or_equal,
               test_value = 0 ),
    property ( index = 3,
               property_type = requires,
               type = input,
               test_type = less_than_or_equal,
               test_value = 36 ),
    property ( index = 3,
               property_type = requires,
               type = input,
               test_type = not_equal,
               test_value = 1 ),
    no_other_side_effects
  }

#####################################################################
# rand
#####################################################################
  set attr(rand)  "$side_effect_on_environment,
                   $return_ge_0_like,
                   no_other_side_effects"

#####################################################################
# srand
#####################################################################
  set attr(srand) "$side_effect_on_environment,
                   no_other_side_effects"

#####################################################################
# calloc
#####################################################################
  set attr(calloc) "$calloc_like, 
                    $returns_malloc_like,
                    no_other_side_effects"

#####################################################################
# free
#####################################################################
  set attr(free) "$free_like,
                  $requires_malloc_like_arg_1,
                  no_other_side_effects"

#####################################################################
# malloc
#####################################################################
  set attr(malloc) "$malloc_like,
                    $returns_malloc_like,
                    no_other_side_effects"

#####################################################################
# realloc
#####################################################################
  set attr(realloc) "$realloc_like,
                     $requires_malloc_like_arg_1,
                     $returns_malloc_like,
                     no_other_side_effects"

#####################################################################
# abort
#####################################################################
  set attr(abort) "noreturn, 
                   assert,
                   no_other_side_effects"

#####################################################################
# atexit
#####################################################################
  set attr(atexit) "$requires_arg_1_non_null_like,
                    anchor ( index = 1 ) if ( index = return,
                                              type = output,
                                              test_type = not_equal,
                                              test_value = 0 ),
                    no_other_side_effects"

#####################################################################
# exit
#####################################################################
  set attr(exit) "noreturn,
                  no_other_side_effects"

#####################################################################
# getenv
#####################################################################
  set attr(getenv)         "$requires_arg_1_non_null_like,
                            $returns_static_like,
                            pure,
                            no_other_side_effects"

#####################################################################
# system
#####################################################################
  set attr(system) "$side_effect_on_environment,
                    no_other_side_effects"

#####################################################################
# bsearch
#* FIXME: return_overlaps for bsearch needs to be:
#         $return_overlaps_arg_2 if ( index = return,
#                                     type = output,
#                                     test_type = not_equal,
#                                     test_value = 0 ),
# The first argument points to memory of unknown size which is read.
# We assume that 1 byte is read, which is guaranteed to happen.
#####################################################################
  set attr(bsearch)        "pure,
                          property ( index = 1, 
                                     property_type = requires,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 2, 
                                     type = read, 
                                     size_index = 4, 
                                     multiplier_index = 3 ),
                          property ( index = 5, 
                                     property_type = requires, 
                                     type = input, 
                                     test_type = not_equal, 
                                     test_value = 0),
                    return_overlap ( return_index = return,
                                     points_into_index = 2,
                                     fate = may ),
                            no_other_side_effects"

#####################################################################
# qsort
#####################################################################
  set attr(qsort)          "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2,
                                     multiplier_index = 3 ),
                          property ( index = 4, 
                                     property_type = requires, 
                                     type = input, 
                                     test_type = not_equal, 
                                     test_value = 0),
                            no_other_side_effects"

#####################################################################
# abs and friends
# FIXME: If we could represent the smallest integer, we could write
# FIXME: an attribute saying that abs() and friends can not be applied
# FIXME: to the most negative integer (the result is undefined).
#####################################################################
  set attr(abs) "const, 
                 $return_ge_0_like,
                 no_other_side_effects"

#####################################################################
# div and friends
#####################################################################
  set attr(div) "const,
                 property ( index = 2,
                            property_type = requires,
                            type = input,
                            test_type = not_equal,
                            test_value = 0 ),
                 no_other_side_effects"

#####################################################################
# mblen
#####################################################################
  set attr(mblen)          "pure,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     bound_string_index = 1,
                                     bound_size_index   = 2,
                                     bound_padding = 1 )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            property ( index = return,
                                       property_type = provides,
                                       type = output,
                                       test_type = greater_than_or_equal,
                                       test_value = -1 )
                                  if ( index = 1,
                                       type = input,
                                       test_type = not_equal,
                                       test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# mbtowc
#####################################################################
  set attr(mbtowc)         "buffer ( buffer_index = 2,
                                     type = read,
                                     bound_string_index = 2,
                                     bound_size_index   = 3,
                                     bound_padding = 1 )
                                if ( index = 2,
                                     type  = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     padding = 1,
                                     units = elements )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            property ( index = return,
                                       property_type = provides,
                                       type = output,
                                       test_type = greater_than_or_equal,
                                       test_value = -1 )
                                  if ( index = 2,
                                       type = input,
                                       test_type = not_equal,
                                       test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# wctomb
# We assume that 2 bytes are written into the memory pointed to by the 1st arg.
# We don't know that but it seems conservative enough
#####################################################################
  set attr(wctomb)         "buffer ( buffer_index = 1,
                                     type = write,
                                     size = 2)
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            property ( index = return,
                                       property_type = provides,
                                       type = output,
                                       test_type = greater_than_or_equal,
                                       test_value = -1 )
                                  if ( index = 1,
                                       type = input,
                                       test_type = not_equal,
                                       test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# mbstowcs
# Approximate: assume #arg1 chars are written; assume the 2nd arg
# string is read completely
#####################################################################
  set attr(mbstowcs)       "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 3,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                            no_other_side_effects"

#####################################################################
# memcpy
#####################################################################
  set attr(memcpy)         "buffer ( buffer_index = 2,
                                     type = read,
                                     size_index = 3 ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# strcpy
#####################################################################
  set attr(strcpy)         "buffer ( buffer_index = 1,
                                     type = write,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# strncpy
# Non-obvious: this does not bound the writing of characters based on
# the length of the string being copied from, but rather fills in the
# destination string with null characters until the specified number
# of character has been written, so don't use bound keywords for the
# write buffer. 
#####################################################################
  set attr(strncpy)        "buffer ( buffer_index = 1, 
                                     type = write,
                                     size_index = 3,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     bound_string_index = 2,
                                     bound_size_index = 3, 
                                     bound_padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# strcat
#####################################################################
  set attr(strcat)         "buffer ( buffer_index = 1,
                                     type = write,
                                     string_index = 1,
                                     string2_index = 2,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# strncat
# D.B.: After reading the documentation several times it looks right to me.
# It surprised me that strncpy(a, b, n) guarantees not to write more that 
# n bytes into a, but strncat(a, b, n) may write n+1 bytes.
#####################################################################
  set attr(strncat)        "buffer ( buffer_index = 1,
                                     type = write,
                                     string_index = 1,
                                     padding = 1,
                                     bound_string_index = 2,
                                     bound_size_index = 3,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     bound_string_index = 2,
                                     bound_size_index = 3,
                                     bound_padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# memcmp
#####################################################################
  set attr(memcmp)         "pure,
                            buffer ( buffer_index = 1,
                                     type = read, 
                                     size_index = 3 ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     size_index = 3 ),
                            no_other_side_effects"

#####################################################################
# strcmp
#####################################################################
  set attr(strcmp)         "pure,
                            buffer ( buffer_index = 1, 
                                     type = read, 
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read, 
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                            no_other_side_effects"

#####################################################################
# strncmp
#####################################################################
  set attr(strncmp)        "pure,
                            buffer ( buffer_index = 1, 
                                     type = read, 
                                     bound_string_index = 1,
                                     bound_padding = 1, 
                                     bound_size_index = 3,
                                     units = elements ),
                            buffer ( buffer_index = 2, 
                                     type = read, 
                                     bound_string_index = 2, 
                                     bound_padding = 1, 
                                     bound_size_index = 3,
                                     units = elements ),
                            no_other_side_effects"

#####################################################################
# strxfrm
#####################################################################
  set attr(strxfrm)        "buffer ( buffer_index = 1,
                                     type = write,
                                     bound_string_index = 2,
                                     bound_size_index = 3,
                                     bound_padding = 1,
                                     units = elements )
                                if ( index = 3,
                                     type = input, 
                                     test_type = not_equal, 
                                     test_value = 0 ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     bound_string_index = 2,
                                     bound_size_index = 3, 
                                     bound_padding = 1,
                                     units = elements )
                                if ( index = 3,
                                     type = input, 
                                     test_type = not_equal, 
                                     test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# memchr
# FIXME: return_overlaps needs to be conditional:
#         if ( index = return,
#              type = output,
#              test_type = not_equal,
#              test_value = 0 ),
#####################################################################
  set attr(memchr)         "pure,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                            no_other_side_effects"

#####################################################################
# strchr
# FIXME: return_overlaps needs to be conditional:
#####################################################################
  set attr(strchr)         "pure,
                            buffer ( buffer_index = 1, 
                                     type = read,
                                     string_index = 1, 
                                     padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                          property ( index = return, 
                                     property_type = provides, 
                                     type = output, 
                                     test_type = greater_than, 
                                     test_value = 0)
                                if ( index = 2,
                                     num_dereference = 0,
                                     type = input,
                                     test_type = equal,
                                     test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# strcspn
#####################################################################
  set attr(strcspn)        "pure,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                            no_other_side_effects"

#####################################################################
# strpbrk
# FIXME: return_overlaps needs to be conditional:
#         $return_overlaps_arg_1 if ( index = return,
#                                     type = output,
#                                     test_type = not_equal,
#                                     test_value = 0 ),
#####################################################################
  set attr(strpbrk)        "pure,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                            no_other_side_effects"

#####################################################################
# strtok
#####################################################################
  set attr(strtok)         "buffer ( buffer_index = 1,
                                     type = read, 
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ) 
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     string_index = 1,
                                     units = elements ) 
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     string_index = 2,
                                     padding = 1,
                                     units = elements ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# memset
#####################################################################
  set attr(memset)         "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects"

#####################################################################
# strerror
#####################################################################
  set attr(strerror) "$returns_static_like,
                      no_other_side_effects"

#####################################################################
# strlen
#####################################################################
  set attr(strlen)         "pure,
                            buffer ( buffer_index = 1,
                                     type = read,        
                                     string_index = 1,
                                     padding = 1,
                                     units = elements ),    
                            no_other_side_effects"

#####################################################################
# mktime
# Approximation: not all fields of the passed in struct tm * are being
# read. But we cannot express that. If we knew for sure that the first
# n bytes are always read then we could say that. In the absence of 
# such certainty, avoid invalid ERROR1 by not specifying anything read.
# But anything in the struct passed in could be written.
#####################################################################
  set attr(mktime)         "buffer ( buffer_index = 1,
                                     type = write,
                                     units = elements,
                                     size  = 1),
                            no_other_side_effects"

#####################################################################
# time
#####################################################################
  set attr(time)           "buffer ( buffer_index = 1, 
                                     type  = write,
                                     units = elements,
                                     size  = 1 )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            no_other_side_effects" 

#####################################################################
# asctime
#####################################################################
  set attr(asctime)        "buffer ( buffer_index = 1, 
                                     type  = read,
                                     size  = 1,
                                     units = elements ),
                            $returns_static_like,
                            no_other_side_effects"

#####################################################################
# gmtime
#####################################################################
  set attr(gmtime)         "buffer ( buffer_index = 1, 
                                     type  = read,
                                     size  = 1,
                                     units = elements ),
                            $returns_static_like,
                            no_other_side_effects"

#####################################################################
# strftime
#####################################################################
  set attr(strftime)       "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2,
                                     units = elements ),
                            buffer ( buffer_index = 3,
                                     type = read,
                                     string_index = 3,
                                     padding = 1,
                                     units = elements ),
                            buffer ( buffer_index = 4,
                                     type  = read,
                                     size  = 1,
                                     units = elements ),
                            no_other_side_effects"

#####################################################################
# fwide
#####################################################################
  set attr(fwide)          "buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            buffer ( buffer_index = 1,
                                     type = write,
                                     size = 1,
                                     units = elements ),
                            $side_effect_on_environment,
                            no_other_side_effects"

#####################################################################
# mbsinit
#####################################################################
  set attr(mbsinit)        "pure, 
                            no_other_side_effects,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ) 
                                if ( index = 1,
                                     type  = input,
                                     test_type = not_equal,
                                     test_value = 0 )"

#####################################################################
# mbrlen
#####################################################################
  set attr(mbrlen)         "buffer ( buffer_index = 1,
                                     type = read,
                                     bound_string_index = 1,
                                     bound_size_index = 2 )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 3,
                                     type = read,
                                     size = 1,
                                     units = elements )
                                if ( index = 3,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# mbrtowc
#####################################################################
  set attr(mbrtowc)        "buffer ( buffer_index = 1,
                                     type  = write,
                                     size  = 1, 
                                     units = elements )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 2,
                                     type = read,
                                     bound_string_index = 2,
                                     bound_size_index = 3 )
                                if ( index = 2,
                                     type  = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 4,
                                     type = read,
                                     size = 1,
                                     units = elements )
                                if ( index = 4,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            no_other_side_effects"

#####################################################################
# wcrtomb
#####################################################################
  set attr(wcrtomb)        "buffer ( buffer_index = 1,
                                     type = write )
                                if ( index = 1,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                            buffer ( buffer_index = 3,
                                     type = read,
                                     size = 1,
                                     units = elements )
                                if ( index = 3,
                                     type = input,
                                     test_type = not_equal,
                                     test_value = 0 ),
                                     no_other_side_effects"

#####################################################################
# wctype
#####################################################################
  set attr(wctype)         "pure,
                            buffer ( buffer_index = 1,
                                     type = read, 
                                     string_index = 1,
                                     padding = 1 ),
                            no_other_side_effects"

#####################################################################
# wctrans
#####################################################################
  set attr(wctrans)        "pure,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     string_index = 1,
                                     padding = 1 ),
                            no_other_side_effects"
}
