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

   Source File Name = dpsDef.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSDEF_HPP_
#define DPSDEF_HPP_

#include "ossTypes.h"

#if defined (_WINDOWS)
#define DPS_INVALID_LSN_OFFSET   0xFFFFFFFFFFFFFFFFLL
#elif defined (_LINUX)
#define DPS_INVALID_LSN_OFFSET   0xFFFFFFFFFFFFFFFFll
#endif
#define DPS_MERGE_BLOCK_MAX_DATA    20
#define DPS_INVALID_LSN_VERSION     0

#define DPS_DEFAULT_PAGE_SIZE    ( 64 * 1024 )
#define DPS_LOG_HEAD_LEN         ( 64 * 1024 )

#define DPS_DMP_OPT_HEX            0x00000001
#define DPS_DMP_OPT_HEX_WITH_ASCII 0x00000002
#define DPS_DMP_OPT_FORMATTED      0x00000004
#define DPS_INVALID_TRANS_ID       0

#define DPS_INVALID_TAG    0
#define DPS_DUMMY_TAG      256
#define DPS_TAG            UINT8

#define DPS_LOG_FILE_SIZE_UNIT         (1024 * 1024)

#define DPS_LOG_INVALIDCATA_TYPE_ALL   ( 0xff )
#define DPS_LOG_INVALIDCATA_TYPE_CATA  ( 0x01 )
#define DPS_LOG_INVALIDCATA_TYPE_STAT  ( 0x02 )
#define DPS_LOG_INVALIDCATA_TYPE_PLAN  ( 0x04 )
#define DPS_LOG_INVALIDCATA_TYPE_TRANS ( 0x08 )

typedef UINT64 DPS_TRANS_ID ;
typedef UINT64 DPS_LSN_OFFSET ;
typedef UINT32 DPS_LSN_VER ;

#define DPS_TRANSID_ROLLBACKTAG_BIT          0X0000800000000000ll
#define DPS_TRANSID_FIRSTOP_BIT              0X0000400000000000ll
#define DPS_TRANSID_AUTOCOMMIT_BIT           0X0000200000000000ll
#define DPS_TRANSID_RBPENDING_BIT            0X0000100000000000ll
#define DPS_TRANSID_SN_BIT                   0X000000FFFFFFFFFFll
#define DPS_TRANSID_VALID_BIT                0XFFFF00FFFFFFFFFFll

#define DPS_TRANS_CLEAR_FIRSTOP( id )  \
   OSS_BIT_CLEAR( id, DPS_TRANSID_FIRSTOP_BIT )

#define DPS_TRANS_IS_FIRSTOP( id )  \
   OSS_BIT_TEST( id, DPS_TRANSID_FIRSTOP_BIT )

#define DPS_TRANS_SET_FIRSTOP( id )  \
   OSS_BIT_SET( id, DPS_TRANSID_FIRSTOP_BIT )

#define DPS_TRANS_IS_AUTOCOMMIT( id ) \
   OSS_BIT_TEST( id, DPS_TRANSID_AUTOCOMMIT_BIT )

#define DPS_TRANS_SET_AUTOCOMMIT( id ) \
   OSS_BIT_SET( id, DPS_TRANSID_AUTOCOMMIT_BIT )

#define DPS_TRANS_CLEAR_AUTOCOMMIT( id ) \
   OSS_BIT_CLEAR( id, DPS_TRANSID_AUTOCOMMIT_BIT )

#define DPS_TRANS_IS_ROLLBACK( id ) \
   OSS_BIT_TEST( id, DPS_TRANSID_ROLLBACKTAG_BIT )

#define DPS_TRANS_SET_ROLLBACK( id ) \
   OSS_BIT_SET( id, DPS_TRANSID_ROLLBACKTAG_BIT )

#define DPS_TRANS_IS_RBPENDING( id ) \
   OSS_BIT_TEST( id, DPS_TRANSID_RBPENDING_BIT )

#define DPS_TRANS_SET_RBPENDING( id ) \
   OSS_BIT_SET( id, DPS_TRANSID_RBPENDING_BIT )

#define DPS_TRANS_CLEAR_RBPENDING( id ) \
   OSS_BIT_CLEAR( id, DPS_TRANSID_RBPENDING_BIT )


#define DPS_TRANS_GET_NODEID( id )           ( (DPS_TRANS_ID)(id) >> 48 )
#define DPS_TRANS_GET_SN( id )               ( (id) & DPS_TRANSID_SN_BIT )
#define DPS_TRANS_GET_ID( id )               ( (id) & DPS_TRANSID_VALID_BIT )

enum DPS_LOG_TYPE
{
   LOG_TYPE_DUMMY        = 0x00,
   LOG_TYPE_DATA_INSERT  = 0x01,
   LOG_TYPE_DATA_UPDATE  = 0x02,
   LOG_TYPE_DATA_DELETE  = 0x03,
   LOG_TYPE_CS_CRT       = 0x04,
   LOG_TYPE_CS_DELETE    = 0x05,
   LOG_TYPE_CL_CRT       = 0x06,
   LOG_TYPE_CL_DELETE    = 0x07,
   LOG_TYPE_IX_CRT       = 0x08,
   LOG_TYPE_IX_DELETE    = 0x09,
   LOG_TYPE_CL_RENAME    = 0x0A,
   LOG_TYPE_CL_TRUNC     = 0x0B,
   LOG_TYPE_TS_COMMIT    = 0x0C,
   LOG_TYPE_TS_ROLLBACK  = 0x0D,
   LOG_TYPE_INVALIDATE_CATA   = 0x0E,
   LOG_TYPE_LOB_WRITE    = 0x0F,
   LOG_TYPE_LOB_REMOVE   = 0x10,
   LOG_TYPE_LOB_UPDATE   = 0x11,
   LOG_TYPE_LOB_TRUNCATE = 0x12,
   LOG_TYPE_CS_RENAME    = 0x13,
   LOG_TYPE_DATA_POP     = 0x14,
   LOG_TYPE_ALTER        = 0x15,
   LOG_TYPE_ADDUNIQUEID  = 0x16
} ;

enum DPS_MOMENT
{
   DPS_BEFORE = 0,
   DPS_AFTER  = 1,
} ;

namespace engine
{
   class _pmdEDUCB ;
   /*
      _dpsEventHandler define
   */
   class _dpsEventHandler
   {
      public:
         _dpsEventHandler () {}
         virtual ~_dpsEventHandler () {}

         virtual INT32 canAssignLogPage( UINT32 reqLen, _pmdEDUCB *cb ) = 0 ;

         virtual void  onPrepareLog( UINT32 csLID, UINT32 clLID,
                                     INT32 extLID, DPS_LSN_OFFSET offset ) = 0 ;

         virtual void  onWriteLog( DPS_LSN_OFFSET offset ) = 0 ;

         virtual INT32 onCompleteOpr( _pmdEDUCB *cb, INT32 w ) = 0 ;

         virtual void  onSwitchLogFile( UINT32 preLogicalFileId,
                                        UINT32 preFileId,
                                        UINT32 curLogicalFileId,
                                        UINT32 curFileId ) = 0 ;

         // moment is callback time,
         // it's value can be DPS_BEFORE/DPS_AFTER
         virtual void  onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                  DPS_LSN_VER moveToVersion,
                                  DPS_LSN_OFFSET expectOffset,
                                  DPS_LSN_VER expectVersion,
                                  DPS_MOMENT moment,
                                  INT32 errcode ) = 0 ;
   } ;
   typedef _dpsEventHandler dpsEventHandler ;

}

#endif // DPSDEF_HPP_

