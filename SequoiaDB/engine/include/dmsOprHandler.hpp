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

   Source File Name = dmsOprHandler.hpp

   Descriptive Name = External data process handler for dms.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_OPR_HANDLER_HPP__
#define DMS_OPR_HANDLER_HPP__

#include "oss.hpp"
#include "ixm.hpp"
#include "dms.hpp"
#include "utilInsertResult.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _dmsMBContext ;
   class _dmsRecordRW ;
   class _pmdEDUCB ;

   /*
      _dmsTransRecordInfo define
   */
   struct _dmsTransRecordInfo
   {
      UINT32         _refCount ;
      BOOLEAN        _transInsert ;
      BOOLEAN        _transInsertDeleted ;
      BOOLEAN        _transLockEscalated ;

      _dmsTransRecordInfo()
      {
         reset() ;
      }
      void reset()
      {
         _refCount = 0 ;
         _transInsert = FALSE ;
         _transInsertDeleted = FALSE ;
         _transLockEscalated = FALSE ;
      }
   } ;
   typedef _dmsTransRecordInfo dmsTransRecordInfo ;

   /*
      _IDmsOprHandler define
   */
   class _IDmsOprHandler
   {
   public:
      _IDmsOprHandler() {}
      virtual ~_IDmsOprHandler() {}

   public:

      virtual void  onCSClosed( INT32 csID ) = 0 ;
      virtual void  onCLTruncated( INT32 csID, UINT16 clID ) = 0 ;

      virtual INT32 onCreateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   _pmdEDUCB *cb ) = 0 ;

      virtual INT32 onDropIndex( _dmsMBContext *context,
                                 const ixmIndexCB *indexCB,
                                 _pmdEDUCB *cb ) = 0 ;

      virtual INT32 onRebuildIndex( _dmsMBContext *context,
                                    const ixmIndexCB *indexCB,
                                    _pmdEDUCB *cb,
                                    utilWriteResult *pResult ) = 0 ;

      virtual INT32 onInsertRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) = 0 ;

      virtual INT32 onDeleteRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    BOOLEAN markDeleting,
                                    _pmdEDUCB* cb ) = 0 ;

      virtual INT32 onUpdateRecord( _dmsMBContext *context,
                                    const BSONObj &orignalObj,
                                    const BSONObj &newObj,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) = 0 ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) = 0 ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObj &keyObj,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) = 0 ;

      virtual INT32 onUpdateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &oldKeySet,
                                   const BSONObjSet &newKeySet,
                                   const dmsRecordID &rid,
                                   BOOLEAN isRollback,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) = 0 ;

      virtual INT32 onDeleteIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb ) = 0 ;

   } ;
   typedef _IDmsOprHandler IDmsOprHandler ;

}

#endif /* DMS_OPR_HANDLER_HPP__ */

