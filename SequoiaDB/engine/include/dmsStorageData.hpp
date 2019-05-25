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

   Source File Name = dmsStorageData.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATA_HPP_
#define DMSSTORAGE_DATA_HPP_

#include "dmsStorageDataCommon.hpp"
#include "dmsEventHandler.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _mthModifier ;

   class _dmsStorageData : public _dmsStorageDataCommon
   {
      friend class _dmsStorageUnit ;
   public:
      _dmsStorageData ( const CHAR *pSuFileName,
                        dmsStorageInfo *pInfo,
                        _IDmsEventHolder *pEventHolder ) ;
      virtual ~_dmsStorageData () ;

   public:
      virtual void postLoadExt( dmsMBContext *context,
                                dmsExtent *extAddr,
                                SINT32 extentID ) ;

      virtual INT32 dumpExtOptions( dmsMBContext *context,
                                    BSONObj &extOptions ) ;

      INT32 fetch ( dmsMBContext *context,
                    const dmsRecordID &recordID,
                    BSONObj &dataRecord,
                    _pmdEDUCB *cb,
                    BOOLEAN dataOwned = FALSE ) ;



   private:
      virtual const CHAR* _getEyeCatcher() const ;

      virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                           dmsExtentID &extOptExtent,
                                           UINT16 &extentPageNum ) ;

      virtual INT32 _onAddCollection( const BSONObj *extOption,
                                      dmsExtentID extOptExtent,
                                      UINT32 extentSize,
                                      UINT16 collectionID ) ;

      virtual void _onAllocExtent( dmsMBContext *context,
                                   dmsExtent *extAddr,
                                   SINT32 extentID ) ;

      virtual INT32 _prepareInsertData( const BSONObj &record,
                                        BOOLEAN mustOID,
                                        pmdEDUCB *cb,
                                        dmsRecordData &recordData,
                                        BOOLEAN &memReallocate,
                                        INT64 position ) ;

      virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

      virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                            UINT32 size,
                                            INT64 position,
                                            dmsRecordID &foundRID,
                                            _pmdEDUCB *cb ) ;

      virtual void _finalRecordSize( UINT32 &size,
                                     const dmsRecordData &recordData ) ;

      virtual INT32 _onInsertFail( dmsMBContext *context, BOOLEAN hasInsert,
                                   dmsRecordID rid, SDB_DPSCB *dpscb,
                                   ossValuePtr dataPtr, _pmdEDUCB *cb ) ;

      virtual INT32 extractData( dmsMBContext *mbContext,
                                 dmsRecordRW &recordRW,
                                 _pmdEDUCB *cb,
                                 dmsRecordData &recordData ) ;

      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;

   private:

      /*
         When recordSize == 0, will not change the delete record size
      */
      INT32 _saveDeletedRecord ( dmsMB *mb,
                                 const dmsRecordID &recordID,
                                 INT32 recordSize = 0 ) ;

      INT32 _saveDeletedRecord ( dmsMB *mb,
                                 const dmsRecordID &rid,
                                 INT32 recordSize,
                                 dmsExtent *extAddr,
                                 dmsDeletedRecord *pRecord ) ;

      void  _mapExtent2DelList ( dmsMB *mb, dmsExtent *extAddr,
                                 SINT32 extentID ) ;

      INT32 _freeExtent ( dmsExtentID extentID,
                          INT32 collectionID ) ;

      INT32 _reserveFromDeleteList ( dmsMBContext *context,
                                     UINT32 requiredSize,
                                     dmsRecordID &resultID,
                                     _pmdEDUCB *cb ) ;

      INT32 _truncateCollection ( dmsMBContext *context,
                                  BOOLEAN needChangeCLID = TRUE ) ;

      INT32 _truncateCollectionLoads( dmsMBContext *context ) ;

      INT32 _extentInsertRecord ( dmsMBContext *context,
                                  dmsExtRW &extRW,
                                  dmsRecordRW &recordRW,
                                  const dmsRecordData &recordData,
                                  UINT32 needRecordSize,
                                  _pmdEDUCB *cb,
                                  BOOLEAN isInsert = TRUE ) ;

      INT32 _extentRemoveRecord ( dmsMBContext *context,
                                  dmsExtRW &extRW,
                                  dmsRecordRW &recordRW,
                                  _pmdEDUCB *cb,
                                  BOOLEAN decCount = TRUE ) ;

      INT32 _extentUpdatedRecord ( dmsMBContext *context,
                                   dmsExtRW &extRW,
                                   dmsRecordRW &recordRW,
                                   const dmsRecordData &recordData,
                                   const BSONObj &newObj,
                                   _pmdEDUCB *cb ) ;

   } ;
   typedef _dmsStorageData dmsStorageData ;
}

#endif /* DMSSTORAGE_DATANORMAL_HPP */

