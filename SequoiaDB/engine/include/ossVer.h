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

#define SDB_ENGINE_VERISON_CURRENT     SDB_ENGINE_VERSION_2

/*
 *    SequoiaDB Engine Subversion
 */
#define SDB_ENGINE_SUBVERSION_0        0
#define SDB_ENGINE_SUBVERSION_1        1
#define SDB_ENGINE_SUBVERSION_2        2
#define SDB_ENGINE_SUBVERSION_3        3
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

#define SDB_ENGINE_SUBVERSION_CURRENT  SDB_ENGINE_SUBVERSION_9

/*
      SequoiaDB Engine Fix version
*/
#define SDB_ENGINE_FIXVERSION_1        1


/*
      Build time
*/

   #ifdef _DEBUG
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT"(Debug)"
   #else
      #define SDB_ENGINE_BUILD_TIME    SDB_ENGINE_BUILD_CURRENT
   #endif // _DEBUG


/*
 *    Get the version, subversion and release version.
 */
void ossGetVersion ( INT32 *version,
                     INT32 *subVersion,
                     INT32 *fixVersion,
                     INT32 *release,
                     const CHAR **ppBuild ) ;

void ossGetSimpleVersion( CHAR *pBuff, UINT32 bufLen ) ;

void ossPrintVersion( const CHAR *prompt ) ;

void ossSprintVersion( const CHAR *prompt, CHAR *pBuff, UINT32 len,
                       BOOLEAN multiLine ) ;

#endif /* OSSVER_HPP_ */

