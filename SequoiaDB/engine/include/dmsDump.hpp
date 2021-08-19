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

#include <deque>

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

   /*
      Tool functions
   */
   string getIndexTypeDesp ( UINT16 type ) ;

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
                                 UINT32 pageNum ) ;

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
                                         deque<dmsExtentID> &childExtents,
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
                                        UINT32 options);

         static UINT32 dumpDmsLobDataMapBlk( dmsLobDataMapBlk *blk,
                                        CHAR * outBuf,
                                        UINT32 outSize, 
                                        CHAR * addrPrefix,
                                        UINT32 options, 
                                        UINT32 pageSize);
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

