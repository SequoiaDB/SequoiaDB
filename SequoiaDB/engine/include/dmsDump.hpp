/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmsDump.hpp

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
#ifndef DMSDUMP_HPP__
#define DMSDUMP_HPP__

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

#include <set>

using namespace bson ;
using namespace std ;

namespace engine
{

   enum
   {
       TITLE1 = 0,
       TITLE2 =1,
       TITLE3 = 3,
       TITLE4 = 5,
       TITLE5 = 7
   };
   #define DMS_SU_DMP_OPT_HEX                ((UINT32)(0x00000001))
   #define DMS_SU_DMP_OPT_HEX_WITH_ASCII     ((UINT32)(0x00000002))
   #define DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR ((UINT32)(0x00000004))
   #define DMS_SU_DMP_OPT_FORMATTED          ((UINT32)(0x00000008))

   class _pmdEDUCB ;

   /*
      _dmsDump define
   */
   class _dmsDump : public SDBObject
   {
      public:
         _dmsDump () {}
         ~_dmsDump () {}

      public:
         static UINT32 dumpHeader ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    CHAR * addrPrefix,
                                    UINT32 options,
                                    UINT32 &pageSize,
                                    UINT32 &pageNum ) ;

         static UINT32 dumpSME ( void * inBuf,
                                 UINT32 inSize,
                                 CHAR * outBuf,
                                 UINT32 outSize,
                                 UINT32 pageNum,
                                 UINT32 &usedPages,
                                 SINT32 &hwmPages ) ;

         static UINT32 dumpMME ( void * inBuf,
                                 UINT32 inSize,
                                 CHAR * outBuf,
                                 UINT32 outSize,
                                 CHAR * addrPrefix,
                                 UINT32 options,
                                 const CHAR *collectionName,
                                 std::vector<UINT16> &collections,
                                 BOOLEAN force ) ;

         static UINT32 dumpMB( void * inBuf,
                               UINT32 inSize,
                               CHAR * outBuf,
                               UINT32 outSize,
                               CHAR * addrPrefix,
                               UINT32 options,
                               const CHAR *collectionName,
                               std::vector<UINT16> &collections,
                               BOOLEAN force ) ;

         static UINT32 dumpMBEx( void * inBuf,
                                 UINT32 inSize,
                                 CHAR * outBuf,
                                 UINT32 outSize,
                                 CHAR * addrPrefix,
                                 UINT32 options,
                                 dmsExtentID extID ) ;

         static UINT32 dumpDictExtent( void * inBuf,
                                       UINT32 inSize,
                                       CHAR * outBuf,
                                       UINT32 outSize,
                                       CHAR * addrPrefix,
                                       UINT32 options,
                                       dmsExtentID extID ) ;

         static UINT32 dumpExtOptExtent( CHAR * inBuf,
                                         UINT32 inSize,
                                         CHAR * outBuf,
                                         UINT32 outSize,
                                         CHAR * addrPrefix,
                                         UINT32 options,
                                         dmsExtentID extID,
                                         DMS_STORAGE_TYPE type ) ;

         static UINT32 dumpRawPage ( void * inBuf,
                                     UINT32 inSize,
                                     CHAR * outBuf,
                                     UINT32 outSize ) ;

         static UINT32 dumpDataExtent ( _pmdEDUCB *cb,
                                        CHAR * inBuf,
                                        UINT32 inSize,
                                        CHAR * outBuf,
                                        UINT32 outSize,
                                        CHAR * addrPrefix,
                                        UINT32 options,
                                        dmsExtentID &nextExtent,
                                        dmsCompressorEntry *compressorEntry,
                                        set<dmsRecordID> *ridList = NULL,
                                        BOOLEAN dumpRecord = FALSE,
                                        BOOLEAN capped = FALSE ) ;

         static UINT32 dumpExtentHeader ( void * inBuf,
                                          UINT32 inSize,
                                          CHAR * outBuf,
                                          UINT32 outSize ) ;

         static UINT32 dumpDataExtentHeader ( void * inBuf,
                                              UINT32 inSize,
                                              CHAR * outBuf,
                                              UINT32 outSize ) ;

