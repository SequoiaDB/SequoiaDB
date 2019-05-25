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

INT32 getPipeNames( const OSSPID & ppid , CHAR * f2bName , UINT32 f2bSize ,
                    CHAR * b2fName , UINT32 b2fSize )
{
   INT32          rc          = SDB_OK ;
   INT32          nWritten    = 0 ;

   SDB_ASSERT ( f2bName && b2fName && f2bSize > 0 && b2fSize > 0 ,
                "Invalid arguments" ) ;

   nWritten = ossSnprintf ( f2bName, f2bSize,
                            SDB_SHELL_F2B_PIPE_PREFIX"%u",
                            ppid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < f2bSize , SDB_INVALIDSIZE ) ;

   nWritten = ossSnprintf ( b2fName, b2fSize,
                            SDB_SHELL_B2F_PIPE_PREFIX"%u",
                            ppid ) ;
   SH_VERIFY_COND ( nWritten >= 0 , SDB_SYS ) ;
   SH_VERIFY_COND ( (UINT32) nWritten < b2fSize , SDB_INVALIDSIZE ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 getPipeNames2( const OSSPID & ppid , const OSSPID & pid ,
                     CHAR * f2bName , UINT32 f2bSize ,
                     CHAR * b2fName , UINT32 b2fSize )
{
   INT32          rc          = SDB_OK ;
   INT32          nWritten    = 0 ;

   SDB_ASSERT ( f2bName && b2fName && f2bSize > 0 && b2fSize > 0 ,
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

done :
   return rc ;
error :
   goto done ;
}

INT32 getPipeNames1( CHAR * bpf2bName , UINT32 bpf2bSize ,
                     CHAR * bpb2fName , UINT32 bpb2fSize ,
                     CHAR * f2bName , CHAR * b2fName )
{
   INT32 rc = SDB_OK ;
   std::vector < std::string > names ;

   ossMemset( bpf2bName, 0, bpf2bSize ) ;
   ossMemset( bpb2fName, 0, bpb2fSize ) ;

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

