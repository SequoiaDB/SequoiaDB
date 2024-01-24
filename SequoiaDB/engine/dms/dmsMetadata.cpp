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

   Source File Name = dmsMetadata.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsMetadata.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsCLMetadataKey implement
    */
   _dmsCLMetadataKey::_dmsCLMetadataKey( const dmsMetadataBlock *mb )
   : _clOrigUID(
         utilBuildCLUniqueID(
               utilGetCSUniqueID( mb->_clUniqueID ),
               mb->_origInnerUID ) ),
     _clOrigLID( mb->_origLID )
   {
   }

   void _dmsCLMetadataKey::init( const _dmsMetadataBlock *mb )
   {
      _clOrigUID = utilBuildCLUniqueID( utilGetCSUniqueID( mb->_clUniqueID ),
                                        mb->_origInnerUID ) ;
      _clOrigLID = mb->_origLID ;
   }

   /*
      _dmsIdxMetadataKey implement
    */
   _dmsIdxMetadataKey::_dmsIdxMetadataKey( const _dmsMetadataBlock *mb,
                                           const _ixmIndexCB *idxCB )
   : _dmsCLMetadataKey( mb ),
     _idxInnerID( utilGetIdxInnerIDWithFlag( idxCB->getUniqueID() ) )
   {
   }

   void _dmsIdxMetadataKey::init( const _dmsMetadataBlock *mb,
                                  const _ixmIndexCB *idxCB )
   {
      _dmsCLMetadataKey::init( mb ) ;
      _idxInnerID = utilGetIdxInnerIDWithFlag( idxCB->getUniqueID() ) ;
   }

   /*
      _dmsCSMetadata implement
    */
   _dmsCSMetadata::_dmsCSMetadata( dmsSUDescriptor *su )
   : _su( su ),
     _csUID( su ? su->getStorageInfo()._csUniqueID : UTIL_UNIQUEID_NULL )
   {
   }

   /*
      _dmsCLMetadata implement
    */
   _dmsCLMetadata::_dmsCLMetadata( dmsSUDescriptor *su,
                                   dmsMetadataBlock *mb,
                                   dmsMBStatInfo *mbStat )
   : _dmsCSMetadata( su ),
     _mb( mb ),
     _mbStat( mbStat ),
     _clInnerID( mb ? utilGetCLInnerID( mb->_clUniqueID ) : UTIL_UNIQUEID_NULL ),
     _clLID( mb ? mb->_logicalID : DMS_INVALID_LOGICCLID ),
     _clOrigInnerID( mb ? utilGetCLInnerID( mb->_origInnerUID ) : UTIL_UNIQUEID_NULL ),
     _clOrigLID( mb ? mb->_origLID : DMS_INVALID_LOGICCLID )
   {
   }

   UINT64 _dmsCLMetadata::fetchSnapshotID()
   {
      return _mbStat ? _mbStat->_snapshotID.fetch() : DMS_INVALID_SNAPSHOT_ID ;
   }

   /*
      _dmsIdxMetadata implement
    */
   _dmsIdxMetadata::_dmsIdxMetadata( dmsSUDescriptor *su,
                                     dmsMetadataBlock *mb,
                                     dmsMBStatInfo *mbStat,
                                     _ixmIndexCB *indexCB )
   : _dmsCLMetadata( su, mb, mbStat ),
     _idxInnerID( utilGetIdxInnerIDWithFlag( indexCB->getUniqueID() ) ),
     _idxLID( indexCB->getLogicalID() ),
     _keyPattern( indexCB->keyPattern() ),
     _ordering( Ordering::make( indexCB->keyPattern() ) ),
     _isUnique( indexCB->unique() ),
     _isStrictUnique( indexCB->isIDIndex() ),
     _isEnforced( indexCB->enforced() ),
     _isNotNull( indexCB->notNull() ),
     _isNotArray( indexCB->notArray() )
   {
   }

   _dmsIdxMetadata::_dmsIdxMetadata( const _dmsIdxMetadata &o, BOOLEAN getOwned )
   : _dmsIdxMetadata( o )
   {
      if ( getOwned )
      {
         _keyPattern = _keyPattern.copy() ;
      }
   }

}