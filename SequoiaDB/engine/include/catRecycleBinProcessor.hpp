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

   Source File Name = catRecycleBinProcessor.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_RECYCLE_BIN_PROCESSOR_HPP__
#define CAT_RECYCLE_BIN_PROCESSOR_HPP__

#include "oss.hpp"
#include "catDef.hpp"
#include "utilRecycleItem.hpp"
#include "catRecycleReturnInfo.hpp"
#include "catCtxEventHandler.hpp"
#include "clsCatalogAgent.hpp"
#include "pmdEDU.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   // pre-define
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _catRecycleBinManager ;

   /*
      _catRecycleBinProcessor define
    */
   // catRecycleBinPRocessor: processor for each row of recycle objects
   // - query objects by matchers
   // - processor each objects ( check, or recycle, or return, etc )
   class _catRecycleBinProcessor
   {
   public:
      _catRecycleBinProcessor( _catRecycleBinManager *recyBinMgr,
                               utilRecycleItem &item ) ;
      virtual ~_catRecycleBinProcessor() ;

      utilRecycleItem &getRecycleItem()
      {
         return _item ;
      }

      void increaseMatchedCount()
      {
         ++ _matchedCount ;
      }

      UINT32 getMatchedCount()
      {
         return _matchedCount ;
      }

      void increaseProcessedCount()
      {
         ++ _processedCount ;
      }

      UINT32 getProcessedCount()
      {
         return _processedCount ;
      }

      virtual const CHAR *getCollection() const = 0 ;
      virtual const CHAR *getName() const = 0 ;

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) = 0 ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) = 0 ;

      virtual INT32 getExpectedCount() const
      {
         // -1 means we don't know numbers of objects should be processed
         return -1 ;
      }

   protected:
      _catRecycleBinManager * _recyBinMgr ;
      utilRecycleItem & _item ;
      UINT32            _matchedCount ;
      UINT32            _processedCount ;
   } ;

   typedef class _catRecycleBinProcessor catRecycleBinProcessor ;

   /*
      _catDropItemSubCLChecker define
    */
   // check if sub-collection is in a recycled collection space
   class _catDropItemSubCLChecker : public _catRecycleBinProcessor
   {
   public:
      _catDropItemSubCLChecker( _catRecycleBinManager *recyBinMgr,
                                utilRecycleItem &item ) ;
      virtual ~_catDropItemSubCLChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "DropItemSubCLChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;

   protected:
      BOOLEAN _isChecked( utilCSUniqueID csUniqueID ) const ;
      INT32 _saveChecked( utilCSUniqueID csUniqueID ) ;

   protected:
      typedef ossPoolSet< utilCSUniqueID > _CAT_CS_UID_SET ;
      _CAT_CS_UID_SET _checkedSet ;
   } ;

   typedef class _catDropItemSubCLChecker catDropItemSubCLChecker ;

   /*
      _catRecycleProcessor define
    */
   class _catRecycleProcessor : public _catRecycleBinProcessor
   {
   public:
      _catRecycleProcessor( _catRecycleBinManager *recyBinMgr,
                            utilRecycleItem &item,
                            UTIL_RECYCLE_TYPE type ) ;
      virtual ~_catRecycleProcessor() ;

      virtual const CHAR *getCollection() const ;

      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;
   protected:
      INT32 _saveObject( const bson::BSONObj &returnObject,
                         pmdEDUCB *cb,
                         INT16 w ) ;

      virtual INT32 _postSaveObject( const bson::BSONObj &originObject,
                                     bson::BSONObj &recycleObject,
                                     pmdEDUCB *cb,
                                     INT16 w )
      {
         return SDB_OK ;
      }

      INT32 _buildObject( const bson::BSONObj &originObject,
                          pmdEDUCB *cb,
                          INT16 w,
                          BSONObj &recycleObject ) ;

   protected:
      _SDB_DMSCB *            _dmsCB ;
      _dpsLogWrapper *        _dpsCB ;
      UTIL_RECYCLE_TYPE       _type ;
   } ;

   typedef class _catRecycleProcessor catRecycleProcessor ;

   /*
      _catRecycleCSProcessor define
    */
   class _catRecycleCSProcessor : public _catRecycleProcessor
   {
   public:
      _catRecycleCSProcessor( _catRecycleBinManager *recyBinMgr,
                              utilRecycleItem &item ) ;
      virtual ~_catRecycleCSProcessor() ;

      virtual const CHAR *getName() const
      {
         return "RecycleCSProcessor" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
   } ;

   typedef class _catRecycleCSProcessor catRecycleCSProcessor ;

   /*
      _catRecycleCLProcessor define
    */
   class _catRecycleCLProcessor : public _catRecycleProcessor
   {
   public:
      _catRecycleCLProcessor( _catRecycleBinManager *recyBinMgr,
                              utilRecycleItem &item ) ;
      virtual ~_catRecycleCLProcessor() ;

      virtual const CHAR *getName() const
      {
         return "RecycleCLProcessor" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 _postSaveObject( const bson::BSONObj &originObject,
                                     bson::BSONObj &recycleObject,
                                     pmdEDUCB *cb,
                                     INT16 w ) ;
   } ;

   typedef class _catRecycleCLProcessor catRecycleCLProcessor ;

   /*
      _catRecycleSubCLProcessor define
    */
   class _catRecycleSubCLProcessor : public _catRecycleProcessor
   {
   public:
      _catRecycleSubCLProcessor( _catRecycleBinManager *recyBinMgr,
                                 utilRecycleItem &item ) ;
      virtual ~_catRecycleSubCLProcessor() ;

      virtual const CHAR *getName() const
      {
         return "RecycleSubCLProcessor" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 _postSaveObject( const bson::BSONObj &originObject,
                                     bson::BSONObj &recycleObject,
                                     pmdEDUCB *cb,
                                     INT16 w ) ;
   } ;

   typedef class _catRecycleSubCLProcessor catRecycleSubCLProcessor ;

   /*
      _catRecycleSeqProcessor define
    */
   class _catRecycleSeqProcessor : public _catRecycleProcessor
   {
   public:
      _catRecycleSeqProcessor( _catRecycleBinManager *recyBinMgr,
                               utilRecycleItem &item ) ;
      virtual ~_catRecycleSeqProcessor() ;

      virtual const CHAR *getName() const
      {
         return "RecycleSeqProcessor" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
   } ;

   typedef class _catRecycleSeqProcessor catRecycleSeqProcessor ;

   /*
      _catRecycleIdxProcessor define
    */
   class _catRecycleIdxProcessor : public _catRecycleProcessor
   {
   public:
      _catRecycleIdxProcessor( _catRecycleBinManager *recyBinMgr,
                               utilRecycleItem &item ) ;
      virtual ~_catRecycleIdxProcessor() ;

      virtual const CHAR *getName() const
      {
         return "RecycleIdxProcessor" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
   } ;

   typedef class _catRecycleIdxProcessor catRecycleIdxProcessor ;

   /*
      _catReturnCheckerBase define
    */
   class _catReturnCheckerBase : public _catRecycleBinProcessor
   {
   public:
      _catReturnCheckerBase( _catRecycleBinManager *recyBinMgr,
                             utilRecycleItem &item,
                             const catReturnConfig &conf,
                             catRecycleReturnInfo &info ) ;
      _catReturnCheckerBase( _catReturnCheckerBase &checker ) ;
      virtual ~_catReturnCheckerBase() ;

   protected:
      const catReturnConfig & _conf ;
      catRecycleReturnInfo &  _info ;
   } ;

   typedef class _catReturnCheckerBase catReturnCheckerBase ;

   /*
      _catReturnChecker define
    */
   class _catReturnChecker : public _catReturnCheckerBase
   {
   public:
      _catReturnChecker( _catRecycleBinManager *recyBinMgr,
                         utilRecycleItem &item,
                         const catReturnConfig &conf,
                         catRecycleReturnInfo &info,
                         catCtxLockMgr &lockMgr,
                         catCtxGroupHandler &groupHandler ) ;
      _catReturnChecker( _catReturnChecker &checker ) ;
      virtual ~_catReturnChecker() ;

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;

   protected:
      virtual const CHAR *_getOrigUIDField() const
      {
         return FIELD_NAME_UNIQUEID ;
      }

   protected:
      catCtxLockMgr &         _lockMgr ;
      catCtxGroupHandler &    _groupHandler ;
   } ;

   typedef class _catReturnChecker catReturnChecker ;

   /*
      _catReturnCSChecker define
    */
   class _catReturnCSChecker : public _catReturnChecker
   {
   public:
      _catReturnCSChecker( _catRecycleBinManager *recyBinMgr,
                           utilRecycleItem &item,
                           const catReturnConfig &conf,
                           catRecycleReturnInfo &info,
                           catCtxLockMgr &lockMgr,
                           catCtxGroupHandler &groupHandler ) ;
      virtual ~_catReturnCSChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "ReturnCSChecker" ;
      }

      virtual INT32 getExpectedCount() const
      {
         return 1 ;
      }

      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;

   protected:
      INT32 _checkCS( const CHAR *csName,
                      const bson::BSONObj &object,
                      pmdEDUCB *cb ) ;
      INT32 _checkReturnCSToName( utilRecycleItem &recycleItem,
                                  const CHAR *returnName,
                                  pmdEDUCB *cb ) ;
      INT32 _checkConflictCSByName( const utilRecycleItem &item,
                                    const utilReturnNameInfo &nameInfo,
                                    _pmdEDUCB *cb,
                                    BOOLEAN &isConflict ) ;
      INT32 _checkReturnCLInCS( utilRecycleItem &recycleItem,
                                _pmdEDUCB *cb ) ;
      INT32 _lockCollectionSpace( const utilReturnNameInfo &nameInfo,
                                  BOOLEAN isConflict ) ;
      INT32 _checkDomain( const bson::BSONObj &object,
                          _pmdEDUCB *cb ) ;
      INT32 _checkGroups() ;
   } ;

   typedef class _catReturnCSChecker catReturnCSChecker ;

   /*
      _catReturnCLChecker define
    */
   class _catReturnCLChecker : public _catReturnChecker
   {
   public:
      _catReturnCLChecker( _catRecycleBinManager *recyBinMgr,
                           utilRecycleItem &item,
                           const catReturnConfig &conf,
                           catRecycleReturnInfo &info,
                           catCtxLockMgr &lockMgr,
                           catCtxGroupHandler &groupHandler ) ;
      virtual ~_catReturnCLChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "ReturnCLChecker" ;
      }

      virtual INT32 getExpectedCount() const
      {
         return 1 ;
      }

      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;

   protected:
      _catReturnCLChecker( _catReturnChecker &checker ) ;

      virtual INT32 _checkCL( const CHAR *clName,
                              const bson::BSONObj &object,
                              clsCatalogSet &catSet,
                              pmdEDUCB *cb ) ;
      INT32 _checkDomain( clsCatalogSet &catSet,
                          pmdEDUCB *cb ) ;

      INT32 _checkConflictCLByName( const utilRecycleItem &item,
                                    const utilReturnNameInfo &nameInfo,
                                    const clsCatalogSet &recycleSet,
                                    _pmdEDUCB *cb,
                                    BOOLEAN &isConflict ) ;
      INT32 _checkConflictCLByUID( const utilRecycleItem &item,
                                   const utilReturnNameInfo &nameInfo,
                                   const clsCatalogSet &recycleSet,
                                   _pmdEDUCB *cb,
                                   BOOLEAN &isConflict ) ;
      INT32 _checkConflictSeqByUID( const clsCatalogSet &recycleSet ) ;
      INT32 _checkReturnCLToName( utilRecycleItem &item,
                                  const CHAR *returnName,
                                  _pmdEDUCB *cb ) ;
      INT32 _checkReturnMainCL( const clsCatalogSet &mainCLSet,
                                _pmdEDUCB *cb ) ;
      INT32 _lockCollection( const utilReturnNameInfo &nameInfo,
                             BOOLEAN isConflict ) ;
   } ;

   typedef class _catReturnCLChecker catReturnCLChecker ;

   /*
      _catReturnCLInCSChecker define
    */
   class _catReturnCLInCSChecker : public _catReturnCLChecker
   {
   public:
      _catReturnCLInCSChecker( catReturnCSChecker &checker ) ;
      virtual ~_catReturnCLInCSChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "ReturnCLInCSChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;

      virtual INT32 getExpectedCount() const
      {
         return -1 ;
      }

   protected:
      virtual INT32 _checkCL( const CHAR *clName,
                              const bson::BSONObj &object,
                              clsCatalogSet &catSet,
                              pmdEDUCB *cb ) ;
   } ;

   typedef class _catReturnCLInCSChecker catReturnCLInCSChecker ;

   /*
      _catReturnSubCLChecker define
    */
   class _catReturnSubCLChecker : public _catReturnCLChecker
   {
   public:
      _catReturnSubCLChecker( catReturnCLChecker &mainCLChecker,
                              UINT32 subCLCount ) ;
      virtual ~_catReturnSubCLChecker() ;

      virtual const CHAR *getName() const
      {
         return "ReturnSubCLChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;

      virtual INT32 getExpectedCount() const
      {
         return (INT32)_subCLCount ;
      }

   protected:
      INT32 _checkCL( const CHAR *clName,
                      const bson::BSONObj &object,
                      clsCatalogSet &catSet,
                      pmdEDUCB *cb ) ;

   protected:
      UINT32 _subCLCount ;
   } ;

   typedef class _catReturnSubCLChecker catReturnSubCLChecker ;

   /*
      _catReturnIdxChecker define
    */
   class _catReturnIdxChecker : public _catReturnCheckerBase
   {
   public:
      _catReturnIdxChecker( _catReturnChecker &checker,
                            UINT16 rebuildTypes ) ;
      virtual ~_catReturnIdxChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "ReturnIdxChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;

      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;
   protected:
      UINT16 _rebuildTypes ;
   } ;

   typedef class _catReturnIdxChecker catReturnIdxChecker ;

   /*
      _catReturnProcessor define
    */
   class _catReturnProcessor : public _catRecycleBinProcessor
   {
   public:
      _catReturnProcessor( _catRecycleBinManager *recyBinMgr,
                           utilRecycleItem &item,
                           catRecycleReturnInfo &info,
                           UTIL_RECYCLE_TYPE type ) ;
      virtual ~_catReturnProcessor() ;

      virtual const CHAR *getCollection() const ;

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;
   protected:
      INT32 _saveObject( const bson::BSONObj &returnObject,
                         pmdEDUCB *cb,
                         INT16 w ) ;
      virtual INT32 _buildObject( const bson::BSONObj &recycleObject,
                                  pmdEDUCB *cb,
                                  INT16 w,
                                  BSONObj &returnObject,
                                  BOOLEAN &needProcess ) = 0 ;

      virtual INT32 _postSaveObject( const bson::BSONObj &recycleObject,
                                     pmdEDUCB *cb,
                                     INT16 w )
      {
         return SDB_OK ;
      }

   protected:
      _SDB_DMSCB *            _dmsCB ;
      _dpsLogWrapper *        _dpsCB ;
      catRecycleReturnInfo &  _info ;
      UTIL_RECYCLE_TYPE       _type ;
   } ;

   typedef class _catReturnProcessor catReturnProcessor ;

   /*
      _catReturnCSProcessor define
    */
   class _catReturnCSProcessor : public _catReturnProcessor
   {
   public:
      _catReturnCSProcessor( _catRecycleBinManager *recyBinMgr,
                             utilRecycleItem &item,
                             catRecycleReturnInfo &info ) ;
      virtual ~_catReturnCSProcessor() ;

      virtual const CHAR *getName() const
      {
         return "ReturnCSProcessor" ;
      }

   protected:
      virtual INT32 _buildObject( const bson::BSONObj &recycleObject,
                                  pmdEDUCB *cb,
                                  INT16 w,
                                  bson::BSONObj &returnObject,
                                  BOOLEAN &needProcess ) ;
   } ;

   typedef class _catReturnCSProcessor catReturnCSProcessor ;

   /*
      _catReturnCLProcessor define
    */
   class _catReturnCLProcessor : public _catReturnProcessor
   {
   public:
      _catReturnCLProcessor( _catRecycleBinManager *recyBinMgr,
                             utilRecycleItem &item,
                             catRecycleReturnInfo &info ) ;
      virtual ~_catReturnCLProcessor() ;

      virtual const CHAR *getName() const
      {
         return "ReturnCLProcessor" ;
      }

   protected:
      virtual INT32 _buildObject( const bson::BSONObj &recycleObject,
                                  pmdEDUCB *cb,
                                  INT16 w,
                                  bson::BSONObj &returnObject,
                                  BOOLEAN &needProcess ) ;

      INT32 _buildCLObject( const CHAR *clName,
                            const bson::BSONObj &recycleObject,
                            pmdEDUCB *cb,
                            INT16 w,
                            bson::BSONObj &returnObject,
                            BOOLEAN &needProcess ) ;
      INT32 _rebuildCLName( const CHAR *targetCLName,
                            utilReturnNameInfo &nameInfo,
                            BOOLEAN needCheckCS,
                            BOOLEAN &isInSameCS ) ;
      INT32 _rebuildObject( const bson::BSONObj &recycleObject,
                            INT32 clVersion,
                            const utilReturnUIDInfo &uidInfo,
                            const utilReturnNameInfo &nameInfo,
                            const utilReturnNameInfo &mainCLNameInfo,
                            BOOLEAN rebuildSubCL,
                            const bson::BSONObj &subCLInfo,
                            BOOLEAN rebuildAutoInc,
                            const bson::BSONObj &autoIncInfo,
                            bson::BSONObj &returnObject ) ;
      INT32 _rebuildAutoInc( const _clsCatalogSet &returnSet,
                             bson::BSONObj &autoIncInfo ) ;

      virtual INT32 _postSaveObject( const bson::BSONObj &recycleObject,
                                     pmdEDUCB *cb,
                                     INT16 w ) ;

   protected:
      utilReturnNameInfo _returnNameInfo ;
      utilReturnUIDInfo  _returnUIDInfo ;
   } ;

   typedef class _catReturnCLProcessor catReturnCLProcessor ;

   /*
      _catReturnSeqProcessor define
    */
   class _catReturnSeqProcessor : public _catReturnProcessor
   {
   public:
      _catReturnSeqProcessor( _catRecycleBinManager *recyBinMgr,
                              utilRecycleItem &item,
                              catRecycleReturnInfo &info ) ;
      virtual ~_catReturnSeqProcessor() ;

      virtual const CHAR *getName() const
      {
         return "ReturnSeqProcessor" ;
      }

   protected:
      virtual INT32 _buildObject( const bson::BSONObj &recycleObject,
                                  pmdEDUCB *cb,
                                  INT16 w,
                                  bson::BSONObj &returnObject,
                                  BOOLEAN &needProcess ) ;
   } ;

   typedef class _catReturnSeqProcessor catReturnSeqProcessor ;

   /*
      _catReturnIdxProcessor define
    */
   class _catReturnIdxProcessor : public _catReturnProcessor
   {
   public:
      _catReturnIdxProcessor( _catRecycleBinManager *recyBinMgr,
                              utilRecycleItem &item,
                              catRecycleReturnInfo &info,
                              UINT16 rebuildTypes ) ;
      virtual ~_catReturnIdxProcessor() ;

      virtual const CHAR *getName() const
      {
         return "ReturnIdxProcessor" ;
      }

   protected:
      virtual INT32 _buildObject( const bson::BSONObj &recycleObject,
                                  pmdEDUCB *cb,
                                  INT16 w,
                                  bson::BSONObj &returnObject,
                                  BOOLEAN &needProcess ) ;

   protected:
      UINT16 _rebuildTypes ;
   } ;

   typedef class _catReturnIdxProcessor catReturnIdxProcessor ;

   /*
     _catRecycleSubCLLocker define
    */
   class _catRecycleSubCLLocker : public _catRecycleBinProcessor
   {
   public:
      _catRecycleSubCLLocker( _catRecycleBinManager *recyBinMgr,
                              utilRecycleItem item,
                              catCtxLockMgr &lockMgr,
                              OSS_LATCH_MODE &mode,
                              ossPoolSet< utilCSUniqueID > *lockedCS,
                              ossPoolSet< utilCSUniqueID > &lockedSubCLCS ) ;
      virtual ~ _catRecycleSubCLLocker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "RecycleSubCLLocker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w )  ;
   protected:
      catCtxLockMgr &                     _lockMgr ;
      OSS_LATCH_MODE &                    _lockMode ;
      ossPoolSet< utilCSUniqueID > *      _lockedCS ;
      ossPoolSet< utilCSUniqueID > &      _lockedSubCLCS ;
   } ;

   typedef class  _catRecycleSubCLLocker catRecycleSubCLLocker ;

   /*
      _catDropCSItemChecker define
    */
   // check if collections within another ( main collection ) recycle item is
   // in a dropping collection space recycle item
   class _catDropCSItemChecker : public _catRecycleBinProcessor
   {
   public:
      _catDropCSItemChecker( _catRecycleBinManager *recyBinMgr,
                             utilRecycleItem &item ) ;
      virtual ~_catDropCSItemChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "DropCSItemChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;
   } ;

   typedef class _catDropCSItemChecker catDropCSItemChecker ;


}

#endif // CAT_RECYCLE_BIN_PROCESSOR_HPP__
