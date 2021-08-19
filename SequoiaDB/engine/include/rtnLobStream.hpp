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

   Source File Name = rtnLobStream.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBSTREAM_HPP_
#define RTN_LOBSTREAM_HPP_

#include "dmsLobDef.hpp"
#include "rtnLobWindow.hpp"
#include "rtnLobDataPool.hpp"
#include "rtnLobPieces.hpp"
#include "utilSectionMgr.hpp"
#include "msgDef.hpp"
#include "rtnLob.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   class _pmdEDUCB ;
   class _dmsStorageUnit ;
   class _dpsLogWrapper ;
   class _rtnContextBase ;
   class _rtnContextBuf ;

   class _rtnLobStream : public SDBObject
   {
   public:
      _rtnLobStream() ;
      virtual ~_rtnLobStream() ;

   public:

      INT32 open( const CHAR *fullName,
                  const bson::OID &oid,
                  INT32 mode,
                  INT32 flags,
                  _rtnContextBase *context,
                  _pmdEDUCB *cb ) ;

      INT32 close( _pmdEDUCB *cb ) ;

      INT32 write( UINT32 len,
                   const CHAR *buf,
                   _pmdEDUCB *cb ) ;

      INT32 read( UINT32 len,
                  _rtnContextBase *context,
                  _pmdEDUCB *cb,
                  UINT32 &read ) ;

      INT32 lock( _pmdEDUCB *cb,
                  INT64 offset,
                  INT64 length ) ;

      INT32 getRTDetail( _pmdEDUCB *cb, bson::BSONObj &detail ) ;

      /// buf may be invalid when do next read.
      /// copy data to your own buf if necessary.
      INT32 next( _pmdEDUCB *cb,
                  const CHAR **buf,
                  UINT32 &len ) ;

      INT32 seek( SINT64 offset,
                  _pmdEDUCB *cb ) ;

      INT32 truncate( INT64 len,
                      _pmdEDUCB *cb ) ;

      INT32 closeWithException( _pmdEDUCB *cb ) ;

      INT32 getMetaData( bson::BSONObj &meta ) ;

      OSS_INLINE const bson::OID &getOID() const
      {
         return _oid ;
      }

      OSS_INLINE BOOLEAN isOpened() const
      {
         return _opened ;
      }

      OSS_INLINE SINT64 curOffset() const
      {
         return _offset ;
      }

      virtual _dmsStorageUnit *getSU() = 0 ;

      OSS_INLINE void setDPSCB( _dpsLogWrapper *dpsCB )
      {
         _dpsCB = dpsCB ;
         return ;
      }

      OSS_INLINE const CHAR *getFullName() const
      {
         return _fullName ;
      }

      OSS_INLINE BOOLEAN isReadonly() const
      {
         return SDB_IS_LOBREADONLY_MODE( _mode ) ;
      }

      virtual void   getErrorInfo( INT32 rc,
                                   _pmdEDUCB *cb,
                                   _rtnContextBuf *buf )
      {
      }

      OSS_INLINE INT64 uniqueId() const
      {
         return _uniqueId ;
      }

      OSS_INLINE void setUniqueId( INT64 uniqueId )
      {
         _uniqueId = uniqueId ;
      }

      OSS_INLINE INT32 mode() const
      {
         return _getMode() ;
      }

   protected:
      OSS_INLINE _dmsLobMeta &_getMeta()
      {
         return _meta ;
      }

      OSS_INLINE INT32 _getPageSz() const
      {
         return _lobPageSz ;
      }

      OSS_INLINE _dpsLogWrapper *_getDPSCB()
      {
         return _dpsCB ;
      }

      OSS_INLINE _rtnLobDataPool &_getPool()
      {
         return _pool ;
      }

      OSS_INLINE _rtnLobPiecesInfo &_getPiecesInfo()
      {
         return _lobPieces ;
      }

      OSS_INLINE _utilSectionMgr &_getSectionMgr()
      {
         return _sectionMgr ;
      }

      OSS_INLINE INT32 _getMode() const
      {
         return _mode ;
      }

      OSS_INLINE INT32 _getFlags() const
      {
         return _flags ;
      }

      UINT32 _getSequence( INT64 offset ) const ;

   private:
      virtual INT32 _prepare( _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _queryLobMeta( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta,
                                   BOOLEAN allowUncompleted = FALSE,
                                   _rtnLobPiecesInfo* piecesInfo = NULL ) = 0 ;

      virtual INT32 _ensureLob( _pmdEDUCB *cb,
                                _dmsLobMeta &meta,
                                BOOLEAN &isNew ) = 0 ;

      virtual INT32 _getLobPageSize( INT32 &pageSize ) = 0 ;

      virtual INT32 _write( const _rtnLobTuple &tuple,
                            _pmdEDUCB *cb,
                            BOOLEAN orUpdate = FALSE ) = 0 ;

      virtual INT32 _writev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb,
                             BOOLEAN orUpdate = FALSE ) = 0 ;

      virtual INT32 _update( const _rtnLobTuple &tuple,
                             _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _updatev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _readv( const RTN_LOB_TUPLES &tuples,
                            _pmdEDUCB *cb,
                            const _rtnLobPiecesInfo* piecesInfo = NULL ) = 0 ;

      virtual INT32 _completeLob( const _rtnLobTuple &tuple,
                                  _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _lock( _pmdEDUCB *cb,
                           INT64 offset,
                           INT64 length ) = 0 ;

      virtual INT32 _close( _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _rollback( _pmdEDUCB *cb ) { return SDB_SYS ; }

      virtual INT32 _queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                 _dmsLobMeta &meta ) = 0 ;

      virtual INT32 _removev( const RTN_LOB_TUPLES &tuples,
                              _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _getRTDetail( _pmdEDUCB *cb, bson::BSONObj &detail ) = 0 ;

   private:
      INT32 _readFromPool( UINT32 len,
                           _rtnContextBase *context,
                           _pmdEDUCB *cb,
                           UINT32 &readLen ) ;

      INT32 _open4Create( _pmdEDUCB *cb ) ;

      INT32 _open4Read( _pmdEDUCB *cb ) ;

      INT32 _open4Write( _pmdEDUCB *cb ) ;

      INT32 _open4Remove( _pmdEDUCB *cb ) ;

      INT32 _open4Truncate( _pmdEDUCB *cb ) ;

      INT32 _writeLobMeta( _pmdEDUCB *cb, BOOLEAN withData = TRUE ) ;

      INT32 _meta2Obj( bson::BSONObj& obj ) const ;

      INT32 _writeOrUpdate( const _rtnLobTuple &tuple,
                            _pmdEDUCB *cb ) ;

      INT32 _writeOrUpdateV( RTN_LOB_TUPLES &tuples,
                            _pmdEDUCB *cb ) ;

      INT64 _calculateLockedLobLen( INT64 lobLen, BOOLEAN wholeLobLocked,
                                    INT64 lockedEnd ) ;

   private:
      INT64                _uniqueId ;
      CHAR                 _fullName[ DMS_COLLECTION_SPACE_NAME_SZ +
                                      DMS_COLLECTION_NAME_SZ + 2 ] ;
      _dpsLogWrapper*      _dpsCB ;
      bson::OID            _oid ;
      _dmsLobMeta          _meta ;
      bson::BSONObj        _metaObj ;
      _rtnLobDataPool      _pool ;
      BOOLEAN              _opened ;
      _rtnLobWindow        _lw ;
      UINT32               _mode ;
      INT32                _flags ;
      INT32                _lobPageSz ;
      UINT32               _logarithmic ;
      INT64                _offset ;

      _rtnLobPiecesInfo    _lobPieces ;
      BOOLEAN              _hasPiecesInfo ;

      _utilSectionMgr      _sectionMgr ;
      BOOLEAN              _wholeLobLocked ;

      BOOLEAN              _truncated ;
   } ;
   typedef class _rtnLobStream rtnLobStream ;
}

#endif

