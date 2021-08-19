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

   Source File Name = dmsLobDef.hpp

   Descriptive Name = Data Management Service Storage(Hash) Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDEF_HPP_
#define DMS_LOBDEF_HPP_

#include "dms.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   #define DMS_LOB_OID_LEN                   12
   #define DMS_LOB_INVALID_PAGEID            DMS_INVALID_EXTENT

   typedef SINT32 DMS_LOB_PAGEID ;

   #define DMS_LOB_VERSION_1                 1
   #define DMS_LOB_CUR_VERSION               2
   #define DMS_LOB_META_SEQUENCE             0

   #define DMS_LOB_COMPLETE                  1
   #define DMS_LOB_UNCOMPLETE                0

   /*
      Lob meta fixed size, 1K
   */
   #define DMS_LOB_META_LENGTH               ( 1024 )
   #define DMS_LOB_META_PIECESINFO_MAX_LEN   ( 320 )

   #define RTN_LOB_GET_SEQUENCE( offset, isMerge, log ) \
     ( (isMerge) ? ( ( (INT64)(offset) + DMS_LOB_META_LENGTH ) >> (log) ) : \
                   ( ( ( (INT64)offset) >> (log) ) + 1 ) )

   #define RTN_LOB_GET_OFFSET_IN_SEQUENCE( offset, isMerge, pagesize ) \
     ( (isMerge) ? ( ((INT64)(offset) + DMS_LOB_META_LENGTH ) & ((pagesize)-1) ) : \
                   ( (INT64)(offset) & ((pagesize)-1) ) )

   #define RTN_LOB_GET_OFFSET_OF_LOB( pageSz, sequence, offsetInSeq, isMerge ) \
      ( (isMerge) ? ( (SINT64)(sequence)*(SINT64)(pageSz)+ \
                      (SINT64)(offsetInSeq)-DMS_LOB_META_LENGTH ) : \
                    ( ((SINT64)(sequence)-1)*(SINT64)(pageSz)+ \
                      (SINT64)(offsetInSeq) ) )

   /*
      _dmsLobRecord define
   */
   struct _dmsLobRecord : public SDBObject
   {
      const bson::OID   *_oid ;
      UINT32            _sequence ;
      UINT32            _offset ;  /// offset in a page, not the offset of lob
      UINT32            _hash ;
      UINT32            _dataLen ;
      const CHAR        *_data ;

      _dmsLobRecord()
      :_oid( NULL ),
       _sequence( 0 ),
       _offset( 0 ),
       _hash( 0 ),
       _dataLen( 0 ),
       _data( NULL )
      {
      }

      void clear()
      {
         _oid = NULL ;
         _sequence = 0 ;
         _offset = 0 ;
         _hash = 0 ;
         _dataLen = 0 ;
         _data = NULL ;
         return ;
      }

      BOOLEAN empty() const
      {
         return NULL == _oid ;
      }

      void set( const bson::OID *oid,
                UINT32 sequence,
                UINT32 offset,
                UINT32 dataLen,
                const CHAR *data )
      {
         _oid = oid ;
         _sequence = sequence ;
         _offset = offset ;
         _hash = ossHash( ( const BYTE * )_oid->getData(), 12,
                          ( const BYTE * )( &_sequence ),
                          sizeof( _sequence ) ) ;
         _dataLen = dataLen ;
         _data = data ;
         return ;
      }

      string toString() const
      {
         stringstream ss ;
         if ( _oid )
         {
            ss << "oid:" << _oid->str() << ", " ;
         }
         ss << "sequence:" << _sequence
            << ", offset:" << _offset
            << ", len:" << _dataLen
            << endl ;
         return ss.str() ;
      }
   } ;
   typedef struct _dmsLobRecord dmsLobRecord ;

   #pragma pack(1)

   #define DMS_LOB_META_MERGE_DATA_VERSION   ( 2 )

   #define DMS_LOB_META_CURRENT_VERSION       DMS_LOB_META_MERGE_DATA_VERSION

   #define DMS_LOB_META_FLAG_PIECESINFO_INSIDE 0x00000001

   /*
      _dmsLobMeta define
   */
   struct _dmsLobMeta : public SDBObject
   {
      INT64       _lobLen ;
      UINT64      _createTime ;
      UINT8       _status ;
      UINT8       _version ;
      UINT16      _padding ;
      UINT64      _modificationTime ;
      UINT32      _flag ;
      INT32       _piecesInfoNum ;
      CHAR        _pad[476] ;

      _dmsLobMeta()
      :_lobLen( 0 ),
       _createTime( 0 ),
       _status( DMS_LOB_UNCOMPLETE ),
       _version( DMS_LOB_META_CURRENT_VERSION ),
       _padding( 0 ),
       _modificationTime( 0 ),
       _flag( 0 ),
       _piecesInfoNum( 0 )
      {
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
         SDB_ASSERT( sizeof( _dmsLobMeta ) == 512,
                     "Lob meta size must be 512" ) ;
         SDB_ASSERT( sizeof( _dmsLobMeta ) <= DMS_LOB_META_LENGTH,
                     "Lob meta size must <= DMS_LOB_META_LENGTH" ) ;
      }

      void clear()
      {
         _lobLen = 0 ;
         _createTime = 0 ;
         _status = DMS_LOB_UNCOMPLETE ;
         _version = DMS_LOB_META_CURRENT_VERSION ;
         _padding = 0 ;
         _modificationTime = 0 ;
         _flag = 0 ;
         _piecesInfoNum = 0 ;
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
      }

      BOOLEAN isDone() const
      {
         return ( DMS_LOB_COMPLETE == _status ) ? TRUE : FALSE ;
      }

      BOOLEAN hasPiecesInfo() const
      {
         return ( _flag & DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ? TRUE : FALSE ;
      }

      string toString() const
      {
         stringstream ss ;
         ss << "Len:" << _lobLen
            << ", CreateTime:" << _createTime
            << ", ModificationTime:" << _modificationTime
            << ", Status:" << (UINT32)_status
            << ", Version:" << (UINT32)_version
            << ", Flag:" << _flag
            << ", PiecesInfoNum:" << _piecesInfoNum
            << endl ;
         return ss.str() ;
      }
   } ;
   typedef struct _dmsLobMeta dmsLobMeta ;


   #define DMS_LOB_PAGE_NORMAL               ( 0 )
   #define DMS_LOB_PAGE_REMOVED              ( 1 )
   #define DMS_LOB_PAGE_FLAG_NEW             ( 1 )
   #define DMS_LOB_PAGE_FLAG_OLD             ( 0 )

   /*
      _dmsLobDataMapBlk define
   */
   struct _dmsLobDataMapBlk: public SDBObject
   {
      CHAR           _pad1[4] ;
      BYTE           _oid[DMS_LOB_OID_LEN] ;
      UINT32         _sequence ;
      UINT32         _dataLen ;
      SINT32         _prevPageInBucket ;
      SINT32         _nextPageInBucket ;
      UINT32         _clLogicalID ;
      UINT16         _mbID ;
      BYTE           _status ;
      BYTE           _newFlag ;
      CHAR           _pad2[24];  /// sizeof( _dmsLobDataMapBlk ) == 64B

      _dmsLobDataMapBlk()
      {
         reset() ;
         ossMemset( _pad2, 0, sizeof( _pad2 ) ) ;
         SDB_ASSERT( 64 == sizeof( _dmsLobDataMapBlk ), "invalid blk" ) ;
      }

      void reset()
      {
         ossMemset( this, 0, sizeof( _pad1 ) + sizeof( _oid ) ) ;

         _sequence = 0 ;
         _dataLen = 0 ;
         _prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
         _nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
         _clLogicalID = DMS_INVALID_CLID ;
         _mbID = DMS_INVALID_MBID ;
         _status = DMS_LOB_PAGE_REMOVED ;
         _newFlag = DMS_LOB_PAGE_FLAG_NEW ;
      }

      BOOLEAN isUndefined() const
      {
         const static BYTE s_emptyOID[ DMS_LOB_OID_LEN ] = { 0 } ;

         if ( 0 == _sequence && 0 == _dataLen &&
              0 == _clLogicalID && 0 == _mbID &&
              0 == ossMemcmp( _oid, s_emptyOID, DMS_LOB_OID_LEN ) )
         {
            return TRUE ;
         }
         return FALSE ;
      }

      BOOLEAN isNormal() const
      {
         return _status == DMS_LOB_PAGE_NORMAL ? TRUE : FALSE ;
      }
      BOOLEAN isRemoved() const
      {
         return _status == DMS_LOB_PAGE_REMOVED ? TRUE : FALSE ;
      }

      void setNormal() { _status = DMS_LOB_PAGE_NORMAL ; }
      void setRemoved() { _status = DMS_LOB_PAGE_REMOVED ; }

      BOOLEAN isNew() const
      {
         return _newFlag == DMS_LOB_PAGE_FLAG_NEW ? TRUE : FALSE ;
      }
      void setOld() { _newFlag = DMS_LOB_PAGE_FLAG_OLD ;}

      BOOLEAN equals( const BYTE *oid, UINT32 sequence ) const
      {
         /// compare sequence first.
         return ( _sequence == sequence &&
                  0 == ossMemcmp( _oid, oid, DMS_LOB_OID_LEN ) ) ?
                TRUE : FALSE ;
      }
   } ;
   typedef struct _dmsLobDataMapBlk dmsLobDataMapBlk ;

   #pragma pack()

   struct _dmsLobInfoOnPage
   {
      UINT32 _sequence ;
      UINT32 _len ;
      bson::OID _oid ;
   } ;
   typedef struct _dmsLobInfoOnPage dmsLobInfoOnPage ;
}

#endif // DMS_LOBDEF_HPP_

