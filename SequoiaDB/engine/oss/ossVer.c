/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossVer.cpp

   Descriptive Name = Operating System Services Version

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains engine version information

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/16/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "ossUtil.h"
#include <iostream>

#define SDB_INVALID_GIT_VERSION  "$GITVER$"
#define SDB_GIT_VER_MAX_SIZE     40

static BOOLEAN ossGitVerValid()
{
   return ( 0 != ossStrcmp( SDB_ENGINE_GIT_VERSION, SDB_INVALID_GIT_VERSION ) ) ;
}

void ossGetVersion ( INT32 *version,
                     INT32 *subVersion,
                     INT32 *fixVersion,
                     INT32 *release,
                     const CHAR **ppBuild,
                     const CHAR **ppGitVer )
{
   if ( version )
      *version = SDB_ENGINE_VERISON_CURRENT ;
   if ( subVersion )
      *subVersion = SDB_ENGINE_SUBVERSION_CURRENT ;
   if ( release )
      *release = SDB_ENGINE_RELEASE_CURRENT ;
   if ( fixVersion )
   {
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
      *fixVersion = SDB_ENGINE_FIXVERSION_CURRENT ;
#else
      *fixVersion = 0 ;
#endif // SDB_ENGINE_FIXVERSION_CURRENT
   }
   if ( ppBuild )
      *ppBuild = SDB_ENGINE_BUILD_TIME ;
   if ( ppGitVer )
   {
      *ppGitVer = ossGitVerValid() ? SDB_ENGINE_GIT_VERSION : NULL ;
   }
}

void ossGetSimpleVersion( CHAR *pBuff, UINT32 bufLen )
{
   if ( NULL == pBuff || 0 == bufLen )
   {
      return ;
   }
   ossMemset( pBuff, 0, bufLen ) ;

#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   ossSnprintf( pBuff, bufLen - 1, "%d.%d.%d",
                SDB_ENGINE_VERISON_CURRENT,
                SDB_ENGINE_SUBVERSION_CURRENT,
                SDB_ENGINE_FIXVERSION_CURRENT ) ;
#else
   ossSnprintf( pBuff, bufLen - 1, "%d.%d",
                SDB_ENGINE_VERISON_CURRENT,
                SDB_ENGINE_SUBVERSION_CURRENT ) ;
#endif
}

void ossSprintVersion( const CHAR *prompt, CHAR *pBuff, UINT32 len,
                       BOOLEAN multiLine )
{
   const CHAR gitVerFieldName[] = "Git version: " ;
   // 2 bytes for delimiter( new line or ", "), 1 more byte for '\0'.
   CHAR gitVer[ sizeof( gitVerFieldName ) + SDB_GIT_VER_MAX_SIZE + 2 + 1 ] = { 0 } ;

   if ( 0 == len )
   {
      return ;
   }
   ossMemset( pBuff, 0, len ) ;

   if ( ossGitVerValid() )
   {
      ossSnprintf( gitVer, sizeof( gitVerFieldName ) + SDB_GIT_VER_MAX_SIZE + 2,
                   "%s%s%s", gitVerFieldName, SDB_ENGINE_GIT_VERSION,
                   multiLine ? OSS_NEWLINE : ", " ) ;
   }

#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   if ( multiLine )
   {
      ossSnprintf( pBuff, len - 1,
                   "%s: %d.%d.%d%sRelease: %d%s%s%s%s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_FIXVERSION_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_RELEASE_CURRENT,
                   OSS_NEWLINE, gitVer,
                   SDB_ENGINE_BUILD_TIME,
                   OSS_NEWLINE ) ;
   }
   else
   {
      ossSnprintf( pBuff, len - 1,
                   "%s: %d.%d.%d, Release: %d, %sBuild: %s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_FIXVERSION_CURRENT,
                   SDB_ENGINE_RELEASE_CURRENT,
                   gitVer, SDB_ENGINE_BUILD_TIME ) ;
   }
#else
   if ( multiLine )
   {
      ossSnprintf( pBuff, len - 1,
                   "%s: %d.%d%sRelease: %d%s%s%s%s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_RELEASE_CURRENT,
                   OSS_NEWLINE, gitVer,
                   SDB_ENGINE_BUILD_TIME,
                   OSS_NEWLINE ) ;
   }
   else
   {
      ossSnprintf( pBuff, len - 1,
                   "%s: %d.%d, Release: %d, %sBuild: %s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_RELEASE_CURRENT,
                   gitVer, SDB_ENGINE_BUILD_TIME ) ;
   }
#endif //SDB_ENGINE_FIXVERSION_CURRENT
}

void ossPrintVersion( const CHAR *prompt )
{
   CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   ossSprintVersion( prompt, verText, sizeof( verText ), TRUE ) ;
   std::cout << verText ;
}

