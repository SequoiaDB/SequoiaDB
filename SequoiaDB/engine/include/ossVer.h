/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossVer.h

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
#ifndef OSSVER_H__
#define OSSVER_H__

#include "core.h"
#include "ossVer_Autogen.h"

/*
 *    SequoiaDB Engine Version
 */
#define SDB_ENGINE_VERSION_0           0
#define SDB_ENGINE_VERSION_1           1
#define SDB_ENGINE_VERSION_2           2
#define SDB_ENGINE_VERSION_3           3
#define SDB_ENGINE_VERSION_5           5

#define SDB_ENGINE_VERISON_CURRENT     SDB_ENGINE_VERSION_5

/*
 *    SequoiaDB Engine Subversion
 */
#define SDB_ENGINE_SUBVERSION_0        0
#define SDB_ENGINE_SUBVERSION_1        1
#define SDB_ENGINE_SUBVERSION_2        2
#define SDB_ENGINE_SUBVERSION_3        3
#define SDB_ENGINE_SUBVERSION_4        4
#define SDB_ENGINE_SUBVERSION_5        5
#define SDB_ENGINE_SUBVERSION_6        6
#define SDB_ENGINE_SUBVERSION_7        7
#define SDB_ENGINE_SUBVERSION_8        8
#define SDB_ENGINE_SUBVERSION_9        9
#define SDB_ENGINE_SUBVERSION_10       10
#define SDB_ENGINE_SUBVERSION_11       11
#define SDB_ENGINE_SUBVERSION_12       12
#define SDB_ENGINE_SUBVERSION_13       13
#define SDB_ENGINE_SUBVERSION_14       14

#define SDB_ENGINE_SUBVERSION_CURRENT  SDB_ENGINE_SUBVERSION_8

/*
      SequoiaDB Engine Fix version
*/
#define SDB_ENGINE_FIXVERSION_1        1
#define SDB_ENGINE_FIXVERSION_2        2
#define SDB_ENGINE_FIXVERSION_3        3
#define SDB_ENGINE_FIXVERSION_4        4
#define SDB_ENGINE_FIXVERSION_5        5
#define SDB_ENGINE_FIXVERSION_6        6
#define SDB_ENGINE_FIXVERSION_7        7

#define SDB_ENGINE_FIXVERSION_CURRENT  SDB_ENGINE_FIXVERSION_7

/*
      Build time
*/
#ifdef SDB_ENTERPRISE

   #ifdef _DEBUG
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT"(Enterprise Debug)"
   #else
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT"(Enterprise)"
   #endif // _DEBUG

#else

   #ifdef _DEBUG
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT"(Debug)"
   #else
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT
   #endif // _DEBUG

#endif // SDB_ENTERPRISE

/*
 *    Get the version, subversion and release version.
 */
void ossGetVersion ( INT32 *version,
                     INT32 *subVersion,
                     INT32 *fixVersion,
                     INT32 *release,
                     const CHAR **ppBuild,
                     const CHAR **ppGitVer ) ;

void ossGetSimpleVersion( CHAR *pBuff, UINT32 bufLen ) ;

void ossPrintVersion( const CHAR *prompt ) ;

void ossSprintVersion( const CHAR *prompt, CHAR *pBuff, UINT32 len,
                       BOOLEAN multiLine ) ;

#endif /* OSSVER_HPP_ */

