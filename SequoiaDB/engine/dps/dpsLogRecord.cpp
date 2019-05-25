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

   Source File Name = dpsLogRecord.cpp

   Descriptive Name = Data Protection Services Log Record

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains implementation for log record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/05/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "dpsLogRecord.hpp"
#include "dms.hpp"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "ossMem.h"
#include "dpsLogRecordDef.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

using namespace bson ;
namespace engine
{
#define DPS_RECORD_ELE_HEADER_LEN 5

#define DPS_GET_RECORD_TAG(a) \
        (*((DPS_TAG *)((CHAR *)(a))))

#define DPS_GET_RECORD_LENGTH(a) \
        (*(UINT32 *)((CHAR *)(a) + sizeof(DPS_TAG)))

#define DPS_GET_RECORD_VALUE( a ) \
        ((CHAR *)(a) + DPS_RECORD_ELE_HEADER_LEN )

   _dpsLogRecord::_dpsLogRecord ()
   :_write(0)
   {
      ossMemset ( _data, 0, sizeof(_data) ) ;
      _result = SDB_OK ;
   }

   _dpsLogRecord::_dpsLogRecord( const _dpsLogRecord &record )
   :_head(record._head),
    _write( record._write )
   {
      ossMemcpy( _data, record._data, sizeof(_data) ) ;
      ossMemcpy( _dataHeader, record._dataHeader, sizeof(_dataHeader) ) ;
      _result = record._result ;
   }

   _dpsLogRecord::~_dpsLogRecord ()
   {
   }

   _dpsLogRecord &_dpsLogRecord::operator=( const _dpsLogRecord &record )
   {
      _head = record._head ;
      ossMemcpy( _data, record._data, sizeof(_data) ) ;
      ossMemcpy( _dataHeader, record._dataHeader, sizeof(_dataHeader) ) ;
      _write = record._write ;
      _result = record._result ;
      return *this ;
   }

   UINT32 _dpsLogRecord::alignedLen() const
   {
      UINT32 len = 0 ;
      for ( UINT32 i = 0; i < DPS_MERGE_BLOCK_MAX_DATA; i++ )
      {
         if ( DPS_INVALID_TAG == _dataHeader[i].tag )
         {
            break ;
         }
         else
         {
            len += _dataHeader[i].len ;
            len += sizeof(_dpsRecordEle) ;
         }
      }

      return ossRoundUpToMultipleX(len + sizeof(dpsLogRecordHeader),
                                   sizeof(UINT32)) ;
   }

