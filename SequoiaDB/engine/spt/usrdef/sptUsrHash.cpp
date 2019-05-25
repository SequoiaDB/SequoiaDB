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

   Source File Name = sptUsrSystem.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrHash.hpp"
#include "ossUtil.hpp"
#include "ossIO.hpp"
#include "../bson/lib/md5.hpp"

using namespace bson ;

#define SPT_MD5_READ_LEN 1024

namespace engine
{
   JS_STATIC_FUNC_DEFINE( _sptUsrHash, md5 )
   JS_STATIC_FUNC_DEFINE( _sptUsrHash, fileMD5 )
   JS_STATIC_FUNC_DEFINE( _sptUsrHash, help )

   JS_BEGIN_MAPPING( _sptUsrHash, "Hash" )
      JS_ADD_STATIC_FUNC( "md5", md5 )
      JS_ADD_STATIC_FUNC( "fileMD5", fileMD5 )
      JS_ADD_STATIC_FUNC( "help", help )
   JS_MAPPING_END()


   INT32 _sptUsrHash::md5( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string code ;
      string str ;

      if ( 0 == arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "not specified the argument" ;
         goto error ;
      }
      rc = arg.getString( 0, str ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to get argument: %d", rc ) ;
         ss << "argument \'str\' must be a string" ;
         goto error ;
      }
      
      code = md5::md5simpledigest( str ) ; 
      rval.getReturnVal().setValue( code ) ;
       
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrHash::fileMD5( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 bufSize = SPT_MD5_READ_LEN ;
      SINT64 hasRead = 0 ;
      CHAR readBuf[SPT_MD5_READ_LEN + 1] = { 0 } ;
      OSSFILE file ;
      string filename ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      md5_state_t st ;
      md5_init( &st ) ;
      md5::md5digest digest ;
      string code ;

      if ( 0 == arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "not specified the file" ;
         goto error ;
      }
      rc = arg.getString( 0,  filename ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to get filename argument: %d", rc ) ;
         ss << "argument \'filename\' must be a string" ;
         goto error ;
      }
      rc = ossOpen( filename.c_str(), OSS_READONLY, 0, file ) ;
      if ( rc )
      {
         ss << "open file[" << filename.c_str() << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      while ( TRUE )
      {
         rc = ossReadN( &file, bufSize, readBuf, hasRead ) ;
         if ( SDB_EOF == rc || 0 == hasRead )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            ss << "Read file[" << filename.c_str() << "] failed, rc: " << rc ;
            goto error ;
         }
         md5_append( &st, (const md5_byte_t *)readBuf, hasRead ) ;
      }
      md5_finish( &st, digest ) ;
      code = md5::digestToString( digest ) ;
      rval.getReturnVal().setValue( code ) ;

   done:
      if ( TRUE == isOpen )
         ossClose( file ) ;
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrHash::help( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      stringstream ss ;
      ss << "Hash functions:" << endl
         << " Hash.md5( str )" << endl 
         << " Hash.fileMD5( filename )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