         static UINT32 dumpMetaExtentHeader( void * inBuf,
                                             UINT32 inSize,
                                             CHAR * outBuf,
                                             UINT32 outSize ) ;
         static UINT32 dumpDictExtentHeader( void *inBuf,
                                             UINT32 inSize,
                                             CHAR * outBuf,
                                             UINT32 outSize ) ;

         static UINT32 dumpExtOptExtentHeader( void *inBuf,
                                               UINT32 inSize,
                                               CHAR * outBuf,
                                               UINT32 outSize ) ;

         static UINT32 dumpDataRecord ( pmdEDUCB *cb,
                                        CHAR * inBuf,
                                        UINT32 inSize,
                                        CHAR * outBuf,
                                        UINT32 outSize,
                                        dmsOffset &nextRecord,
                                        dmsCompressorEntry *compressorEntry,
                                        set<dmsRecordID> *ridList = NULL ) ;

         static UINT32 dumpCappedDataRecord( pmdEDUCB *cb,
                                             dmsCappedRecord *record,
                                             CHAR *outBuf,
                                             UINT32 outSize,
                                             dmsCompressorEntry *compressorEntry ) ;

         static UINT32 dumpIndexExtent ( void * inBuf,
                                         UINT32 inSize,
                                         CHAR * outBuf,
                                         UINT32 outSize,
                                         CHAR * addrPrefix,
                                         UINT32 options,
                                         set<dmsExtentID> &childExtents,
                                         BOOLEAN dumpIndexKey = FALSE ) ;

         static UINT32 dumpIndexExtentHeader ( void * inBuf,
                                               UINT32 inSize,
                                               CHAR * outBuf,
                                               UINT32 outSize ) ;

         static UINT32 dumpIndexRecord ( void * inBuf,
                                         UINT32 inSize,
                                         CHAR * outBuf,
                                         UINT32 outSize,
                                         UINT32 keyOffset ) ;

         static UINT32 dumpIndexCBExtentHeader ( void * inBuf,
                                                 UINT32 inSize,
                                                 CHAR * outBuf,
                                                 UINT32 outSize ) ;

         static UINT32 dumpIndexCBExtent (  void * inBuf,
                                            UINT32 inSize,
                                            CHAR * outBuf,
                                            UINT32 outSize,
                                            CHAR * addrPrefix,
                                            UINT32 options,
                                            dmsExtentID &root ) ;

         static UINT32 dumpDmsLobMeta( CHAR *inBuf,
                                        UINT32 inSize,
                                        CHAR * outBuf,
                                        UINT32 outSize,
                                        CHAR * addrPrefix,
                                        UINT32 options);

         static UINT32 dumpDmsLobData( CHAR *inBuf,
                                       UINT32 inSize,
                                       CHAR * outBuf,
                                       UINT32 outSize,
                                       CHAR * addrPrefix,
                                       UINT32 options );

         static UINT32 dumpDmsLobDataMapBlk( UINT32 pageID,
                                             dmsLobDataMapBlk *blk,
                                             CHAR * outBuf,
                                             UINT32 outSize,
                                             CHAR * addrPrefix,
                                             UINT32 options,
                                             UINT32 pageSize,
                                             UTIL_LOB_HASH_TYPE hashType );
      private:
         static UINT32 _dumpExtentHeaderComm( const dmsExtent *extent,
                                              CHAR *outBuf, UINT32 outSize ) ;

         static UINT32 _dumpDictDetail( void *inBuf, UINT32 inSize,
                                        CHAR *outBuf, UINT32 outSize ) ;

         static UINT32 _dumpExtOptionDetail( CHAR *inBuf, UINT32 inSize,
                                             CHAR *outBuf, UINT32 outSize,
                                             DMS_STORAGE_TYPE type ) ;

         static UINT32 _dumpNormalExtent( CHAR *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize,
                                          dmsCompressorEntry *compressorEntry,
                                          set< dmsRecordID > *ridList,
                                          pmdEDUCB *cb ) ;

         static UINT32 _dumpCappedExtent( CHAR *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize,
                                          dmsCompressorEntry *compressorEntry,
                                          pmdEDUCB *cb ) ;
   } ;
   typedef _dmsDump dmsDump ;

}

#endif //DMSDUMP_HPP__