   void _dpsLogRecord::clear()
   {
      if ( 0 != _write )
      {
         for ( UINT32 i = 0; i < _write; i++ )
         {
            _data[i] = NULL ;
            _dataHeader[i].tag = DPS_INVALID_TAG ;
            _dataHeader[i].len = 0 ;
         }

         _write = 0 ;
      }

      if ( DPS_INVALID_LSN_OFFSET != _head._lsn )
      {
         _head.clear() ;
      }

      _result = SDB_OK ;

      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_LOADROWBODY, "_dpsLogRecord::loadRowBody" )
   INT32 _dpsLogRecord::loadRowBody()
   {
      PD_TRACE_ENTRY ( SDB__DPSLGRECD_LOADROWBODY ) ;
      INT32 rc = SDB_OK ;

      PD_CHECK( _head._length >= sizeof( dpsLogRecordHeader) &&
                _head._length <= DPS_RECORD_MAX_LEN,
                SDB_DPS_CORRUPTED_LOG, error, PDERROR,
                "the length of record is out of range: %d",
                _head._length) ;
      if ( LOG_TYPE_DUMMY == _head._type )
      {
         goto done ;
      }

      {
      _dpsLogRecord::iterator iter = find( DPS_LOG_ROW_ROWDATA ) ;
      if ( !iter.valid() )
      {
         goto done ;
      }

      rc = loadBody( iter.value(), iter.len() - DPS_RECORD_ELE_HEADER_LEN ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse row-record(rc=%d)!", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DPSLGRECD_LOADROWBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_LOADBODY, "_dpsLogRecord::loadBody" )
   INT32 _dpsLogRecord::loadBody( const CHAR * pData, INT32 totalSize )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGRECD_LOADBODY );
      SDB_ASSERT( pData, "pData can't be null!" ) ;

      INT32 rc = SDB_OK ;
      INT32 loadSize = 0 ;
      const CHAR *location = pData ;
      while ( loadSize < totalSize )
      {
         DPS_TAG tag = DPS_GET_RECORD_TAG(location) ;
         UINT32 valueSize = DPS_GET_RECORD_LENGTH( location ) ;

         if ( DPS_MERGE_BLOCK_MAX_DATA == _write )
         {
            PD_LOG( PDERROR, "data num is larger than %d",
                    DPS_MERGE_BLOCK_MAX_DATA ) ;
            SDB_ASSERT( FALSE, "impossible" ) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }
         else if ( DPS_INVALID_TAG == tag )
         {
            break ;
         }
         else if ( (UINT32)( totalSize - loadSize ) < valueSize )
         {
            PD_LOG( PDERROR, "get a invalid value size:%d, total size: %d, "
                    "load size: %d", valueSize, totalSize, loadSize ) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }

         _dataHeader[_write].tag = tag ;
         _dataHeader[_write].len = valueSize ;
         _data[_write++] = DPS_GET_RECORD_VALUE(location)  ;
         loadSize += ( valueSize + DPS_RECORD_ELE_HEADER_LEN ) ;
         location += ( valueSize + DPS_RECORD_ELE_HEADER_LEN ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DPSLGRECD_LOADBODY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_LOAD, "_dpsLogRecord::load" )
   INT32 _dpsLogRecord::load( const CHAR *pData )
   {
      PD_TRACE_ENTRY ( SDB__DPSLGRECD_LOAD );
      SDB_ASSERT( NULL != pData, "impossible" ) ;
      INT32 rc = SDB_OK ;
      INT32 totalSize = 0 ;
      const CHAR *location = NULL ;
      _head = *(( dpsLogRecordHeader * )pData) ;

      if ( _head._length < sizeof( dpsLogRecordHeader ) ||
           DPS_RECORD_MAX_LEN < _head._length )
      {
         PD_LOG ( PDERROR, "the length of record is out of range: %d",
                  _head._length ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( LOG_TYPE_DUMMY == _head._type )
      {
         goto done ;
      }

      location = pData + sizeof( dpsLogRecordHeader ) ;

      totalSize = _head._length
                  - sizeof( dpsLogRecordHeader )
                  - DPS_RECORD_ELE_HEADER_LEN ;
      rc = loadBody( location, totalSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse row-record(rc=%d)!", rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB__DPSLGRECD_LOAD, rc );
      return rc ;
   error:
      _result = rc ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_FIND, "_dpsLogRecord::find" )
   _dpsLogRecord::iterator _dpsLogRecord::find( DPS_TAG tag ) const
   {
      PD_TRACE_ENTRY ( SDB__DPSLGRECD_FIND );
      _dpsLogRecord::iterator itr( this ) ;

      for ( UINT32 i = 0; i < DPS_MERGE_BLOCK_MAX_DATA; i++ )
      {
         if ( DPS_INVALID_TAG == _dataHeader[i].tag)
         {
            break ;
         }
         else if ( _dataHeader[i].tag  == tag )
         {
            itr._current = i ;
            break ;
         }
         else
         {
         }
      }
      PD_TRACE_EXIT( SDB__DPSLGRECD_FIND) ;
      return itr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_PUSH, "_dpsLogRecord::push" )
   INT32 _dpsLogRecord::push( DPS_TAG tag, UINT32 len, const CHAR *value )
   {
      PD_TRACE_ENTRY( SDB__DPSLGRECD_PUSH ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( DPS_INVALID_TAG != tag , "impossible" ) ;
      if ( DPS_MERGE_BLOCK_MAX_DATA == _write )
      {
         PD_LOG( PDERROR, "data num is larger than %d",
                 DPS_MERGE_BLOCK_MAX_DATA ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }
      else
      {
         _dataHeader[_write].tag = tag ;
         _dataHeader[_write].len = len ;
         _data[_write++] = value ;
      }
   done:
       PD_TRACE_EXITRC( SDB__DPSLGRECD_PUSH, rc ) ;
       return rc ;
   error:
       goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGRECD_DUMP, "_dpsLogRecord::dump" )
   UINT32 _dpsLogRecord::dump ( CHAR *outBuf,
                                UINT32 outSize,
                                UINT32 options ) const
   {
      PD_TRACE_ENTRY( SDB__DPSLGRECD_DUMP ) ;
      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;

      if ( DPS_DMP_OPT_HEX & options )
      {
         hexDumpOption |= OSS_HEXDUMP_INCLUDE_ADDR ;
         if ( !(DPS_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }

         if ( 0 != _write )
         {
            ossHexDumpBuffer ( _data[0]-sizeof(dpsLogRecordHeader)-
                               DPS_RECORD_ELE_HEADER_LEN,
                               _head._length, outBuf, outSize, NULL,
                               hexDumpOption ) ;
         }
         else
         {
            ossHexDumpBuffer ( (void*)&_head, sizeof(_head),
                               outBuf, outSize, NULL,
                               hexDumpOption ) ;
         }
         len = ossStrlen ( outBuf ) ;
         outBuf [ len ] = '\n' ;
         ++len ;
      }

      if ( DPS_DMP_OPT_FORMATTED & options )
      {
         dpsLogRecord::iterator itrTransID, itrTransLsn, itrTransRel ;

         /* dump output looks like:
          * Version : 0x00000001(1)
          * LSN     : 0x0000000012345678(305419896)
          * PreLSN  : 0x0000000010002354(268444500)
          * Length  : 356
          * Type    : INSERT(1)
          * Name    : foo.bar
          * Insert  : { hello: "world" }
          */
         len += ossSnprintf ( outBuf + len, outSize - len,
                              OSS_NEWLINE
                              " Version: 0x%08lx(%d)"OSS_NEWLINE,
                              _head._version, _head._version ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " LSN    : 0x%016lx(%lld)"OSS_NEWLINE,
                              _head._lsn, _head._lsn ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " PreLSN : 0x%016lx(%lld)"OSS_NEWLINE,
                              _head._preLsn, _head._preLsn ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Length : %d"OSS_NEWLINE,
                              _head._length ) ;

         itrTransID = this->find( DPS_LOG_PUBLIC_TRANSID ) ;
         itrTransLsn = this->find( DPS_LOG_PUBLIC_PRETRANS ) ;
         itrTransRel = this->find( DPS_LOG_PUBLIC_RELATED_TRANS ) ;

         switch ( _head._type )
         {
         case LOG_TYPE_DUMMY :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "PAD", LOG_TYPE_DUMMY ) ;
            break ;
         }
         case LOG_TYPE_DATA_INSERT :
         {
            dpsLogRecord::iterator itrName, itrObj ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "INSERT", LOG_TYPE_DATA_INSERT ) ;
            itrName = this->find(DPS_LOG_PUBLIC_FULLNAME) ;
            if ( !itrName.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " FullName : %s"OSS_NEWLINE,
                                 itrName.value() ) ;
            itrObj = this->find( DPS_LOG_INSERT_OBJ ) ;
            if ( !itrObj.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find obj in record" ) ;
               PD_LOG( PDERROR, "Failed to find obj in record" ) ;
               goto done ;
            }

            try
            {
               BSONObj obj( itrObj.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Insert : %s"OSS_NEWLINE,
                                    obj.toString().c_str() ) ;
            }
            catch ( std::exception &e )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s: %s"OSS_NEWLINE,
                                    "Invalid insert record", e.what() ) ;
               goto done ;
            }
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "UPDATE", LOG_TYPE_DATA_UPDATE ) ;

            dpsLogRecord::iterator itrFullName, itrOldM, itrOldO,
                                   itrNewM, itrNewO,
                                   itrOldSK, itrNewSK ;
            itrFullName = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrFullName.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " FullName : %s"OSS_NEWLINE,
                                 itrFullName.value() ) ;

            itrOldM = this->find( DPS_LOG_UPDATE_OLDMATCH ) ;
            if ( !itrOldM.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find oldmatch in record" ) ;
               PD_LOG( PDERROR, "Failed to find oldmatch in record" ) ;
               goto done ;
            }

            itrOldO = this->find( DPS_LOG_UPDATE_OLDOBJ ) ;
            if ( !itrOldO.valid() )
            {
               PD_LOG( PDERROR, "failed to find oldobj in record" ) ;
               goto done ;
            }

            itrNewM = this->find( DPS_LOG_UPDATE_NEWMATCH ) ;
            if ( !itrNewM.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find newmatch in record" ) ;
               PD_LOG( PDERROR, "Failed to find newmatch in record" ) ;
               goto done ;
            }

            itrNewO = this->find( DPS_LOG_UPDATE_NEWOBJ ) ;
            if ( !itrNewO.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find newobj in record" ) ;
               PD_LOG( PDERROR, "Failed to find newobj in record" ) ;
               goto done ;
            }

            itrOldSK = this->find( DPS_LOG_UPDATE_OLDSHARDINGKEY ) ;
            itrNewSK = this->find( DPS_LOG_UPDATE_NEWSHARDINGKEY ) ;

            try
            {
               BSONObj oldM( itrOldM.value() ) ;
               BSONObj oldO( itrOldO.value() ) ;
               BSONObj newM( itrNewM.value() ) ;
               BSONObj newO( itrNewO.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Orig id : %s"OSS_NEWLINE,
                                    oldM.toString().c_str() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Orig   : %s"OSS_NEWLINE,
                                    oldO.toString().c_str() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " New id  : %s"OSS_NEWLINE,
                                    newM.toString().c_str() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " New    : %s"OSS_NEWLINE,
                                    newO.toString().c_str() ) ;
               if ( itrOldSK.valid() )
               {
                  BSONObj oldSK( itrOldSK.value() ) ;
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       " Old ShardingKey: %s"OSS_NEWLINE,
                                       oldSK.toString().c_str() ) ;
               }
               if ( itrNewSK.valid() )
               {
                  BSONObj newSK( itrNewSK.value() ) ;
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       " New ShardingKey: %s"OSS_NEWLINE,
                                       newSK.toString().c_str() ) ;
               }
            }
            catch ( std::exception &e )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s: %s"OSS_NEWLINE,
                                    "Invalid update record", e.what() ) ;
               goto done ;
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "DELETE", LOG_TYPE_DATA_DELETE ) ;
            dpsLogRecord::iterator itrFullName, itrM ;
            itrFullName = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrFullName.valid() )
            {
               PD_LOG( PDERROR, "failed to find fullname in record" ) ;
               goto done ;
            }
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrFullName.value() ) ;

