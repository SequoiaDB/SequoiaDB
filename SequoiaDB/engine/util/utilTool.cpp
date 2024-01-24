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

   Source File Name = utilTool.cpp

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
#include "utilTool.hpp"
#include "ossUtil.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <iostream>

void sdbPrintError( const CHAR* format, ... )
{
   va_list ap ;
   CHAR errorMsg[ SDB_PRINT_STRINGMAX + 1 ] = { 0 } ;
   va_start( ap, format ) ;
   ossVsnprintf( errorMsg, SDB_PRINT_STRINGMAX, format, ap ) ;
   va_end( ap ) ;
   std::cerr << errorMsg << std::endl ;
}