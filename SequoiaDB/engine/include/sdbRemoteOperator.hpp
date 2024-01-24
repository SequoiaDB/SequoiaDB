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

   Source File Name = sdbRemoteOperator.hpp

   Descriptive Name = Sequoiadb Remote operator Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/10/2020   LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_REMOTE_OPERATOR_HPP_
#define SDB_REMOTE_OPERATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "utilInsertResult.hpp"
#include "utilUniqueID.hpp"

using namespace bson ;

namespace engine
{
   class _sdbRemoteOpCtrl ;

   class _IRemoteOperator : public SDBObject
   {
   public:
      _IRemoteOperator() {}
      virtual ~_IRemoteOperator() {}

   public:
      virtual INT32           transBegin() = 0 ;

      virtual INT32           transCommit() = 0 ;

      virtual INT32           transRollback() = 0 ;

      virtual INT32           insert( const CHAR *clName,
                                      const BSONObj &insertor,
                                      INT32 flags,
                                      utilInsertResult *pResult = NULL ) = 0 ;

      virtual INT32           remove( const CHAR *clName,
                                      const BSONObj &matcher,
                                      const BSONObj &hint, INT32 flags,
                                      utilDeleteResult *pResult = NULL ) = 0 ;

      virtual INT32           update( const CHAR *clName,
                                      const BSONObj &matcher,
                                      const BSONObj &updator,
                                      const BSONObj &hint,
                                      INT32 flags,
                                      utilUpdateResult *pResult = NULL ) = 0 ;

      virtual INT32           truncateCL( const CHAR *clName ) = 0 ;

      virtual INT32           snapshot( INT64 &contextID,
                                        const CHAR *pCommand,
                                        const BSONObj &matcher = BSONObj(),
                                        const BSONObj &selector = BSONObj(),
                                        const BSONObj &orderBy = BSONObj(),
                                        const BSONObj &hint = BSONObj(),
                                        INT64 numToSkip = 0,
                                        INT64 numToReturn = -1,
                                        INT32 flag = 0 ) = 0 ;

      virtual INT32           snapshotIndexes( INT64 &contextID,
                                               const CHAR *clName,
                                               const CHAR *indexName,
                                               BOOLEAN rawData = FALSE ) = 0 ;

      virtual INT32           list( INT64 &contextID,
                                    const CHAR *pCommand,
                                    const BSONObj &matcher = BSONObj(),
                                    const BSONObj &selector = BSONObj(),
                                    const BSONObj &orderBy = BSONObj(),
                                    const BSONObj &hint = BSONObj(),
                                    INT64 numToSkip = 0,
                                    INT64 numToReturn = -1,
                                    INT32 flag = 0 ) = 0 ;

      virtual INT32           listCSIndexes( INT64 &contextID,
                                             utilCSUniqueID csUniqID ) = 0 ;

      virtual INT32           stopCriticalMode( const UINT32 &groupID ) = 0 ;

      virtual INT32           stopMaintenanceMode( const UINT32 &groupID,
                                                   const CHAR *pNodeName ) = 0 ;

      virtual UINT64          getSucCount() = 0 ;
      virtual UINT64          getFailureCount() = 0 ;

      virtual _sdbRemoteOpCtrl*  getController() = 0 ;
   } ;

   typedef _IRemoteOperator IRemoteOperator ;

   class _sdbRemoteOpCtrl : public SDBObject
   {
   public:
      _sdbRemoteOpCtrl()
      {
         reset() ;
      }

      virtual ~_sdbRemoteOpCtrl()
      {
      }

   public:
      // return TRUE if need to execute, FALSE if no need to execute
      BOOLEAN beforeExecute()
      {
         if ( _isEnabled && _isUndo && _count <= 0 )
         {
            return FALSE ;
         }

         return TRUE ;
      }

      void afterSucExecute()
      {
         if ( !_isEnabled )
         {
            // no need to record if disabled
            return ;
         }

         if ( !_isUndo )
         {
            ++_count ;
         }
         else
         {
            if ( _count > 0 )
            {
               --_count ;
            }
         }
      }

      BOOLEAN isUndoFinished()
      {
         if ( _isUndo )
         {
            return _count == 0 ? TRUE : FALSE ;
         }

         return TRUE ;
      }

      void reset()
      {
         _isEnabled = TRUE ;
         _isUndo = FALSE ;
         _count = 0 ;
      }

      void disable()
      {
         _isEnabled = FALSE ;
         _isUndo = FALSE ;
      }

      BOOLEAN isEnabled()
      {
         return _isEnabled ;
      }

      void switchToUndoState()
      {
         _isUndo = TRUE ;
      }

      BOOLEAN isUndo()
      {
         return _isUndo ;
      }

   private:
      BOOLEAN _isEnabled ;
      BOOLEAN _isUndo ;
      UINT64 _count ;
   } ;

   typedef _sdbRemoteOpCtrl sdbRemoteOpCtrl ;

   class _sdbRemoteOpCtrlAssist : public SDBObject
   {
   public:
      _sdbRemoteOpCtrlAssist( _sdbRemoteOpCtrl *opCtrl )
      {
         _opCtrl = opCtrl ;
         _opCtrl->reset() ;
      }

      ~_sdbRemoteOpCtrlAssist()
      {
         _opCtrl->disable() ;
         _opCtrl = NULL ;
      }

   public:
      void switchToUndo()
      {
         _opCtrl->switchToUndoState() ;
      }

      BOOLEAN isUndoFinished()
      {
         return _opCtrl->isUndoFinished() ;
      }

   private:
      _sdbRemoteOpCtrl *_opCtrl ;
   } ;

   class _sdbRemoteCountAssist : public SDBObject
   {
   public:
      _sdbRemoteCountAssist()
      {
         _sucCount = 0 ;
         _failureCount = 0 ;
      }

      ~_sdbRemoteCountAssist() {}

   public:
      void setCounts( UINT64 sucCount, UINT64 failureCount )
      {
         _sucCount = sucCount ;
         _failureCount = failureCount ;
      }

      BOOLEAN isExistFailure( UINT64 failureCount )
      {
         return failureCount == _failureCount ? FALSE : TRUE ;
      }

      BOOLEAN isPartialFailure( UINT64 sucCount, UINT64 failureCount )
      {
         UINT64 failureInc = failureCount > _failureCount ?
                             (failureCount - _failureCount) :
                             (_failureCount - failureCount) ;

         UINT64 sucInc = sucCount > _sucCount ? (sucCount - _sucCount) :
                                                (_sucCount - sucCount) ;

         if ( failureInc > 0 && sucInc > 0 )
         {
            return TRUE ;
         }

         return FALSE ;
      }

   private:
      UINT64 _sucCount ;
      UINT64 _failureCount ;
   } ;

   typedef _sdbRemoteCountAssist sdbRemoteCountAssist ;
}

#endif //SDB_REMOTE_OPERATOR_HPP_