            itrM = this->find( DPS_LOG_DELETE_OLDOBJ ) ;
            if ( !itrM.valid() )
            {
               PD_LOG( PDERROR, "failed to find oldobj in record" ) ;
               goto done ;
            }
            try
            {
               BSONObj objOld ( itrM.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Orig   : %s"OSS_NEWLINE,
                                    objOld.toString().c_str() ) ;
            }
            catch ( std::exception &e )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    OSS_NEWLINE
                                    "Error: Invalid delete record: %s"
                                    OSS_NEWLINE,
                                    e.what() ) ;
               goto done ;
            }
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Type   : %s(%d)"OSS_NEWLINE,
                                "POP", LOG_TYPE_DATA_POP ) ;
            dpsLogRecord::iterator itrFullName, itrLID, itrDirect ;
            itrFullName = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrFullName.valid() )
            {
               PD_LOG( PDERROR, "failed to find fullname in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " CLName : %s"OSS_NEWLINE,
                                itrFullName.value() ) ;
            itrLID = this->find( DPS_LOG_POP_LID ) ;
            if ( !itrLID.valid() )
            {
               PD_LOG( PDERROR, "failed to find pop LogicalID in record" ) ;
               goto done ;
            }

            len += ossSnprintf( outBuf + len, outSize - len,
                                " LogicalID: %u"OSS_NEWLINE,
                                *( (INT64*)itrLID.value() ) ) ;

            itrDirect = this->find( DPS_LOG_POP_DIRECTION ) ;
            if ( !itrDirect.valid() )
            {
               PD_LOG( PDERROR, "failed to find pop Direction in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Direction: %d"OSS_NEWLINE,
                                *( (INT8*)itrDirect.value() ) ) ;
            break ;
         }
         case LOG_TYPE_CS_CRT:
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CS CREATE", LOG_TYPE_CS_CRT ) ;
            dpsLogRecord::iterator itrCS, itrPageSize ;
            itrCS = this->find( DPS_LOG_CSCRT_CSNAME ) ;
            if ( !itrCS.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find csname in record" ) ;
               PD_LOG( PDERROR, "Failed to find csname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CSName : %s"OSS_NEWLINE,
                                 itrCS.value() ) ;

            itrPageSize = this->find( DPS_LOG_CSCRT_PAGESIZE ) ;
            if ( !itrPageSize.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find pagesize in record" ) ;
               PD_LOG( PDERROR, "Failed to find pagesize in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " PageSize : %d"OSS_NEWLINE,
                                 *((UINT32 *)itrPageSize.value()) ) ;
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CS DROP", LOG_TYPE_CS_DELETE ) ;
            dpsLogRecord::iterator itrCS ;
            itrCS = this->find( DPS_LOG_CSCRT_CSNAME ) ;
            if ( !itrCS.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find csname in record" ) ;
               PD_LOG( PDERROR, "Failed to find csname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CSName : %s"OSS_NEWLINE,
                                 itrCS.value() ) ;

            break ;
         }
         case LOG_TYPE_CS_RENAME:
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CS RENAME", LOG_TYPE_CS_RENAME ) ;
            dpsLogRecord::iterator itrCS, itrNewCS ;
            itrCS = this->find( DPS_LOG_CSRENAME_CSNAME ) ;
            if ( !itrCS.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find csname in record" ) ;
               PD_LOG( PDERROR, "Failed to find csname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CSName : %s"OSS_NEWLINE,
                                 itrCS.value() ) ;

            itrNewCS = this->find( DPS_LOG_CSRENAME_NEWNAME ) ;
            if ( !itrNewCS.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find new csname in record" ) ;
               PD_LOG( PDERROR, "Failed to find new csname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "New CSName : %s"OSS_NEWLINE,
                                 itrNewCS.value() ) ;

            break ;
         }
         case LOG_TYPE_CL_CRT :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CL CREATE", LOG_TYPE_CL_CRT ) ;
            dpsLogRecord::iterator itrCL =
                      this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrCL.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find clname in record" ) ;
               PD_LOG( PDERROR, "Failed to find clname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrCL.value() ) ;

            itrCL = this->find( DPS_LOG_CLCRT_ATTRIBUTE ) ;
            if ( itrCL.valid() )
            {
               UINT32 attribute = *((UINT32 *)itrCL.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Attr   : 0x%08x (%u)"OSS_NEWLINE,
                                    attribute, attribute ) ;
            }

            itrCL = this->find( DPS_LOG_CLCRT_COMPRESS_TYPE ) ;
            if ( itrCL.valid() )
            {
               UINT8 comType = *((UINT8 *)itrCL.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " ComType: 0x%02x (%u)"OSS_NEWLINE,
                                    comType, comType ) ;
            }
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CL DROP", LOG_TYPE_CL_DELETE ) ;
            dpsLogRecord::iterator itrCL =
                      this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrCL.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find clname in record" ) ;
               PD_LOG( PDERROR, "Failed to find clname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrCL.value() ) ;
            break ;

         }
         case LOG_TYPE_IX_CRT :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "IX CREATE", LOG_TYPE_IX_CRT ) ;

            dpsLogRecord::iterator itrFullName, itrIX ;
            itrFullName = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrFullName.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrFullName.value() ) ;

            itrIX = this->find( DPS_LOG_IXCRT_IX ) ;
            if ( !itrIX.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find ix in record" ) ;
               PD_LOG( PDERROR, "Failed to find ix in record" ) ;
               goto done ;
            }

            try
            {
               BSONObj obj ( itrIX.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " IXDef  : %s"OSS_NEWLINE,
                                    obj.toString().c_str() ) ;
            }
            catch ( std::exception &e )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s: %s"OSS_NEWLINE,
                                    "Invalid ixdef record", e.what() ) ;
               goto done ;
            }
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "IX DROP", LOG_TYPE_IX_DELETE ) ;

            dpsLogRecord::iterator itrFullName, itrIX ;
            itrFullName = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrFullName.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrFullName.value() ) ;

            itrIX = this->find(DPS_LOG_IXDEL_IX ) ;
            if ( !itrIX.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find ix in record" ) ;
               PD_LOG( PDERROR, "Failed to find ix in record" ) ;
               goto done ;
            }

            try
            {
               BSONObj obj ( itrIX.value() ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " IXDef  : %s"OSS_NEWLINE,
                                    obj.toString().c_str() ) ;
            }
            catch ( std::exception &e )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s: %s"OSS_NEWLINE,
                                    "Invalid ixdef record", e.what() ) ;
               goto done ;
            }
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CL RENAME", LOG_TYPE_CL_RENAME ) ;
            dpsLogRecord::iterator itrCS, itrO, itrN ;
            itrCS = this->find( DPS_LOG_CLRENAME_CSNAME ) ;
            if ( !itrCS.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find cs in record" ) ;
               PD_LOG( PDERROR, "Failed to find cs in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CSName : %s"OSS_NEWLINE,
                                 itrCS.value() ) ;

            itrO = this->find( DPS_LOG_CLRENAME_CLOLDNAME ) ;
            if ( !itrO.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find oldname in record" ) ;
               PD_LOG( PDERROR, "Failed to find oldname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Orig   : %s"OSS_NEWLINE,
                                 itrO.value() ) ;

            itrN = this->find( DPS_LOG_CLRENAME_CLNEWNAME ) ;
            if ( !itrN.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find newname in record" ) ;
               PD_LOG( PDERROR, "Failed to find newname in record" ) ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " New    : %s"OSS_NEWLINE,
                                 itrN.value() ) ;
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "CL TRUNCATE", LOG_TYPE_CL_TRUNC ) ;
            dpsLogRecord::iterator itrCL =
                                       this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrCL.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record") ;
               goto done ;
            }

            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " CLName : %s"OSS_NEWLINE,
                                 itrCL.value() ) ;
            break ;
         }
         case LOG_TYPE_INVALIDATE_CATA :
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type    : %s(%d)"OSS_NEWLINE,
                                 "INVALIDATE CATA", LOG_TYPE_INVALIDATE_CATA ) ;
            dpsLogRecord::iterator itrType, itrCL, itrIX ;
            UINT8 invType = 0 ;
            itrType = this->find( DPS_LOG_INVALIDCATA_TYPE ) ;
            if ( itrType.valid() )
            {
               invType = *( ( UINT8 * )( itrType.value() ) ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " InvType : 0x%02x (%u)"OSS_NEWLINE,
                                    invType, invType ) ;
            }

            itrCL = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrCL.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR*  : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record") ;
               goto done ;
            }
            else
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " Name    : %s"OSS_NEWLINE,
                                    itrCL.value() ) ;
            }

            itrIX = this->find( DPS_LOG_INVALIDCATA_IXNAME ) ;
            if ( itrIX.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " IXName  : %s"OSS_NEWLINE,
                                    itrIX.value() ) ;
            }
            break ;
         }
         case LOG_TYPE_TS_COMMIT:
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "COMMIT", LOG_TYPE_TS_COMMIT ) ;
             if ( !itrTransID.valid() )
             {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find transid in record" ) ;
                PD_LOG( PDERROR, "Failed to find transid in record" ) ;
                goto done ;
             }
             break ;
         }
         case LOG_TYPE_TS_ROLLBACK:
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s(%d)"OSS_NEWLINE,
                                 "ROLLBACK", LOG_TYPE_TS_ROLLBACK ) ;
             if ( !itrTransID.valid() )
             {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find transid in record" ) ;
                PD_LOG( PDERROR, "Failed to find transid in record" ) ;
                goto done ;
             }
             break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Type   : %s(%d)"OSS_NEWLINE,
                                 "LOB_W", LOG_TYPE_LOB_WRITE ) ;

            dpsLogRecord::iterator itr ;
            itr = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " FullName : %s"OSS_NEWLINE,
                                 itr.value() ) ;

            itr = this->find( DPS_LOG_LOB_OID ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find oid in record" ) ;
               PD_LOG( PDERROR, "Failed to find oid in record" ) ;
               goto done ;
            }

            {
            bson::OID *oid = ( bson::OID * )( itr.value() ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Oid    : %s"OSS_NEWLINE,
                                 oid->str().c_str() ) ;
            }

            itr = this->find( DPS_LOG_LOB_SEQUENCE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find sequence in record" ) ;
               PD_LOG( PDERROR, "Failed to find sequence in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Sequence : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_OFFSET ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find offset in record" ) ;
               PD_LOG( PDERROR, "Failed to find offset in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Offset : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_LEN ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find len in record" ) ;
               PD_LOG( PDERROR, "Failed to find len in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Len    : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find page in record" ) ;
               PD_LOG( PDERROR, "Failed to find page in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Page   : %d"OSS_NEWLINE,
                                *( ( SINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE_SIZE ) ;
            if ( itr.valid() )
            {
               len += ossSnprintf( outBuf + len, outSize - len,
                                " PageSize : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;
            }

            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Type   : %s(%d)"OSS_NEWLINE,
                                 "LOB_REMOVE", LOG_TYPE_LOB_REMOVE ) ;

            dpsLogRecord::iterator itr ;
            itr = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " FullName : %s"OSS_NEWLINE,
                                 itr.value() ) ;

            itr = this->find( DPS_LOG_LOB_OID ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find oid in record" ) ;
               PD_LOG( PDERROR, "Failed to find oid in record" ) ;
               goto done ;
            }

            {
            bson::OID *oid = ( bson::OID * )( itr.value() ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Oid    : %s"OSS_NEWLINE,
                                 oid->str().c_str() ) ;
            }

            itr = this->find( DPS_LOG_LOB_SEQUENCE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find sequence in record" ) ;
               PD_LOG( PDERROR, "Failed to find sequence in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Sequence : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_OFFSET ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find offset in record" ) ;
               PD_LOG( PDERROR, "Failed to find offset in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Offset : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_LEN ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find len in record" ) ;
               PD_LOG( PDERROR, "Failed to find len in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Len    : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find page in record" ) ;
               PD_LOG( PDERROR, "Failed to find page in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Page   : %d"OSS_NEWLINE,
                                *( ( SINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE_SIZE ) ;
            if ( itr.valid() )
            {
               len += ossSnprintf( outBuf + len, outSize - len,
                                " PageSize : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;
            }

            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Type   : %s(%d)"OSS_NEWLINE,
                                 "LOB_U", LOG_TYPE_LOB_UPDATE ) ;

            dpsLogRecord::iterator itr ;
            itr = this->find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find fullname in record" ) ;
               PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
               goto done ;
            }
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " FullName : %s"OSS_NEWLINE,
                                 itr.value() ) ;

            itr = this->find( DPS_LOG_LOB_OID ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find oid in record" ) ;
               PD_LOG( PDERROR, "Failed to find oid in record" ) ;
               goto done ;
            }

            {
            bson::OID *oid = ( bson::OID * )( itr.value() ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Oid    : %s"OSS_NEWLINE,
                                 oid->str().c_str() ) ;
            }

            itr = this->find( DPS_LOG_LOB_SEQUENCE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find sequence in record" ) ;
               PD_LOG( PDERROR, "Failed to find sequence in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Sequence : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_OFFSET ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find offset in record" ) ;
               PD_LOG( PDERROR, "Failed to find offset in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Offset : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_LEN ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find len in record" ) ;
               PD_LOG( PDERROR, "Failed to find len in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Len    : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_OLD_LEN ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find old len in record" ) ;
               PD_LOG( PDERROR, "Failed to find old len in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Old Len: %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE ) ;
            if ( !itr.valid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "*ERROR* : %s"OSS_NEWLINE,
                                    "Failed to find page in record" ) ;
               PD_LOG( PDERROR, "Failed to find page in record" ) ;
               goto done ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                " Page   : %d"OSS_NEWLINE,
                                *( ( SINT32 * )( itr.value() ) ) ) ;

            itr = this->find( DPS_LOG_LOB_PAGE_SIZE ) ;
            if ( itr.valid() )
            {
               len += ossSnprintf( outBuf + len, outSize - len,
                                " PageSize : %d"OSS_NEWLINE,
                                *( ( UINT32 * )( itr.value() ) ) ) ;
            }

            break ;
         }
         default:
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " Type   : %s"OSS_NEWLINE,
                                 "UNKNOWN" ) ;
            break ;
         }
         }

         if ( itrTransID.valid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " TransID : 0x%08lx"OSS_NEWLINE,
                                 *((DPS_TRANS_ID *)itrTransID.value()) ) ;
         }
         if ( itrTransLsn.valid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " TransPreLSN : 0x%08lx"OSS_NEWLINE,
                                 *((DPS_TRANS_ID *)itrTransLsn.value()) ) ;
         }
         if ( itrTransRel.valid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 " TransRelatedLSN : 0x%08lx"OSS_NEWLINE,
                                 *((DPS_TRANS_ID *)itrTransRel.value()) ) ;
         }
      }

      if ( SDB_OK != _result )
      {
         len += ossSnprintf( outBuf + len, outSize - len,
                             OSS_NEWLINE"*ERROR* : %d(%s)"OSS_NEWLINE,
                             _result, getErrDesp( _result ) ) ;
      }

   done:
      PD_TRACE1 ( SDB__DPSLGRECD_DUMP, PD_PACK_UINT(len) );
      PD_TRACE_EXIT ( SDB__DPSLGRECD_DUMP );
      return len ;
   }
}
