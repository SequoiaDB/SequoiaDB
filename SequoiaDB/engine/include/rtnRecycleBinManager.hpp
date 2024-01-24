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

   Source File Name = rtnRecycleBinManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle bin manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_RECYCLE_BIN_MANAGER_HPP__
#define RTN_RECYCLE_BIN_MANAGER_HPP__

#include "oss.hpp"
#include "ossLatch.hpp"
#include "rtn.hpp"
#include "utilRecycleBinConf.hpp"
#include "utilRecycleItem.hpp"

namespace engine
{

   // check interval in each 1 minute
   // in microseconds
   #define RTN_RECYCLE_CHECK_INTERVAL  ( 60 * 1000 * 1000 )
   #define RTN_RECYCLE_RETRY_INTERVAL  ( 1000 * 1000 )

   /*
      _rtnRecycleBinManager define
    */
   class _rtnRecycleBinManager : public SDBObject
   {
   public:
      _rtnRecycleBinManager() ;
      virtual ~_rtnRecycleBinManager() ;

      INT32 init() ;
      void  fini() ;

      INT32 startBGJob() ;

      // set config of recycle bin
      OSS_INLINE void setConf( const utilRecycleBinConf &conf )
      {
         ossScopedLock _lock( &_confLatch, EXCLUSIVE ) ;
         _conf = conf ;
         _isConfValid = TRUE ;

         // make sure back ground job will start checks with new configures
         // e.g. limits of items may be changed
         enableBGJob() ;
      }

      // get config of recycle bin
      OSS_INLINE utilRecycleBinConf getConf()
      {
         ossScopedLock _lock( &_confLatch, SHARED ) ;
         return _conf ;
      }

      // set configure is invalid
      OSS_INLINE void setConfInvalid()
      {
         ossScopedLock _lock( &_confLatch, EXCLUSIVE ) ;
         _isConfValid = FALSE  ;
      }

      // get if configure is valid
      OSS_INLINE BOOLEAN isConfValid()
      {
         ossScopedLock _lock( &_confLatch, SHARED ) ;
         return _isConfValid ;
      }

      // register return item process
      OSS_INLINE void registerReturn()
      {
         _returnCount.inc() ;
      }

      // unregister return item process
      OSS_INLINE void unregisterReturn()
      {
        _returnCount.dec() ;
      }

      // check if has returning items in progress
      OSS_INLINE BOOLEAN hasReturningItems()
      {
         return !( _returnCount.compare( 0 ) ) ;
      }

      // get recycle item by recycle name
      INT32 getItem( const CHAR *recycleName,
                     pmdEDUCB *cb,
                     utilRecycleItem &item ) ;
      // get recycle item by origin ID
      INT32 getItemByOrigID( utilGlobalID originID,
                             pmdEDUCB *cb,
                             utilRecycleItem &item ) ;
      // get recycle item by recycle ID
      INT32 getItemByRecyID( utilGlobalID recycleID,
                             pmdEDUCB *cb,
                             utilRecycleItem &item ) ;
      // get recycle items by given matcher and order
      INT32 getItems( const bson::BSONObj &matcher,
                      const bson::BSONObj &orderBy,
                      const bson::BSONObj &hint,
                      INT64 numToReturn,
                      pmdEDUCB *cb,
                      UTIL_RECY_ITEM_LIST &itemList )
      {
         return _getItems( matcher, orderBy, hint, numToReturn, cb, itemList ) ;
      }

      // get recycle items in context by given matcher and order
      INT32 getItems( const bson::BSONObj &matcher,
                      const bson::BSONObj &orderBy,
                      const bson::BSONObj &hint,
                      INT64 numToReturn,
                      pmdEDUCB *cb,
                      INT64 &contextID )
      {
         return _getItems( matcher, orderBy, hint, numToReturn, cb, contextID ) ;
      }

      // count all recycle items
      OSS_INLINE INT32 countAllItems( pmdEDUCB *cb,
                                      INT64 &count )
      {
         bson::BSONObj dummy ;
         return _countItems( dummy, cb, count ) ;
      }

      // count recycle items by matcher
      OSS_INLINE INT32 countItems( const bson::BSONObj &matcher,
                                   pmdEDUCB *cb,
                                   INT64 &count )
      {
         return _countItems( matcher, cb, count ) ;
      }

      // drop expired items
      INT32 dropExpiredItems( pmdEDUCB *cb,
                              BOOLEAN &isFinished ) ;

      // enable back ground job
      void enableBGJob()
      {
         enableLimitCheck() ;
         enableEmptyCheck() ;
      }

      // enable back ground job to check limit
      void enableLimitCheck()
      {
         _limitCheckFlag.poke( 1 ) ;
      }

      // enable back ground job to check empty of recycle bin
      void enableEmptyCheck()
      {
         _emptyCheckFlag.poke( 1 ) ;
      }

   protected:
      // get collection to save recycle items
      virtual const CHAR *_getRecyItemCL() const = 0 ;
      // get drop latch
      virtual ossXLatch *_getDropLatch() = 0 ;
      // drop given recycle item
      virtual INT32 _dropItem( const utilRecycleItem &item,
                               pmdEDUCB *cb,
                               INT16 w,
                               BOOLEAN ignoreNotExists,
                               BOOLEAN &isDropped ) = 0 ;
      // create background job for recycle bin
      // e.g. clear expired or out of limit recycle items
      virtual INT32 _createBGJob( utilLightJob **pJob ) ;

