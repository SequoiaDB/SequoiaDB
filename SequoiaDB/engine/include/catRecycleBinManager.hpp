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

   Source File Name = catRecycleBinManager.hpp

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

#ifndef CAT_RECYCLE_BIN_MANAGER_HPP__
#define CAT_RECYCLE_BIN_MANAGER_HPP__

#include "oss.hpp"
#include "catDef.hpp"
#include "utilRecycleBinConf.hpp"
#include "rtnRecycleBinManager.hpp"
#include "catRecycleBinProcessor.hpp"
#include "utilRecycleItem.hpp"
#include "catLevelLock.hpp"
#include "pmdEDU.hpp"

namespace engine
{

   // pre-define
   class sdbCatalogueCB ;

   /*
      _catRecycleBinManager define
    */
   class _catRecycleBinManager : public _rtnRecycleBinManager
   {
   protected:
      typedef _rtnRecycleBinManager _BASE ;

   public:
      _catRecycleBinManager() ;
      virtual ~_catRecycleBinManager() ;

      INT32 init( const utilRecycleBinConf &conf ) ;
      INT32 active( const utilRecycleBinConf &conf ) ;

      // update config of recycle bin
      INT32 updateConf( const utilRecycleBinConf &newConf,
                        pmdEDUCB *cb,
                        INT16 w ) ;

      INT32 prepareItem( utilRecycleItem &item,
                         UTIL_RECY_ITEM_LIST &droppingItems,
                         catCtxLockMgr &lockMgr,
                         pmdEDUCB *cb,
                         INT16 w ) ;

      INT32 commitItem( utilRecycleItem &item,
                        pmdEDUCB *cb,
                        INT16 w ) ;

      INT32 dropItem( const utilRecycleItem &item,
                      pmdEDUCB *cb,
                      INT16 w ) ;
      INT32 dropAllItems( pmdEDUCB *cb, INT16 w ) ;

      INT32 countItemsInCS( utilCSUniqueID csUniqueID,
                            pmdEDUCB *cb,
                            INT64 &count ) ;
      INT32 dropItemsInCS( utilCSUniqueID csUniqueID,
                           pmdEDUCB *cb,
                           INT16 w ) ;

      INT32 returnItem( utilRecycleItem &item,
                        catRecycleReturnInfo &info,
                        pmdEDUCB *cb,
                        INT16 w ) ;
      INT32 tryLockItem( const utilRecycleItem &item,
                         pmdEDUCB *cb,
                         OSS_LATCH_MODE mode,
                         catCtxLockMgr &lockMgr,
                         ossPoolSet< utilCSUniqueID > *lockedCS = NULL,
                         BOOLEAN isCheckSubCL = TRUE ) ;
      OSS_INLINE void reserveItem()
      {
         ++ _reservedCount ;
      }

      OSS_INLINE void unreserveItem()
      {
         -- _reservedCount ;
      }

      INT32 processObjects( catRecycleBinProcessor &processor,
                            pmdEDUCB *cb,
                            INT16 w ) ;

      OSS_INLINE BOOLEAN tryLockDropLatch()
      {
         return _dropLatch.try_get() ;
      }

      OSS_INLINE void unlockDropLatch()
      {
         _dropLatch.release() ;
      }

   protected:
      virtual const CHAR *_getRecyItemCL() const
      {
         return CAT_SYSRECYCLEBIN_ITEM_COLLECTION ;
      }

      virtual ossXLatch *_getDropLatch()
      {
         return &_dropLatch ;
      }

      virtual INT32 _dropItem( const utilRecycleItem &item,
                               pmdEDUCB *cb,
                               INT16 w,
                               BOOLEAN ignoreNotExists,
                               BOOLEAN &isDropped ) ;

      INT32 _dropItemImpl( const utilRecycleItem &item,
                           pmdEDUCB *cb,
                           INT16 w,
                           BOOLEAN ignoreNotExists ) ;
      INT32 _deleteObjects( UTIL_RECYCLE_TYPE type,
                            const bson::BSONObj &matcher,
                            pmdEDUCB *cb,
                            INT16 w ) ;
      INT32 _deleteAllObjects( UTIL_RECYCLE_TYPE type,
                               pmdEDUCB *cb,
                               INT16 w ) ;

      INT32 _checkAvailable( const CHAR *originName,
                             utilCLUniqueID originID,
                             const utilRecycleBinConf &conf,
                             pmdEDUCB *cb,
                             BOOLEAN &isAvailable,
                             BOOLEAN &isLimitedByVersion ) ;
      INT32 _acquireID( utilRecycleID &recycleID,
                        pmdEDUCB *cb,
                        INT16 w ) ;

