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

   Source File Name = dpsUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2019  Linyoub  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSUTIL_HPP_
#define DPSUTIL_HPP_

#include "ossTypes.h"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   #define DPS_TRANS_STR_LEN           ( 64 )

   dpsLogConfig &dpsGetGlobalLogConfig() ;

   const CHAR* dpsTransStatusToString( INT32 status ) ;

   INT32 dpsGetTransIDFromString( const CHAR *pStr, DPS_TRANS_ID &transID ) ;

   const CHAR* dpsTransIDToString( const DPS_TRANS_ID &transID,
                                   CHAR *pBuff,
                                   UINT32 bufSize ) ;

   ossPoolString dpsTransIDToString( const DPS_TRANS_ID &transID ) ;

   const CHAR* dpsTransIDAttrToString( const DPS_TRANS_ID &transID,
                                       CHAR *pBuff,
                                       UINT32 bufSize ) ;

   ossPoolString dpsTransIDAttrToString( const DPS_TRANS_ID &transID ) ;

   void dpsFlags2String( UINT16 flags, CHAR * pBuffer, INT32 bufSize ) ;

   void dpsAppendFlagString( CHAR * pBuffer, INT32 bufSize,
                             const CHAR *flagStr ) ;

   typedef ossPoolSet< DPS_TRANS_ID > DPS_TRANS_ID_SET ;

   // downgrade transaction ID from v1 to v0
   // WARNING: will lose high 16 bits of timestamp
   DPS_TRANS_ID dpsTransIDDowngrade( const dpsTransID_v1 &transID ) ;

}

#endif // DPSUTIL_HPP_


