# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2008-2010 IBM Corporation. All rights reserved.
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
#        Daniel Brand, Francis Wallingford, Florian Krohm
#
#    DESCRIPTION:
#
#        These parms enable more checks than BEAM's default parameters.
#        There may be more false positives and the BEAM runtime may increase,
#        but many more things will be checked and more bugs may be found.
#
#        We recommend that users run with these parameters once they are relatively
#        free of the default BEAM complaints and are looking for more elusive bugs.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        See ChangeLog for more recent modifications.

source beam_default_parms.tcl

#
# For C/C++
#

if { $beam::lang == "c" || $beam::lang == "cpp" } {
  # General parameters
  set beam::use_evidence_not_on_path "below"
  set beam::allocation_may_return_null "yes"

  # Complaint settings
  set beam::ERROR11(heap_memory)::enabling_policy "always"
  set beam::ERROR11(stack_memory)::enabling_policy "always"
  set beam::ERROR11(file)::enabling_policy "always"
  set beam::ERROR11::enabling_policy "always"
  set beam::ERROR18::enabling_policy "always"
  set beam::ERROR25::enabling_policy "mainline"
  set beam::MISTAKE6::enabling_policy "always"
  set beam::MISTAKE7::enabling_policy "always"
  set beam::MISTAKE10::enabling_policy "always"
  set beam::MISTAKE11::enabling_policy "some_side_effect"
  set beam::MISTAKE13::enabling_policy "always"
  set beam::MISTAKE14::enabling_policy "always"
  set beam::MISTAKE23::enabling_policy "always"
  set beam::MISTAKE24::operators { < > <= >= <? >? >> << & | ~ ^ / + - * % [] }
  set beam::MISTAKE24::mixed_operators { & | ~ ^ }
  set beam::WARNING1::enabling_policy "always"
  set beam::WARNING2::enabling_policy "always"
  set beam::WARNING4::enabling_policy "always"
  set beam::WARNING4::threshold_in_bytes 64
  set beam::WARNING8::enabling_policy "always"
  set beam::WARNING13::enabling_policy "always"
  set beam::WARNING13::disabled_files { *.h *.hpp *.H *.HPP }
  set beam::WARNING21::enabling_policy "always"
  set beam::WARNING23::enabling_policy "always"
}

#
# For Java
#
if { $beam::lang == "java" } {
  # Complaint settings
  set beam::MISTAKE7::enabling_policy "always"
  set beam::MISTAKE18::enabling_policy "always"
  set beam::MISTAKE23::enabling_policy "always"
  set beam::WARNING2::enabling_policy "always"
}
