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
      _dmsStorageData ( IStorageService *service,
                        dmsSUDescriptor *suDescriptor,
                        const CHAR *pSuFileName,
                        _IDmsEventHolder *pEventHolder ) ;
      virtual ~_dmsStorageData () ;

   public:
      virtual void postLoadExt( dmsMBContext *context,
                                dmsExtent *extAddr,
                                SINT32 extentID ) ;

      virtual INT32 dumpExtOptions( dmsMBContext *context,
                                    BSONObj &extOptions ) ;

      virtual INT32 setExtOptions ( dmsMBContext * context,
                                    const BSONObj & extOptions ) ;

      virtual OSS_LATCH_MODE getWriteLockType() const
      {
         return SHARED ;
      }

   private:
      virtual const CHAR* _getEyeCatcher() const ;

      virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                           dmsCreateCLOptions &options ) ;

      virtual INT32 _onAddCollection( const BSONObj *extOption,
                                      dmsExtentID extOptExtent,
                                      UINT32 extentSize,
                                      UINT16 collectionID ) ;

      virtual void _onAllocExtent( dmsMBContext *context,
                                   dmsExtent *extAddr,
                                   SINT32 extentID ) ;

      virtual INT32 _checkInsertData( const BSONObj &record,
                                      BOOLEAN mustOID,
                                      pmdEDUCB *cb,
                                      dmsRecordData &recordData,
                                      BOOLEAN &memReallocate,
                                      INT64 position ) ;

      virtual INT32 _prepareInsert( const dmsRecordID &recordID,
                                    const dmsRecordData &recordData ) ;

      virtual INT32 _getRecordPosition( dmsMBContext *context,
                                        const dmsRecordID &rid,
                                        const dmsRecordData &recordData,
                                        pmdEDUCB *cb,
                                        INT64 &position ) ;

      virtual INT32 _checkReusePosition( dmsMBContext *context,
                                         const DPS_TRANS_ID &transID,
                                         pmdEDUCB *cb,
                                         INT64 &position,
                                         dmsRecordID &foundRID ) ;

      virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

      virtual INT32 _checkRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

      virtual void _finalRecordSize( UINT32 &size,
                                     const dmsRecordData &recordData ) ;

      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;

   private:
      //   must be hold the mb EXCLUSIVE lock in this functions :

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
                                 dmsDeletedRecord *pRecord,
                                 BOOLEAN isRecycled ) ;

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
   } ;
   typedef _dmsStorageData dmsStorageData ;
}

#endif /* DMSSTORAGE_DATANORMAL_HPP */