      // get recycle item in BSON by recycle name
      INT32 _getItemObject( const CHAR *recycleName,
                            pmdEDUCB *cb,
                            bson::BSONObj &object ) ;
      // get recycle item in BSON by origin ID
      INT32 _getItemObjectByOrigID( utilGlobalID originID,
                                    pmdEDUCB *cb,
                                    bson::BSONObj &object ) ;
      // get recycle item in BSON by recycle ID
      INT32 _getItemObjectByRecyID( utilGlobalID recycleID,
                                    pmdEDUCB *cb,
                                    bson::BSONObj &object ) ;
      // get recycle item in BSON by matcher
      INT32 _getItemObject( const bson::BSONObj &matcher,
                            pmdEDUCB *cb,
                            bson::BSONObj &object ) ;

      // get recycle items in context by matcher and order
      INT32 _getItems( const bson::BSONObj &matcher,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint,
                       INT64 numToReturn,
                       pmdEDUCB *cb,
                       INT64 &contextID ) ;
      // get recycle items in list by matcher and order
      INT32 _getItems( const bson::BSONObj &matcher,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint,
                       INT64 numToReturn,
                       pmdEDUCB *cb,
                       UTIL_RECY_ITEM_LIST &itemList ) ;

      // count recycle items by matcher
      INT32 _countItems( const bson::BSONObj &matcher,
                         pmdEDUCB *cb,
                         INT64 &count )
      {
         return _countItems( matcher, BSONObj(), cb, count ) ;
      }

      INT32 _countItems( const bson::BSONObj &matcher,
                         const bson::BSONObj &hint,
                         pmdEDUCB *cb,
                         INT64 &count ) ;

      // delete given recycle item
      INT32 _deleteItem( const utilRecycleItem &item,
                         pmdEDUCB *cb,
                         INT16 w,
                         BOOLEAN ignoreNotExists ) ;
      // delete recycle items by matcher
      INT32 _deleteItems( const bson::BSONObj &matcher,
                          pmdEDUCB *cb,
                          INT16 w,
                          UINT64 &deletedCount ) ;
      // delete all recycle items
      INT32 _deleteAllItems( pmdEDUCB *cb, INT16 w ) ;

      // get oldest recycle items
      INT32 _getOldestItems( UINT64 expiredTime,
                             INT64 numToReturn,
                             pmdEDUCB *cb,
                             UTIL_RECY_ITEM_LIST &itemList ) ;

      // get and cache oldest recycle item
      INT32 _getAndCacheOldestItem( pmdEDUCB *cb,
                                    utilRecycleItem &item,
                                    BOOLEAN &fromCache ) ;
      // check if oldest recycle item is expired
      BOOLEAN _isOldestItemExpired( UINT64 expiredTime ) ;
      // get oldest recycle item
      void _getOldestItem( utilRecycleItem &item ) ;
      // cache oldest recycle item
      void _cacheOldestItem( const utilRecycleItem &item ) ;
      // reset oldest recycle item cache
      void _resetOldestItem( const utilRecycleItem &curItem ) ;

      // save recycle item
      INT32 _saveItemObject( const bson::BSONObj &object,
                             pmdEDUCB *cb,
                             INT16 w ) ;

      // update recycle items by matcher
      INT32 _updateItems( const bson::BSONObj &matcher,
                          const bson::BSONObj &updator,
                          pmdEDUCB *cb,
                          INT16 w ) ;

   protected:
      // latch to protect configuration
      ossSpinSLatch        _confLatch ;
      // cache of recycle bin information
      utilRecycleBinConf   _conf ;
      // indicates if configure is valid
      BOOLEAN              _isConfValid ;

      // latch to protect oldest item
      ossSpinXLatch        _oldestItemLatch ;
      // cache of oldest recycle item
      utilRecycleItem      _oldestItem ;

      // count of returning recycle items
      // NOTE: if someone is returning recycle items, we should not do any
      //       dropping in background job
      ossAtomic32          _returnCount ;

      // flag to notify background job to check configure limit for
      // number of recycle bin items
      ossAtomic32          _limitCheckFlag ;
      // flag to notify background job to check recycle bin is empty
      ossAtomic32          _emptyCheckFlag ;
      // indicates whether recycle bin is empty
      BOOLEAN              _isEmptyFlag ;

      SDB_RTNCB * _rtnCB ;
      SDB_DMSCB * _dmsCB ;
      SDB_DPSCB * _dpsCB ;

      bson::BSONObj        _hintRecyID ;
      bson::BSONObj        _hintOrigID ;
      bson::BSONObj        _hintName ;
      bson::BSONObj        _hintOrigName ;
   } ;

   typedef class _rtnRecycleBinManager rtnRecycleBinManager ;

   /*
      _rtnDropRecycleBinBGJob define
    */
   class _rtnDropRecycleBinBGJob : public _utilLightJob
   {
   public:
      _rtnDropRecycleBinBGJob( rtnRecycleBinManager *recycleBinMgr ) ;
      virtual ~_rtnDropRecycleBinBGJob() ;

      virtual const CHAR *name() const
      {
         return "DropRecycleBinBG" ;
      }

      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   protected:
      rtnRecycleBinManager *  _recycleBinMgr ;
   } ;

   typedef class _rtnDropRecycleBinBGJob rtnDropRecycleBinBGJob ;

}

#endif // RTN_RECYCLE_BIN_MANAGER_HPP__
