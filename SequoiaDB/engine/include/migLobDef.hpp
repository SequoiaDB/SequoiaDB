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

   Source File Name = migLobDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MIG_LOBDEF_HPP_
#define MIG_LOBDEF_HPP_

#include "ossUtil.hpp"
#include "pd.hpp"

namespace lobtool
{
#define MIG_FILE_EYE "SDBLOBF"
#define MIG_HOSTNAME "hostname"
#define MIG_SERVICE "svcname"
#define MIG_USRNAME "usrname"
#define MIG_PASSWD "passwd"
#define MIG_CIPHERFILE "cipherfile"
#define MIG_CIPHER "cipher"
#define MIG_TOKEN "token"
#define MIG_SSL "ssl"
#define MIG_CL "collection"
#define MIG_OP "operation"
#define MIG_FILE "file"
#define MIG_IGNOREFE "ignorefe"
#define MIG_DST_HOST "dsthost"
#define MIG_DST_SERVICE "dstservice"
#define MIG_DST_CL "dstcollection"
#define MIG_DST_USRNAME "dstusrname"
#define MIG_DST_PASSWD "dstpasswd"
#define MIG_SESSION_PREFER "prefer"

#define MIG_OP_IMPRT "import"
#define MIG_OP_EXPRT "export"
#define MIG_OP_MIGRATION "migration"

#define MIG_LOB_TOOL_VERSION 1

enum MIG_OP_TYPE
{
   MIG_OP_TYPE_IMPRT = 1,
   MIG_OP_TYPE_EXPRT,
   MIG_OP_TYPE_MIGRATION
} ;

   struct migOptions
   {
      const CHAR *hostname ;
      const CHAR *service ;
      const CHAR *usrname ;
      const CHAR *passwd ;
      const CHAR *collection ;
      const CHAR *file ;
      MIG_OP_TYPE type ;
      BOOLEAN ignorefe ;
      const CHAR *dsthost ;
      const CHAR *dstservice ;
      const CHAR *dstcl ;
      const CHAR *dstusrname ;
      const CHAR *dstpasswd ;
      const CHAR *prefer ;
      UINT32 preferNum ;
#ifdef SDB_SSL
      BOOLEAN     useSSL ;
#endif

      migOptions()
      :hostname( NULL ),
       service( NULL ),
       usrname( NULL ),
       passwd( NULL ),
       collection( NULL ),
       file( NULL ),
       type( MIG_OP_TYPE_IMPRT ),
       ignorefe( FALSE ),
       dsthost( NULL ),
       dstservice( NULL ),
       dstcl( NULL ),
       dstusrname( NULL ),
       dstpasswd( NULL ),
       prefer( NULL ),
       preferNum( 0 )
      {
#ifdef SDB_SSL
         useSSL = FALSE ;
#endif
      }
   } ;

   /// 64KB
   struct migFileHeader 
   {
      CHAR eyeCatcher[8] ;
      UINT32 version ;
      UINT32 pad1 ; 
      UINT64 totalNum ;
      UINT64 crtTime ;
      CHAR pad[65504] ;

      migFileHeader()
      {
         SDB_ASSERT( 65536 == sizeof( migFileHeader ), "must be 64KB" ) ;
         ossMemset( this, 0, sizeof( migFileHeader ) ) ;
         ossMemcpy( eyeCatcher, MIG_FILE_EYE, sizeof( MIG_FILE_EYE ) ) ;
         version = MIG_LOB_TOOL_VERSION ;
      }

      std::string toString() const
      {
         std::stringstream ss ;
         CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestamp t ;
         t.time = crtTime / 1000 ;
         t.microtm = ( crtTime - ( t.time * 1000 ) ) * 1000 ;
         ossTimestampToString( t, szTimestmpStr ) ;

         ss << "File version: " << version
            << " TotalNum: " << totalNum
            << " CreateTime: " << szTimestmpStr ;
         return ss.str() ;
      }
   } ;
}

#endif

