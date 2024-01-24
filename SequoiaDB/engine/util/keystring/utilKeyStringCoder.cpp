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

   Source File Name = utilKeyStringCoder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "keystring/utilKeyStringCoder.hpp"

namespace engine
{
namespace keystring
{
   void _keyStringCoder::encodeRID( const dmsRecordID &rid, void *buf )
   {
      CHAR *ptr = (CHAR *)buf ;
      encodeUnsignedNative( (UINT32)( rid._extent ), FALSE, ptr ) ;
      ptr += sizeof( UINT32 ) ;
      encodeUnsignedNative( (UINT32)( rid._offset ), FALSE, ptr ) ;
      return ;
   }

   dmsRecordID _keyStringCoder::decodeToRID( const void *buf ) const
   {
      dmsRecordID rid ;
      rid._extent = (dmsExtentID)(
            decodeToUnsignedNative<UINT32>( buf, FALSE ) ) ;
      rid._offset = (dmsOffset)(
            decodeToUnsignedNative<UINT32>(
                  (const CHAR *)buf + sizeof( UINT32 ), FALSE ) ) ;
      return rid ;
   }

   void _keyStringCoder::encodeLSN( UINT64 lsn, void *buf )
   {
      encodeUnsignedNative( lsn, TRUE, buf ) ;
   }

   UINT64 _keyStringCoder::decodeToLSN( const void *buf ) const
   {
      return decodeToUnsignedNative<UINT64>( buf, TRUE ) ;
   }

   void _keyStringCoder::encodeIndexID( const dmsIdxMetadataKey &id,
                                        BOOLEAN asUpperKey,
                                        void *buf )
   {
      CHAR *data = (CHAR *)buf ;
      encodeUnsignedNative( (UINT64)( id.getCLOrigUID() ), FALSE, data ) ;
      data += sizeof( UINT64 ) ;
      encodeUnsignedNative( (UINT32)( id.getCLOrigLID() ), FALSE, data ) ;
      data += sizeof( UINT32 ) ;
      encodeUnsignedNative( (UINT32)( ( asUpperKey ) ?
                                      ( id.getIdxInnerID() + 1 ) :
                                      ( id.getIdxInnerID() ) ),
                            FALSE, data ) ;
   }

   dmsIdxMetadataKey _keyStringCoder::decodeToIndexID( const void *buf ) const
   {
      return dmsIdxMetadataKey(
            (utilCLUniqueID)( decodeToUnsignedNative<UINT64>( (const CHAR *)buf, FALSE ) ),
            (UINT32)( decodeToUnsignedNative<UINT32>( (const CHAR *)buf + sizeof( UINT64 ), FALSE ) ),
            (utilIdxInnerID)( decodeToUnsignedNative<UINT32>( (const CHAR *)buf + sizeof( UINT64 ) + sizeof( UINT32 ), FALSE ) ) ) ;
   }

}
}
