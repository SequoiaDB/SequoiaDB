/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilTool.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          21/10/2022  CWJ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_TOOL_HPP_
#define UTIL_TOOL_HPP_

#include "core.hpp"

#define SDB_PRINT_STRINGMAX            ( 512 )

#define SDB_CHECK_PRINT_GOTOERROR( cond, retCode, fmt, ... ) \
   do{                                                       \
      if ( !( cond ) )                                       \
      {                                                      \
         rc = ( retCode ) ;                                  \
         sdbPrintError( fmt, ##__VA_ARGS__ ) ;               \
         goto error ;                                        \
      }                                                      \
   } while ( 0 )                                             \

#define SDB_RC_CHECK_PRINT_GOTOERROR( rc, fmt, ... )           \
   do{                                                         \
      SDB_CHECK_PRINT_GOTOERROR( ( SDB_OK == ( rc ) ), ( rc ), \
                                 fmt, ##__VA_ARGS__ ) ;        \
   } while( 0 )                                                \

void sdbPrintError( const CHAR* format, ... ) ;

#endif //UTIL_TOOL_HPP_