      INT32 _checkCapacity( const utilRecycleBinConf &conf,
                            pmdEDUCB *cb ) ;
      INT32 _checkVersion( const CHAR *originName,
                           utilCLUniqueID originID,
                           const utilRecycleBinConf &conf,
                           pmdEDUCB *cb ) ;
      INT32 _tryLockItemsForRecycle( const utilRecycleItem &item,
                                     const UTIL_RECY_ITEM_LIST &droppingItems,
                                     pmdEDUCB *cb,
                                     catCtxLockMgr &lockMgr ) ;
      INT32 _saveItem( utilRecycleItem &item,
                       pmdEDUCB *cb,
                       INT16 w ) ;
      INT32 _returnCSObjects( utilRecycleItem &item,
                              catRecycleReturnInfo &info,
                              pmdEDUCB *cb,
                              INT16 w ) ;
      INT32 _returnCLObjects( utilRecycleItem &item,
                              catRecycleReturnInfo &info,
                              pmdEDUCB *cb,
                              INT16 w ) ;
      INT32 _returnSeqObjects( utilRecycleItem &item,
                               catRecycleReturnInfo &info,
                               pmdEDUCB *cb,
                               INT16 w ) ;
      INT32 _returnIdxObjects( utilRecycleItem &item,
                               catRecycleReturnInfo &info,
                               pmdEDUCB *cb,
                               INT16 w ) ;
      INT32 _recycleItem( utilRecycleItem &item, pmdEDUCB *cb, INT16 w ) ;
      INT32 _recycleCSObjects( utilRecycleItem &item, pmdEDUCB *cb, INT16 w ) ;
      INT32 _recycleCLObjects( utilRecycleItem &item, pmdEDUCB *cb, INT16 w ) ;
      INT32 _recycleSeqObjects( utilRecycleItem &item, pmdEDUCB *cb, INT16 w ) ;
      INT32 _recycleIdxObjects( utilRecycleItem &item, pmdEDUCB *cb, INT16 w ) ;

      INT32 _setCSRecycled( const utilRecycleItem &item,
                            pmdEDUCB *cb,
                            INT16 w ) ;
      INT32 _unsetCSRecycled( const utilRecycleItem &item,
                              pmdEDUCB *cb,
                              INT16 w ) ;

      INT32 _tryFindOldestItem( pmdEDUCB *cb,
                                utilRecycleItem &oldestItem ) ;
      INT32 _tryFindOldestItem( const CHAR *originName,
                                utilCLUniqueID originID,
                                pmdEDUCB *cb,
                                utilRecycleItem &oldestItem ) ;
      INT32 _getOldestItem( const CHAR *originName,
                            pmdEDUCB *cb,
                            utilRecycleItem &oldestItem ) ;
      INT32 _getOldestItem( utilCLUniqueID originID,
                            pmdEDUCB *cb,
                            utilRecycleItem &oldestItem ) ;
      INT32 _getOldestItem( const bson::BSONObj &matcher,
                            const bson::BSONObj &hint,
                            pmdEDUCB *cb,
                            utilRecycleItem &oldestItem ) ;

      INT32 _saveObject( const CHAR *collection,
                         const bson::BSONObj &object,
                         BOOLEAN canReplace,
                         pmdEDUCB *cb,
                         INT16 w ) ;

      INT32 _processObjects( catRecycleBinProcessor &processor,
                             const bson::BSONObj &matcher,
                             pmdEDUCB *cb,
                             INT16 w ) ;

      INT32 _getItemsInCS( utilCSUniqueID csUniqueID,
                           pmdEDUCB *cb,
                           UTIL_RECY_ITEM_LIST &itemList ) ;

      INT32 _checkDelayLockFailed( const utilRecycleBinConf &conf ) ;

   protected:
      // cache of recycle bin information
      // reserved recycle item counts during any contexts with recycle items
      // which is not added to recycle bin yet
      UINT32               _reservedCount ;
      // exclude drop item and return item
      // WARNING: drop item background job is in another thread which can not
      //          use level lock, use this latch instead
      ossSpinXLatch        _dropLatch ;
   } ;

   typedef class _catRecycleBinManager catRecycleBinManager ;

}

#endif // CAT_RECYCLE_BIN_MANAGER_HPP__
