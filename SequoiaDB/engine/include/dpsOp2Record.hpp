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

   Source File Name = dpsOp2Record.hpp

   Descriptive Name =

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

#ifndef DPSOP2RECORD_HPP_
#define DPSOP2RECORD_HPP_

#include "dpsLogRecord.hpp"
#include "../bson/bson.h"
#include "dmsLobDef.hpp"
#include "utilCompressor.hpp"

using namespace bson ;

namespace engine
{
   INT32 dpsInsert2Record( const CHAR *fullName,
                           const BSONObj &obj,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           dpsLogRecord &record ) ;

   INT32 dpsRecord2Insert( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &obj ) ;

   INT32 dpsUpdate2Record( const CHAR *fullName,
                           const BSONObj &oldMatch,
                           const BSONObj &oldObj,
                           const BSONObj &newMatch,
                           const BSONObj &newObj,
                           const BSONObj &oldShardingKey,
                           const BSONObj &newShardingKey,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           dpsLogRecord &record ) ;

   INT32 dpsRecord2Update( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &oldMatch,
                           BSONObj &oldObj,
                           BSONObj &newMatch,
                           BSONObj &newObj,
                           BSONObj *oldShardingKey = NULL,
                           BSONObj *newShardingKey = NULL ) ;

   INT32 dpsDelete2Record( const CHAR *fullName,
                           const BSONObj &oldObj,
                           const DPS_TRANS_ID &transID,
                           const DPS_LSN_OFFSET &preTransLsn,
                           const DPS_LSN_OFFSET &relatedLSN,
                           dpsLogRecord &record ) ;

   INT32 dpsRecord2Delete( const CHAR *logRecord,
                           const CHAR **fullName,
                           BSONObj &oldObj ) ;

   INT32 dpsPop2Record( const CHAR *fullName,
                        const dmsRecordID &firstRID,
                        const INT64 &logicalID,
                        const INT8 &direction,
                        dpsLogRecord &record ) ;

   INT32 dpsRecord2Pop( const CHAR *logRecord,
                        const CHAR **fullName,
                        INT64 &logicalID,
                        INT8 &direction ) ;

