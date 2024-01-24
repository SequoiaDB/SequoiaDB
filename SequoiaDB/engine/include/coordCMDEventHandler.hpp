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

   Source File Name = coordCMDEventHandler.hpp

   Descriptive Name = Coord Command Event Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_CMD_EVENT_HANDLER_HPP__
#define COORD_CMD_EVENT_HANDLER_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "utilRecycleReturnInfo.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDArguments define
   */
   class _coordCMDArguments : public SDBObject
   {
      public :
         _coordCMDArguments () { _pBuf = NULL ; }

         virtual ~_coordCMDArguments () {}

         /* A copy of the query object */
         BSONObj _boQuery ;

         /* Name of the catalog target to be updated */
         string _targetName ;

         /* ignore error return codes */
         SET_RC _ignoreRCList ;

         /* retry when error returned */
         SET_RC _retryRCList ;

         /* the return context buf pointer */
         rtnContextBuf *_pBuf ;

         CoordGroupList _groupList ;
   } ;
   typedef _coordCMDArguments coordCMDArguments ;

   /*
      _coordCMDEventHandler implement
    */
   class _coordCMDEventHandler
   {
   public:
      _coordCMDEventHandler() {}
      virtual ~_coordCMDEventHandler() {}

      virtual const CHAR *getName() const = 0 ;

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs )
      {
         return SDB_OK ;
      }

      virtual INT32 parseCatP2Return( coordCMDArguments *pArgs,
                                      const std::vector<bson::BSONObj> &cataObjs )
      {
         return SDB_OK ;
      }

      virtual BOOLEAN needRewriteDataMsg()
      {
         return FALSE ;
      }

      virtual INT32 rewriteDataMsg( bson::BSONObjBuilder &queryBuilder,
                                    bson::BSONObjBuilder &hintBuilder )
      {
         return SDB_OK ;
      }

      virtual INT32 onBeginEvent( coordResource *pResource,
                                  coordCMDArguments *pArgs,
                                  pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onCommitEvent( coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onRollbackEvent( coordResource *pResource,
                                     coordCMDArguments *pArgs,
                                     pmdEDUCB *cb )
      {
         return SDB_OK ;
      }
   } ;

   typedef class _coordCMDEventHandler coordCMDEventHandler ;
   typedef ossPoolList< coordCMDEventHandler * > COORD_CMD_EVENT_HANDLER_LIST ;
   typedef COORD_CMD_EVENT_HANDLER_LIST::iterator COORD_CMD_EVENT_HANDLER_LIST_IT ;

   typedef ossPoolList< ossPoolString >   COORD_GLOBIDXCL_NAME_LIST ;
   typedef COORD_GLOBIDXCL_NAME_LIST::iterator
                                          COORD_GLOBIDXCL_NAME_LIST_IT ;
   typedef COORD_GLOBIDXCL_NAME_LIST::const_iterator
                                          COORD_GLOBIDXCL_NAME_LIST_CIT ;

   /*
      _coordDataCMDHelper define
    */
   class _coordDataCMDHelper
   {
   public:
      _coordDataCMDHelper() {}
      ~_coordDataCMDHelper() {}

      INT32 dropCL( coordResource *resource,
                    const CHAR *clName,
                    BOOLEAN skipRecycleBin,
                    BOOLEAN ignoreLock,
                    pmdEDUCB *cb ) ;
      INT32 truncateCL( coordResource *resource,
                        const CHAR *clName,
                        BOOLEAN skipRecycleBin,
                        BOOLEAN ignoreLock,
                        pmdEDUCB *cb ) ;
      INT32 alterCL( coordResource *resource,
                     const CHAR *clName,
                     const bson::BSONObj &options,
                     pmdEDUCB *cb ) ;
      INT32 dropCS( coordResource *resource,
                    const CHAR *csName,
                    BOOLEAN skipRecycleBin,
                    BOOLEAN ignoreLock,
                    pmdEDUCB *cb ) ;
   } ;
   typedef class _coordDataCMDHelper coordDataCMDHelper ;

   /*
      _coordNodeCMDHelper
   */
   class _coordNodeCMDHelper
   {
   public:
      _coordNodeCMDHelper() {}
      ~_coordNodeCMDHelper() {}

      INT32 notify2GroupNodes( coordResource *pResource,
                               UINT32 groupID,
                               pmdEDUCB *cb ) ;

      INT32 notify2GroupNodes( coordResource *pResource,
                               const CHAR* groupName,
                               pmdEDUCB *cb ) ;

      INT32 notify2NodesByGroups( coordResource *pResource,
                                  const CoordGroupList &groupLst,
                                  pmdEDUCB *cb ) ;

      INT32 notify2AllNodes( coordResource *pResource,
                             BOOLEAN exceptSelf,
                             pmdEDUCB *cb ) ;

   private:
      INT32 _notify2GroupNodes( coordResource *pResource,
                                const CoordGroupInfoPtr &groupPtr,
                                pmdEDUCB *cb ) ;
   } ;
   typedef class _coordNodeCMDHelper coordNodeCMDHelper ;

   /*
      _coordCMDGlobIdxHandler define
    */
   class _coordCMDGlobIdxHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDGlobIdxHandler() {}
      virtual ~_coordCMDGlobIdxHandler() {}

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual INT32 onBeginEvent( coordResource *pResource,
                                  coordCMDArguments *pArgs,
                                  pmdEDUCB *cb ) ;

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _repairCheckGlobIdxCLs( coordResource *resource,
                                    BOOLEAN enableRepairCheck,
                                    pmdEDUCB *cb ) ;

   protected:
      COORD_GLOBIDXCL_NAME_LIST _globalIndexes ;
   } ;

   typedef class _coordCMDGlobIdxHandler coordCMDGlobIdxHandler ;

   /*
      _coordDropGlobIdxHandler define
    */
   class _coordDropGlobIdxHandler : public _coordCMDGlobIdxHandler
   {
   public:
      _coordDropGlobIdxHandler() {}
      virtual ~_coordDropGlobIdxHandler() {}

      virtual const CHAR *getName() const
      {
         return "drop global index" ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _dropGlobIdxCLs( coordResource *resource,
                             pmdEDUCB *cb ) ;
   } ;

   typedef class _coordDropGlobIdxHandler coordDropGlobIdxHandler ;

   /*
      _coordTruncGlobIdxHandler define
    */
   class _coordTruncGlobIdxHandler : public _coordCMDGlobIdxHandler
   {
   public:
      _coordTruncGlobIdxHandler() {}
      virtual ~_coordTruncGlobIdxHandler() {}

      virtual const CHAR *getName() const
      {
         return "truncate global index" ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _truncGlobIdxCLs( coordResource *resource,
                              pmdEDUCB *cb ) ;
   } ;

   typedef class _coordTruncGlobIdxHandler coordTruncGlobIdxHandler ;

   /*
      _coordCMDTaskHandler define
    */
   class _coordCMDTaskHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDTaskHandler() ;
      virtual ~_coordCMDTaskHandler() ;

      virtual const CHAR *getName() const
      {
         return "task" ;
      }

      virtual INT32 onBeginEvent( coordResource *resource,
                                  coordCMDArguments *arguments,
                                  pmdEDUCB *cb ) ;

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual INT32 parseCatP2Return( coordCMDArguments *pArgs,
                                      const std::vector<bson::BSONObj> &cataObjs ) ;

   protected:
      INT32 _parseTaskSet( const bson::BSONObj &cataObj ) ;
      INT32 _waitTasks( coordResource *resource,
                        BOOLEAN ignoreCanceled,
                        pmdEDUCB *cb ) ;
      INT32 _waitTask( coordResource *resource,
                       UINT64 taskID,
                       pmdEDUCB *cb ) ;
      INT32 _cancelTasks( coordResource *resource,
                         pmdEDUCB *cb ) ;
      INT32 _cancelTask( coordResource *resource,
                         UINT64 taskID,
                         pmdEDUCB *cb ) ;

   protected:
      ossPoolSet< UINT64 > _taskSet ;
   } ;

   typedef class _coordCMDTaskHandler coordCMDTaskHandler ;

   /*
      _coordCMDRecyTaskHandler define
    */
   // when recycle dropCS, dropCL, truncate, need wait for split tasks to
   // be finished or cancelled
   class _coordCMDRecyTaskHandler : public coordCMDTaskHandler
   {
   public:
      _coordCMDRecyTaskHandler() {}
      virtual ~_coordCMDRecyTaskHandler() {}

      virtual const CHAR *getName() const
      {
         return "recycle task" ;
      }

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;
   } ;

   typedef class _coordCMDRecyTaskHandler coordCMDRecyTaskHandler ;

   /*
      _coordCMDRecycleHandler define
    */
   class _coordCMDRecycleHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDRecycleHandler() {}
      virtual ~_coordCMDRecycleHandler() {}

      virtual const CHAR *getName() const
      {
         return "recycle" ;
      }

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual BOOLEAN needRewriteDataMsg()
      {
         return _recycleOptions.isEmpty() ? FALSE : TRUE ;
      }

      virtual INT32 rewriteDataMsg( bson::BSONObjBuilder &queryBuilder,
                                    bson::BSONObjBuilder &hintBuilder ) ;

      virtual INT32 onBeginEvent( coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

      const bson::BSONObj &getRecycleOptions() const
      {
         return _recycleOptions ;
      }

   protected:
      INT32 _dropRecycleItem( coordResource *resource,
                              const CHAR *recycleName,
                              BOOLEAN ignoreIfNotExists,
                              BOOLEAN isRecursive,
                              BOOLEAN isEnforced,
                              BOOLEAN ignoreLock,
                              pmdEDUCB *cb ) ;

      INT32 _dropRecycleItems( coordResource *resource,
                               BOOLEAN ignoreIfNotExists,
                               BOOLEAN isRecursive,
                               BOOLEAN isEnforced,
                               BOOLEAN ignoreLock,
                               pmdEDUCB *cb ) ;

   protected:
      bson::BSONObj            _recycleOptions ;
      UTIL_RECY_ITEM_NAME_LIST _droppingItems ;
   } ;

   typedef class _coordCMDRecycleHandler coordCMDRecycleHandler ;

   /*
      _coordCMDRtrnTaskHandler define
    */
   // when return recycle item, use return task handler to rebuild
   // indexes with external data, e.g. text indexes and global indexes
   class _coordCMDRtrnTaskHandler : public _coordCMDTaskHandler
   {
   public:
      _coordCMDRtrnTaskHandler() {}
      virtual ~_coordCMDRtrnTaskHandler() {}

      virtual const CHAR *getName() const
      {
         return "return task" ;
      }

      virtual INT32 onCommitEvent( coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

      virtual INT32 onRollbackEvent( coordResource *resource,
                                     coordCMDArguments *arguments,
                                     pmdEDUCB *cb ) ;
   } ;

   typedef class _coordCMDRtrnTaskHandler coordCMDRtrnTaskHandler ;

   /*
      _coordCMDReturnHandler define
    */
   class _coordCMDReturnHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDReturnHandler() {}
      virtual ~_coordCMDReturnHandler() {}

      virtual const CHAR *getName() const
      {
         return "return" ;
      }

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual BOOLEAN needRewriteDataMsg()
      {
         return ( ( _returnInfo.hasChangeUIDCL() ) ||
                  ( _returnInfo.hasRenameCL() ) ||
                  ( _returnInfo.hasRenameCS() ) ) ;
      }

      virtual INT32 rewriteDataMsg( bson::BSONObjBuilder &queryBuilder,
                                    bson::BSONObjBuilder &hintBuilder ) ;

      virtual INT32 onBeginEvent( coordResource *resource,
                                  coordCMDArguments *arguments,
                                  pmdEDUCB *cb ) ;

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

      const utilRecycleReturnInfo &getReturnInfo() const
      {
         return _returnInfo ;
      }

   protected:
      bson::BSONObj         _returnOptions ;
      utilRecycleReturnInfo _returnInfo ;
   } ;

   typedef class _coordCMDReturnHandler coordCMDReturnHandler ;

}

#endif // COORD_CMD_EVENT_HANDLER_HPP__
