/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilPipe.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilPipe.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "ossUtil.h"
#include "ossPrimitiveFileOp.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "ossNPipe.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace fs = boost::filesystem ;


INT32 getWaitPipeName ( const OSSPID & ppid , CHAR * buf , UINT32 bufSize )
{
   INT32          nWritten    = 0 ;
   INT32          rc          = SDB_OK ;

   SDB_ASSERT ( buf && bufSize > 0 , "invalid argument" ) ;

   nWritten = ossSnprintf ( buf, bufSize,
                            SDB_SHELL_WAIT_PIPE_PREFIX"%u",
                            ppid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < bufSize , SDB_INVALIDSIZE ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 getPipeNames( const OSSPID &ppid,
                    CHAR *f2bName, UINT32 f2bSize,
                    CHAR *b2fName, UINT32 b2fSize,
                    CHAR *f2bCtlName, UINT32 f2bCtlSize,
                    CHAR *b2fCtlName, UINT32 b2fCtlSize )
{
   INT32 rc       = SDB_OK ;
   INT32 nWritten = 0 ;

   SDB_ASSERT( f2bName && b2fName && f2bSize > 0 && b2fSize > 0 &&
               f2bCtlName && b2fCtlName && f2bCtlSize > 0 && b2fCtlSize > 0,
               "Invalid arguments" ) ;

   nWritten = ossSnprintf( f2bName, f2bSize,
                           SDB_SHELL_F2B_PIPE_PREFIX"%u",
                           ppid ) ;
   SH_VERIFY_COND( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND( (UINT32) nWritten < f2bSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf( b2fName, b2fSize,
                           SDB_SHELL_B2F_PIPE_PREFIX"%u",
                           ppid ) ;
   SH_VERIFY_COND( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND( (UINT32) nWritten < b2fSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf( f2bCtlName, f2bCtlSize,
                           SDB_SHELL_CTL_F2B_PIPE_PREFIX"%u",
                           ppid ) ;
   SH_VERIFY_COND( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND( (UINT32) nWritten < f2bCtlSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf( b2fCtlName, b2fCtlSize,
                           SDB_SHELL_CTL_B2F_PIPE_PREFIX"%u",
                           ppid ) ;
   SH_VERIFY_COND( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND( (UINT32) nWritten < b2fCtlSize , SDB_INVALIDSIZE ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 getPipeNames2( const OSSPID &ppid, const OSSPID &pid,
                     CHAR *f2bName, UINT32 f2bSize,
                     CHAR *b2fName, UINT32 b2fSize,
                     CHAR *f2bCtlName, UINT32 f2bCtlSize,
                     CHAR *b2fCtlName, UINT32 b2fCtlSize )
{
   INT32          rc          = SDB_OK ;
   INT32          nWritten    = 0 ;

   SDB_ASSERT ( f2bName && b2fName && f2bSize > 0 && b2fSize > 0 &&
                f2bCtlName && b2fCtlName && f2bCtlSize > 0 && b2fCtlSize > 0,
                "Invalid arguments" ) ;

   nWritten = ossSnprintf ( f2bName, f2bSize,
                            SDB_SHELL_F2B_PIPE_PREFIX"%u-%u",
                            ppid, pid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < f2bSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf ( b2fName, b2fSize,
                            SDB_SHELL_B2F_PIPE_PREFIX"%u-%u",
                            ppid, pid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < b2fSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf ( f2bCtlName, f2bCtlSize,
                            SDB_SHELL_CTL_F2B_PIPE_PREFIX"%u-%u",
                            ppid, pid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < f2bCtlSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf ( b2fCtlName, b2fCtlSize,
                            SDB_SHELL_CTL_B2F_PIPE_PREFIX"%u-%u",
                            ppid, pid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < b2fCtlSize , SDB_INVALIDSIZE ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 getPipeNames1( CHAR *bpf2bName, UINT32 bpf2bSize,
                     CHAR *bpb2fName, UINT32 bpb2fSize,
                     CHAR *bpf2bCtlName, UINT32 bpf2bCtlSize,
                     CHAR *bpb2fCtlName, UINT32 bpb2fCtlSize,
                     CHAR *f2bName, CHAR *b2fName,
                     CHAR *f2bCtlName, CHAR *b2fCtlName )
{
   INT32 rc = SDB_OK ;
   std::vector<std::string> names ;

   ossMemset( bpf2bName, 0, bpf2bSize ) ;
   ossMemset( bpb2fName, 0, bpb2fSize ) ;
   ossMemset( bpf2bCtlName, 0, bpf2bCtlSize ) ;
   ossMemset( bpb2fCtlName, 0, bpb2fCtlSize ) ;

   rc = ossEnumNamedPipes( names, f2bName, OSS_MATCH_LEFT ) ;
   if ( rc )
   {
      std::cerr << "enum pipes failed: " << rc ;
      goto error ;
   }
   if ( names.size() == 0 )
   {
      rc = SDB_FNE ;
      goto error ;
   }
   ossStrncpy( bpf2bName, (*names.begin()).c_str(), bpf2bSize - 1 ) ;

   names.clear() ;
   rc = ossEnumNamedPipes( names, b2fName, OSS_MATCH_LEFT ) ;
   if ( rc )
   {
      std::cerr << "enum pipes failed: " << rc ;
      goto error ;
   }
   if ( names.size() == 0 )
   {
      rc = SDB_FNE ;
      goto error ;
   }
   ossStrncpy( bpb2fName, (*names.begin()).c_str(), bpb2fSize - 1 ) ;

   names.clear() ;
   rc = ossEnumNamedPipes( names, f2bCtlName, OSS_MATCH_LEFT ) ;
   if ( rc )
   {
      std::cerr << "enum pipes failed: " << rc ;
      goto error ;
   }
   if ( names.size() == 0 )
   {
      rc = SDB_FNE ;
      goto error ;
   }
   ossStrncpy( bpf2bCtlName, (*names.begin()).c_str(), bpf2bCtlSize - 1 ) ;

   names.clear() ;
   rc = ossEnumNamedPipes( names, b2fCtlName, OSS_MATCH_LEFT ) ;
   if ( rc )
   {
      std::cerr << "enum pipes failed: " << rc ;
      goto error ;
   }
   if ( names.size() == 0 )
   {
      rc = SDB_FNE ;
      goto error ;
   }
   ossStrncpy( bpb2fCtlName, (*names.begin()).c_str(), bpb2fCtlSize - 1 ) ;

done:
   return rc ;
error:
   goto done ;
}

void clearDirtyShellPipe( const CHAR *prefix )
{
#ifndef _WINDOWS
   std::vector < std::string > names ;
   const CHAR *pFind = NULL ;
   OSSPID pid = OSS_INVALID_PID ;

   ossEnumNamedPipes( names, prefix, OSS_MATCH_LEFT ) ;
   for( UINT32 i = 0 ; i < names.size() ; ++i )
   {
      pFind = ossStrrchr( names[ i ].c_str(), '-' ) ;
      if ( pFind && 0 != ( pid = ossAtoi( pFind + 1 ) ) &&
           FALSE == ossIsProcessRunning( pid ) )
      {
         ossCleanNamedPipeByName( names[ i ].c_str() ) ;
      }
   }
#endif // _WINDOWS
}

