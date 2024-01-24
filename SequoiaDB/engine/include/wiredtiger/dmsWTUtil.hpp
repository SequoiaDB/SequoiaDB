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

   Source File Name = dmsWTUtil.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_UTIL_HPP_
#define DMS_WT_UTIL_HPP_

#include "dmsDef.hpp"
#include "ossLikely.hpp"
#include "utilUniqueID.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   int dmsWTGetLastErrorCode() ;

   // convert WiredTiger error code to SequoiaDB error code
   INT32 dmsWTRCToDBRCSlow( int retCode, WT_SESSION *session, BOOLEAN checkConflict ) ;

   // quick convert WiredTiger error code to SequoiaDB error code
   OSS_INLINE INT32 dmsWTRCToDBRC( int retCode,
                                   WT_SESSION *session,
                                   BOOLEAN checkConflict = FALSE )
   {
      if ( OSS_LIKELY( 0 == retCode ) )
      {
         return SDB_OK ;
      }
      return dmsWTRCToDBRCSlow( retCode, session, checkConflict ) ;
   }

   // WiredTiger call wrapper with error code convert
   #define WT_CALL( func, session ) ( dmsWTRCToDBRC( ( func ), session ) )
   #define WT_CONFLICT_CALL( func, session ) ( dmsWTRCToDBRC( ( func ), session, TRUE ) )

   void dmsWTBuildDataIdent( utilCSUniqueID csUID,
                             utilCLInnerID clInnerID,
                             UINT32 clLID,
                             ossPoolStringStream &ss ) ;

   void dmsWTBuildIndexIdent( utilCSUniqueID csUID,
                              utilCLInnerID clInnerID,
                              UINT32 clLID,
                              utilIdxInnerID idxInnerID,
                              ossPoolStringStream &ss ) ;

   void dmsWTBuildLobIdent( utilCSUniqueID csUID,
                            utilCLInnerID clInnerID,
                            UINT32 clLID,
                            ossPoolStringStream &ss ) ;

   void dmsWTBuildIdentPrefix( utilCSUniqueID csUID,
                               ossPoolStringStream &ss ) ;

}
}

#endif // DMS_WT_UTIL_HPP_
