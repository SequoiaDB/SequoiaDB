#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import re
import optparse
import pdb

DESCRIPTION = 'ptmalloc_stats - print glibc ptmalloc malloc stats information.'
USAGE = "Usage: %prog [OPTIONS] <PID>"
EXTENDED_HELP = ''

GDB_EXEC_CMD='''/usr/bin/gdb --quiet -nx /proc/{PROC}/exe {PROC} <<EOF 2>&1 |
set width 0
set height 0
set pagination off
set target-async 1
set non-stop on
source malloc_info.txt
{FUNC_NAME}
quit
EOF
/bin/sed -n \
    -e 's/^\((gdb) \)*//' \
    -e '/^arena/p' \
    -e '/^Arena/p' \
    -e '/^system/p' \
    -e '/^in/p' \
    -e '/^chunk/p' \
    -e '/^Total/p'
'''
class ptmallocParse(optparse.OptionParser):
   def print_help(self, output=None):
      optparse.OptionParser.print_help(self, output)
   def format_epilog(self, formatter):
      return self.epilog if self.epilog is not None else ''

def main():
   program = os.path.basename(sys.argv[0]).replace(".py", "")
   parser = ptmallocParse(
      description=DESCRIPTION,
      usage=USAGE,
      epilog=EXTENDED_HELP,
      prog=program
   )

   parser.add_option("-t", "--type", action='store', type="string",
                 default="stats", dest="type",
                 help="the type of information going to collect. "
                     "Type: stats, bins_chunks, bins_chunks_unfold. Default: stats")

   opt, args = parser.parse_args()
   if opt.type == 'stats':
      function_name = "my_malloc_stats()"
   elif opt.type == "bins_chunks":
      function_name = "my_malloc_bins_chunks_fold()"
   elif opt.type == "bins_chunks_unfold":
      function_name = "my_malloc_bins_chunks_unfold()"
   else:
      print("Error: Invalid input type '{type}'.".format(type=opt.type))
      return 1


   
   if not args:
       print("Please input pid of process which going to be collected.")
       return 1
   elif len(args) > 1:
       print("Error: Invalid process id. Please input correct pid.")
       return 1
   else:
      process_id = args[0]
   
   proc_pid = os.path.abspath("/proc/"+process_id)
   if not os.path.exists(proc_pid):
       print("Error: Process of '{PID}' not found.".format(PID=process_id))
       return 1

   # Run GDB, strip out unwated noise.
   exec_cmd = GDB_EXEC_CMD.format(PROC=process_id, FUNC_NAME=function_name)
   #print(exec_cmd)
   output = os.popen(exec_cmd).read()
   print(output)

if __name__ == '__main__':
   sys.exit(main())