   INT32 dpsCSCrt2Record( const CHAR *csName,
                          const INT32 &pageSize,
                          const INT32 &lobPageSize,
                          const INT32 &type,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2CSCrt( const CHAR *logRecord,
                          const CHAR **csName,
                          INT32 &pageSize,
                          INT32 &lobPageSize,
                          INT32 &type ) ;

   INT32 dpsCSDel2Record( const CHAR *csName,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2CSDel( const CHAR *logRecord,
                          const CHAR **csName ) ;

   INT32 dpsCSRename2Record( const CHAR *csName,
                             const CHAR *newCSName,
                             dpsLogRecord &record ) ;

   INT32 dpsRecord2CSRename( const CHAR *logRecord,
                             const CHAR **csName,
                             const CHAR **newCSName ) ;

   INT32 dpsCLCrt2Record( const CHAR *fullName,
                          const UINT32 &attribute,
                          const UINT8 &compressorType,
                          const BSONObj *extOptions,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2CLCrt( const CHAR *logRecord,
                          const CHAR **fullName,
                          UINT32 &attribute,
                          UINT8 &compressorType,
                          BSONObj &extOptions ) ;

   INT32 dpsCLDel2Record( const CHAR *fullName,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2CLDel( const CHAR *logRecord,
                          const CHAR **fullName ) ;

   INT32 dpsIXCrt2Record( const CHAR *fullName,
                          const BSONObj &index,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2IXCrt( const CHAR *logRecord,
                          const CHAR **fullName,
                          BSONObj &index ) ;

   INT32 dpsIXDel2Record( const CHAR *fullName,
                          const BSONObj &index,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2IXDel( const CHAR *logRecord,
                          const CHAR **fullName,
                          BSONObj &index ) ;

   INT32 dpsCLRename2Record( const CHAR *csName,
                             const CHAR *clOldName,
                             const CHAR *clNewName,
                             dpsLogRecord &record ) ;

   INT32 dpsRecord2CLRename( const CHAR *logRecord,
                             const CHAR **csName,
                             const CHAR **clOldName,
                             const CHAR **clNewName ) ;

   INT32 dpsCLTrunc2Record( const CHAR *fullName,
                            dpsLogRecord &record ) ;

   INT32 dpsRecord2CLTrunc( const CHAR *logRecord,
                            const CHAR **fullName ) ;

   INT32 dpsTransCommit2Record( const DPS_TRANS_ID &transID,
                                const DPS_LSN_OFFSET &preTransLsn,
                                const DPS_LSN_OFFSET &firstTransLsn,
                                dpsLogRecord &record ) ;

   INT32 dpsRecord2TransCommit( const CHAR *logRecord,
                                DPS_TRANS_ID &transID ) ;


   INT32 dpsInvalidCata2Record( const UINT8 &type,
                                const CHAR * clFullName,
                                const CHAR * ixName,
                                dpsLogRecord &record ) ;

   INT32 dpsRecord2InvalidCata( const CHAR *logRecord,
                                UINT8 &type,
                                const CHAR **clFullName,
                                const CHAR **ixName ) ;

   INT32 dpsLobW2Record( const CHAR *fullName,
                         const bson::OID *oid,
                         const UINT32 &sequence,
                         const UINT32 &offset,
                         const UINT32 &hash,
                         const UINT32 &len,
                         const CHAR *data,
                         const UINT32 &pageSize,
                         const DMS_LOB_PAGEID &pageID,
                         const DPS_TRANS_ID &transID,
                         const DPS_LSN_OFFSET &preTransLsn,
                         const DPS_LSN_OFFSET &relatedLSN,
                         dpsLogRecord &record ) ;

   INT32 dpsRecord2LobW( const CHAR *raw,
                         const CHAR **fullName,
                         const bson::OID **oid,
                         UINT32 &sequence,
                         UINT32 &offset,
                         UINT32 &len,
                         UINT32 &hash,
                         const CHAR **data,
                         DMS_LOB_PAGEID &pageID,
                         UINT32* pageSize = NULL ) ;

   INT32 dpsLobU2Record(  const CHAR *fullName,
                          const bson::OID *oid,
                          const UINT32 &sequence,
                          const UINT32 &offset,
                          const UINT32 &hash,
                          const UINT32 &len,
                          const CHAR *data,
                          const UINT32 &oldLen,
                          const CHAR *oldData,
                          const UINT32 &pageSize,
                          const DMS_LOB_PAGEID &pageID,
                          const DPS_TRANS_ID &transID,
                          const DPS_LSN_OFFSET &preTransLsn,
                          const DPS_LSN_OFFSET &relatedLSN,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2LobU( const CHAR *raw,
                         const CHAR **fullName,
                         const bson::OID **oid,
                         UINT32 &sequence,
                         UINT32 &offset,
                         UINT32 &len,
                         UINT32 &hash,
                         const CHAR **data,
                         UINT32 &oldLen,
                         const CHAR **oldData,
                         DMS_LOB_PAGEID &pageID,
                         UINT32* pageSize = NULL ) ;

   INT32 dpsLobRm2Record( const CHAR *fullName,
                          const bson::OID *oid,
                          const UINT32 &sequence,
                          const UINT32 &offset,
                          const UINT32 &hash,
                          const UINT32 &len,
                          const CHAR *data,
                          const UINT32 &pageSize,
                          const DMS_LOB_PAGEID &page,
                          const DPS_TRANS_ID &transID,
                          const DPS_LSN_OFFSET &preTransLsn,
                          const DPS_LSN_OFFSET &relatedLSN,
                          dpsLogRecord &record ) ;

   INT32 dpsRecord2LobRm( const CHAR *raw,
                          const CHAR **fullName,
                          const bson::OID **oid,
                          UINT32 &sequence,
                          UINT32 &offset,
                          UINT32 &len,
                          UINT32 &hash,
                          const CHAR **data,
                          DMS_LOB_PAGEID &page,
                          UINT32* pageSize = NULL ) ;

   INT32 dpsLobTruncate2Record( const CHAR *fullName,
                                dpsLogRecord &record ) ;

   INT32 dpsRecord2LobTruncate( const CHAR *raw,
                                const CHAR **fullName ) ;

}

#endif

