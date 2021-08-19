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

   Source File Name = dmsInspect.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSINSPECT_HPP__
#define DMSINSPECT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsRecord.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"

#include <deque>

using namespace bson ;
using namespace std ;

namespace engine
{

   class _pmdEDUCB ;

   /*
      _dmsInspect define
   */
   class _dmsInspect : public SDBObject
   {
      public:
         _dmsInspect () {}
         ~_dmsInspect () {}

      public:

         static UINT32 inspectHeader ( void * inBuf,
                                       UINT32 inSize,
                                       CHAR * outBuf,
                                       UINT32 outSize,
                                       UINT32 &pageSize,
                                       UINT32 &pageNum,
                                       UINT32 &segmentSize,
                                       UINT64 &secretValue,
                                       SINT32 &err ) ;

         static UINT32 inspectSME ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    const CHAR *expBuffer,
                                    UINT32 pageNum,
                                    SINT32 &hwmPages,
                                    SINT32 &err ) ;

         static UINT32 inspectMME ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    const CHAR *pCollectionName,
                                    INT32 maxPages,
                                    vector<UINT16> &collections,
                                    SINT32 &err ) ;

         static UINT32 inspectMB ( void * inBuf,
                                   UINT32 inSize,
                                   CHAR * outBuf,
                                   UINT32 outSize,
                                   const CHAR *pCollectionName,
                                   INT32 expCollectionID,
                                   INT32 maxPages,
                                   vector<UINT16> &collections,
                                   SINT32 &err ) ;

         static UINT32 inspectDataExtent ( _pmdEDUCB *cb,
                                           CHAR * inBuf,
                                           UINT32 inSize,
                                           CHAR * outBuf,
                                           UINT32 outSize,
                                           INT32 maxPages,
                                           UINT16 collectionID,
                                           dmsExtentID &nextExtent,
                                           set<dmsRecordID> *ridList,
                                           SINT32 &err,
                                           dmsCompressorEntry *compressorEntry,
                                           UINT64 &recordNum,
                                           UINT64 &compressedNum,
                                           BOOLEAN capped = FALSE ) ;

         static UINT32 inspectDataRecord ( _pmdEDUCB *cb,
                                           void * inBuf,
                                           UINT32 inSize,
                                           CHAR * outBuf,
                                           UINT32 outSize,
                                           INT32 currentRecordID,
                                           dmsOffset &nextRecord,
                                           set<dmsRecordID> *ridList,
                                           SINT32 &err,
                                           dmsCompressorEntry *compressorEntry,
                                           BOOLEAN &isCompressed ) ;

         static UINT32 inspectCappedDataRecord( pmdEDUCB *cb,
                                                dmsCappedRecord *record,
                                                CHAR *outBuf,
                                                UINT32 outSize,
                                                dmsOffset currentOffset,
                                                SINT32 &err,
                                                dmsCompressorEntry *compressorEntry ) ;

         static UINT32 inspectExtentHeader ( void * inBuf,
                                             UINT32 inSize,
                                             CHAR * outBuf,
                                             UINT32 outSize,
                                             UINT16 collectionID,
                                             SINT32 &err ) ;

         static UINT32 inspectDataExtentHeader ( void * inBuf,
                                                 UINT32 inSize,
                                                 CHAR * outBuf,
                                                 UINT32 outSize,
                                                 UINT16 collectionID,
                                                 SINT32 &err ) ;

         static UINT32 inspectIndexRecord ( void * inBuf,
                                            UINT32 inSize,
                                            CHAR * outBuf,
                                            UINT32 outSize,
                                            UINT32 keyOffset,
                                            SINT32 &err ) ;

         static UINT32 inspectIndexExtentHeader ( void * inBuf,
                                                  UINT32 inSize,
                                                  CHAR * outBuf,
                                                  UINT32 outSize,
                                                  UINT16 collectionID,
                                                  dmsExtentID &parentExtent,
                                                  SINT32 &err ) ;

         static UINT32 inspectIndexCBExtentHeader ( void * inBuf,
                                                    UINT32 inSize,
                                                    CHAR * outBuf,
                                                    UINT32 outSize,
                                                    UINT16 collectionID,
                                                    SINT32 &err ) ;

         static UINT32 inspectIndexCBExtent (  void * inBuf,
                                               UINT32 inSize,
                                               CHAR * outBuf,
                                               UINT32 outSize,
                                               UINT16 collectionID,
                                               dmsExtentID &root,
                                               SINT32 &err ) ;

         static UINT32 inspectIndexExtent ( _pmdEDUCB *cb,
                                            void * inBuf,
                                            UINT32 inSize,
                                            CHAR * outBuf,
                                            UINT32 outSize,
                                            UINT16 collectionID,
                                            dmsExtentID extentID,
                                            deque<dmsExtentID> &childExtents,
                                            SINT32 &err ) ;

         static INT32 inspectNormalExtent( CHAR *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize,
                                           UINT16 collectionID,
                                           dmsCompressorEntry *compressorEntry,
                                           UINT64 &recordNum,
                                           UINT64 &compressedNum,
                                           INT32 &localErr,
                                           set< dmsRecordID > *ridList,
                                           pmdEDUCB *cb ) ;

         static INT32 inspectCappedExtent( CHAR *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize,
                                           UINT16 collectionID,
                                           dmsCompressorEntry *compressorEntry,
                                           UINT64 &recordNum,
                                           UINT64 &compressedNum,
                                           INT32 &localErr,
                                           pmdEDUCB *cb ) ;

         static UINT32 inspectLobmHeader( void *inBuf, UINT32 inSize,
                                          CHAR *outBuf, 
                                          UINT32 outSize, 
                                          UINT32 sequence,
                                          UINT32 &pageNum,
                                          UINT32 &lobmPageSize,
                                          UINT64 secretValue,
                                          INT64 fileSize, 
                                          INT32 &totalErr);

         static UINT32 inspectLobdHeader( void *inBuf, UINT32 inSize,
                                          CHAR *outBuf,
                                          UINT32 outSize, 
                                          UINT32 sequence,
                                          UINT64 secretValue,
                                          INT64 fileSize, 
                                          INT32 &totalErr);

         static UINT32 inspectDmsLobMeta( dmsLobMeta *lobMeta,
                                          CHAR * outBuf, 
                                          UINT32 outSize, 
                                          SINT32 &err);

         static UINT32 inspectDmsLobDataMapBlk( dmsLobDataMapBlk *blk, 
                                                CHAR * outBuf, 
                                                UINT32 outSize, 
                                                UINT16 clId, 
                                                SINT32 &err ) ;
                
   } ;
   typedef _dmsInspect dmsInspect ;


}

#endif //DMSINSPECT_HPP__

