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

   Source File Name = catCtxEventHandler.hpp

   Descriptive Name = CATALOG context handler

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context of Catalog.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/15/2022  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_CTX_EVENT_HANDLER_HPP_
#define CAT_CTX_EVENT_HANDLER_HPP_

#include "catDef.hpp"
#include "catLevelLock.hpp"
#include "clsCatalogAgent.hpp"
#include "utilRecycleItem.hpp"
#include "pmd.hpp"

namespace engine
{

   // forward define
   class _catRecycleBinManager ;

   /*
      _catCtxEventHandler define
    */
   class _catCtxEventHandler
   {
   public:
      _catCtxEventHandler( catCtxLockMgr & lockMgr )
      : _lockMgr( lockMgr )
      {
      }

      virtual ~_catCtxEventHandler() {}

      virtual const CHAR *getName() const = 0 ;

      virtual INT32 parseQuery( const bson::BSONObj &boQuery,
                                _pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                  const CHAR *targetName,
                                  const bson::BSONObj &boTarget,
                                  _pmdEDUCB *cb,
                                  INT16 w )
      {
         return SDB_OK ;
      }

      virtual INT32 onExecuteEvent( SDB_EVENT_OCCUR_TYPE type,
                                    _pmdEDUCB *cb,
                                    INT16 w )
      {
         return SDB_OK ;
      }

      virtual INT32 onCommitEvent( SDB_EVENT_OCCUR_TYPE type,
                                   _pmdEDUCB *cb,
                                   INT16 w )
      {
         return SDB_OK ;
      }

      virtual INT32 onRollbackEvent( SDB_EVENT_OCCUR_TYPE type,
                                     _pmdEDUCB *cb,
                                     INT16 w )
      {
         return SDB_OK ;
      }

      virtual void onDeleteEvent()
      {
      }

      virtual INT32 buildP1Reply( bson::BSONObjBuilder &builder )
      {
         return SDB_OK ;
      }

      virtual INT32 buildP2Reply( bson::BSONObjBuilder &builder )
      {
         return SDB_OK ;
      }

      virtual INT32 buildPCReply( bson::BSONObjBuilder &builder )
      {
         return SDB_OK ;
      }

   protected:
      catCtxLockMgr & _lockMgr ;
   } ;
   typedef class _catCtxEventHandler catCtxEventHandler ;
   typedef ossPoolList< catCtxEventHandler * > CAT_CTX_EVENT_HANDLER_LIST ;
   typedef CAT_CTX_EVENT_HANDLER_LIST::iterator CAT_CTX_EVENT_HANDLER_LIST_IT ;

   /*
      _catCtxGroupHandler define
    */
   class _catCtxGroupHandler : public _catCtxEventHandler
   {
   public:
      _catCtxGroupHandler( catCtxLockMgr &lockMgr ) ;
      virtual ~_catCtxGroupHandler() ;

      virtual const CHAR *getName() const
      {
         return "group" ;
      }

      CAT_GROUP_SET &getGroupIDSet()
      {
         return _groupIDSet ;
      }

      BOOLEAN isGroupEmpty() const
      {
         return _groupIDSet.empty() ;
      }

      void setIgnoreNonExist( BOOLEAN ignoreNonExist )
      {
         _ignoreNonExist = ignoreNonExist ;
      }

      INT32 addGroup( UINT32 groupID ) ;
      INT32 addGroups( const bson::BSONObj &boCollection ) ;
      INT32 addGroups( const CAT_GROUP_SET &groupIDSet ) ;
      INT32 addGroups( const CAT_GROUP_LIST &groupIDList ) ;
      INT32 addGroups( const VEC_GROUP_ID &groupIDVec ) ;
      INT32 addGroupsInRecycleBin( utilCSUniqueID csUniqueID ) ;

      virtual INT32 onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                  const CHAR *targetName,
                                  const bson::BSONObj &boTarget,
                                  _pmdEDUCB *cb,
                                  INT16 w ) ;

      virtual INT32 buildP1Reply( bson::BSONObjBuilder &builder ) ;

