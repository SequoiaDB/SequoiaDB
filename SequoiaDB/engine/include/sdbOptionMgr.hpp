/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbOptionMgr.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/


#define SDB_POSITIONAL_OPTIONS_DESCRIPTION                        \
      destd.add ( "shell" , -1 );

#define SDB_ADD_PARAM_OPTIONS_BEGIN(desc)                         \
           desc.add_options()

#define SDB_COMMANDS_OPTIONS                                      \
      ("help,h", "help")                                          \
      ("version,v", "version")                                    \
      ("language,l", po::value< string >(),                       \
       "specified the display language, "                         \
       "can be \"en\" or \"cn\", default to be \"en\"")           \
      ("file,f", po::value< string >(),                           \
       "if the -f option is present, then commands are read from "\
       "the file specified by <string>")                          \
      ("eval,e", po::value< string >(),                           \
       "predefined variables(format: "                            \
       "\"var varname=\'varvalue\'\")")                           \
      ("shell,s", po::value< string >(),                          \
       "if the -s option is present, "                            \
       "then commands are read from <string>")


#define SDB_ADD_PARAM_OPTIONS_END ;