   protected:
      CAT_GROUP_SET _groupIDSet ;
      BOOLEAN       _ignoreNonExist ;
   } ;

   typedef class _catCtxGroupHandler catCtxGroupHandler ;

   /*
      _catCtxGlobIdxHandler define
    */
   class _catCtxGlobIdxHandler : public _catCtxEventHandler
   {
   public:
      _catCtxGlobIdxHandler( catCtxLockMgr &lockMgr ) ;
      virtual ~_catCtxGlobIdxHandler() ;

      virtual const CHAR *getName() const
      {
         return "global index" ;
      }

      void setExclusedCSUniqueID( utilCSUniqueID excludedCSUniqueID )
      {
         _excludedCSUniqueID = excludedCSUniqueID ;
      }

      CAT_PAIR_CLNAME_ID_LIST &getGlobIdxCLList()
      {
         return _globIdxList ;
      }

      INT32 addGlobIdxs( const CAT_PAIR_CLNAME_ID_LIST &globIdxList ) ;
      INT32 addGlobIdxs( const CHAR *collectionName ) ;

      virtual INT32 buildP1Reply( bson::BSONObjBuilder &builder ) ;

   protected:
      // exclude global index collections from specified collection space
      utilCSUniqueID          _excludedCSUniqueID ;
      CAT_PAIR_CLNAME_ID_LIST _globIdxList ;
   } ;

   typedef class _catCtxGlobIdxHandler catCtxGlobIdxHandler ;

   /*
      _catCtxRecycleHelper define
    */
   class _catCtxRecycleHelper
   {
   protected:
      _catCtxRecycleHelper( UTIL_RECYCLE_TYPE type,
                            UTIL_RECYCLE_OPTYPE opType ) ;
      ~_catCtxRecycleHelper() ;

   protected:
      _catRecycleBinManager * _recycleBinMgr ;
      utilRecycleItem         _recycleItem ;
   } ;

   typedef class _catCtxRecycleHelper catCtxRecycleHelper ;

   /*
      _catCtxTaskHandler define
    */
   class _catCtxTaskHandler : public _catCtxEventHandler
   {
   public:
      _catCtxTaskHandler( catCtxLockMgr & lockMgr ) ;
      virtual ~_catCtxTaskHandler() ;

      virtual const CHAR *getName() const
      {
         return "task" ;
      }

      ossPoolSet< UINT64 > &getTaskSet()
      {
         return _taskSet ;
      }

      virtual INT32 buildP1Reply( bson::BSONObjBuilder &builder )
      {
         return _buildTaskReply( builder ) ;
      }

      virtual INT32 buildP2Reply( bson::BSONObjBuilder &builder )
      {
         return _buildTaskReply( builder ) ;
      }

   protected:
      INT32 _buildTaskReply( bson::BSONObjBuilder &builder ) ;
      INT32 _cancelTasks( _pmdEDUCB *cb, INT16 w ) ;

   protected:
      ossPoolSet< UINT64 > _taskSet ;
   } ;

   typedef class _catCtxTaskHandler catCtxTaskHandler ;

   /*
      _catRecyCtxTaskHandler define
    */
   class _catRecyCtxTaskHandler : public _catCtxTaskHandler
   {
   public:
      _catRecyCtxTaskHandler( catCtxLockMgr & lockMgr ) ;
      virtual ~_catRecyCtxTaskHandler() ;

      virtual const CHAR *getName() const
      {
         return "recycle task" ;
      }

      virtual INT32 onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                  const CHAR *targetName,
                                  const bson::BSONObj &boTarget,
                                  _pmdEDUCB *cb,
                                  INT16 w ) ;

      void setRecycleItem( const utilRecycleItem &item )
      {
         _recycleItem = item ;
      }

   protected:
      INT32 _checkTasks( _pmdEDUCB *cb, INT16 w ) ;

   protected:
      utilRecycleItem _recycleItem ;
   } ;

   typedef class _catRecyCtxTaskHandler catRecyCtxTaskHandler ;

   /*
      _catCtxRecycleHandler define
    */
   class _catCtxRecycleHandler : public _catCtxEventHandler,
                                 public _catCtxRecycleHelper
   {
   public:
      _catCtxRecycleHandler( UTIL_RECYCLE_TYPE type,
                             UTIL_RECYCLE_OPTYPE opType,
                             catRecyCtxTaskHandler &taskHandler,
                             catCtxLockMgr &lockMgr ) ;
      virtual ~_catCtxRecycleHandler() ;

      virtual const CHAR *getName() const
      {
         return "recycle" ;
      }

      virtual INT32 parseQuery( const bson::BSONObj &boQuery,
                                 _pmdEDUCB *cb ) ;

      virtual INT32 onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                  const CHAR *targetName,
                                  const bson::BSONObj &boTarget,
                                  _pmdEDUCB *cb,
                                  INT16 w ) ;

      virtual INT32 onExecuteEvent( SDB_EVENT_OCCUR_TYPE type,
                                    _pmdEDUCB *cb,
                                    INT16 w ) ;

      virtual void onDeleteEvent() ;

      virtual INT32 buildP1Reply( bson::BSONObjBuilder &builder ) ;

      INT32 _checkRecycle( _pmdEDUCB *cb, INT16 w ) ;
      INT32 _executeRecycle( _pmdEDUCB *cb, INT16 w ) ;
      INT32 _executeWithoutRecycle( _pmdEDUCB *cb, INT16 w ) ;

   protected:
      catRecyCtxTaskHandler & _taskHandler ;
      BOOLEAN              _isUseRecycleBin ;
      BOOLEAN              _isReservedItem ;
      UTIL_RECY_ITEM_LIST  _droppingItems ;
   } ;

   typedef class _catCtxRecycleHandler catCtxRecycleHandler ;

   /*
      _catRtrnCtxTaskHandler define
    */
   class _catRtrnCtxTaskHandler : public _catCtxTaskHandler
   {
   public:
      _catRtrnCtxTaskHandler( catCtxLockMgr & lockMgr ) ;
      virtual ~_catRtrnCtxTaskHandler() ;

      virtual INT32 onRollbackEvent( SDB_EVENT_OCCUR_TYPE type,
                                     _pmdEDUCB *cb,
                                     INT16 w ) ;

   } ;

   typedef class _catRtrnCtxTaskHandler catRtrnCtxTaskHandler ;

}

#endif // CAT_CTX_EVENT_HANDLER_HPP_
