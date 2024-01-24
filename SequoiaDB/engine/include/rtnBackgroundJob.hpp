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

   Source File Name = rtnBackgroundJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/06/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_BACKGROUND_JOB_HPP_
#define RTN_BACKGROUND_JOB_HPP_

#include "rtnBackgroundJobBase.hpp"
#include "dms.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsCB.hpp"
#include "dmsTaskStatus.hpp"
#include "pmdDummySession.hpp"
#include <string>

#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   /*
      _rtnIndexJob define
   */
   class _rtnIndexJob : public _rtnBaseJob
   {
      public:
         // slave node use it
         _rtnIndexJob ( RTN_JOB_TYPE type,
                        const CHAR *pCLName,
                        const BSONObj &indexObj,
                        SDB_DPSCB *dpsCB,
                        UINT64 lsnOffset,
                        BOOLEAN isRollBackLog,
                        INT32 sortBufSize,
                        UINT64 taskID,
                        UINT64 mainTaskID ) ;

         // master node use it
         _rtnIndexJob () ;

         virtual ~_rtnIndexJob() ;

         virtual INT32 init () ;
         const CHAR* getIndexName () const ;
         const CHAR* getCollectionName() const ;
         utilCLUniqueID getCLUniqueID() const ;
         UINT32 getCSLID() const ;
         UINT32 getCLLID() const ;

         static INT32 checkIndexExist( const CHAR *pCLName,
                                       const CHAR *pIdxName,
                                       BOOLEAN &hasExist ) ;

         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         virtual void _onAttach() ;
         virtual void _onDetach() ;

         virtual INT32 _onDoit( INT32 resultCode ) { return SDB_OK ; }
         INT32 _buildJobName() ;
         virtual BOOLEAN _needRetry( INT32 rc, BOOLEAN &retryLater ) ;

      protected:
         _pmdDummySession        _session ;
         RTN_JOB_TYPE            _type ;
         CHAR                    _clFullName[DMS_COLLECTION_FULL_NAME_SZ + 1] ;
         utilCLUniqueID          _clUniqID ;
         std::string             _indexName ;
         std::string             _jobName ;
         BSONObj                 _indexObj ;
         BSONElement             _indexEle ; // This is for index dropping
         BOOLEAN                 _hasAddUnique ;
         BOOLEAN                 _hasAddGlobal ;
         UINT32                  _csLID ;
         UINT32                  _clLID ;
         SDB_DPSCB*              _dpsCB ;
         SDB_DMSCB*              _dmsCB ;
         UINT64                  _lsn ;
         BOOLEAN                 _isRollbackLog ;
         BOOLEAN                 _regCLJob ;
         INT32                   _sortBufSize ;
         dmsIdxTaskStatusPtr     _taskStatusPtr ;
         UINT64                  _taskID ;
         UINT32                  _locationID ;
         UINT64                  _mainTaskID ;
   };
   typedef _rtnIndexJob rtnIndexJob ;

   /*
      _rtnIndexJobHolder define
    */
   class _rtnIndexJobHolder : public utilPooledObject
   {
   public:
      _rtnIndexJobHolder() ;
      ~_rtnIndexJobHolder() ;

      // register collection index job
      INT32 regCLJob( utilCLUniqueID clUID, RTN_JOB_TYPE type ) ;

      // unregister collection index job
      void  unregCLJob( utilCLUniqueID clUID, RTN_JOB_TYPE type ) ;

      // check whether has collection jobs
      BOOLEAN hasCLJobs( utilCLUniqueID clUID, RTN_JOB_TYPE type ) ;
      BOOLEAN hasCLJobs( utilCLUniqueID clUID ) ;

      // has collection space job
      BOOLEAN hasCSJobs( utilCSUniqueID csUID, RTN_JOB_TYPE type ) ;
      BOOLEAN hasCSJobs( utilCSUniqueID csUID ) ;

      void waitForCLJobs( utilCLUniqueID clUID, RTN_JOB_TYPE type ) ;
      void waitForCLJobs( utilCLUniqueID clUID ) ;
      void waitForCLJobs( const CHAR *clName, RTN_JOB_TYPE type ) ;
      void waitForCLJobs( const CHAR *clName ) ;

      void waitForCSJobs( utilCSUniqueID csUID, RTN_JOB_TYPE type ) ;
      void waitForCSJobs( utilCSUniqueID csUID ) ;
      void waitForCSJobs( const CHAR *csName, RTN_JOB_TYPE type ) ;
      void waitForCSJobs( const CHAR *csName ) ;

      // clear job holder
      void fini() ;

   protected:
      INT32 _getCLUID( const CHAR *clName, utilCLUniqueID &clUID ) ;
      INT32 _getCSUID( const CHAR *csName, utilCSUniqueID &csUID ) ;

   protected:
      struct _rtnCLJobCount : public _utilPooledObject
      {
         _rtnCLJobCount()
         : crtIdxJobCount( 0 ),
           dropIdxJobCount( 0 )
         {
         }

         void incJobCount( RTN_JOB_TYPE type )
         {
            if ( RTN_JOB_CREATE_INDEX == type )
            {
               ++ crtIdxJobCount ;
            }
            else if ( RTN_JOB_DROP_INDEX == type )
            {
               ++ dropIdxJobCount ;
            }
         }

         void decJobCount( RTN_JOB_TYPE type )
         {
            if ( RTN_JOB_CREATE_INDEX == type )
            {
               SDB_ASSERT( crtIdxJobCount > 0, "should have create index job" ) ;
               if ( crtIdxJobCount > 0 )
               {
                  -- crtIdxJobCount ;
               }
            }
            else if ( RTN_JOB_DROP_INDEX == type )
            {
               SDB_ASSERT( dropIdxJobCount > 0, "should have create index job" ) ;
               if ( dropIdxJobCount > 0 )
               {
                  -- dropIdxJobCount ;
               }
            }
         }

         BOOLEAN hasJobs( RTN_JOB_TYPE type )
         {
            if ( RTN_JOB_CREATE_INDEX == type )
            {
               return ( 0 != crtIdxJobCount ) ;
            }
            else if ( RTN_JOB_DROP_INDEX == type )
            {
               return ( 0 != dropIdxJobCount ) ;
            }
            else
            {
               return FALSE ;
            }
         }

         BOOLEAN hasJobs() const
         {
            return  ( 0 != crtIdxJobCount ) ||
                    ( 0 != dropIdxJobCount ) ;
         }

         // count of create index jobs
         UINT32         crtIdxJobCount ;
         // count of drop index jobs
         UINT32         dropIdxJobCount ;
      } ;
      typedef ossPoolMap< utilCLUniqueID, _rtnCLJobCount > _rtnCLJobMap ;
      typedef _rtnCLJobMap::iterator _rtnCLJobMapIter ;

   protected:
      // map latch
      ossSpinSLatch _mapLatch ;
      // index jobs
      _rtnCLJobMap  _clJobs ;
   } ;

   typedef class _rtnIndexJobHolder rtnIndexJobHolder ;

   rtnIndexJobHolder *rtnGetIndexJobHolder() ;

   /*
      _rtnCleanupIdxStatusJob define
   */
   class _rtnCleanupIdxStatusJob : public _utilLightJob
   {
      public:
         _rtnCleanupIdxStatusJob() {}
         virtual ~_rtnCleanupIdxStatusJob() {}

      public:
         virtual const CHAR* name() const ;
         virtual INT32 doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime ) ;
   };
   typedef _rtnCleanupIdxStatusJob rtnCleanupIdxStatusJob ;

   INT32 rtnStartCleanupIdxStatusJob() ;

   /*
      _rtnLoadJob define
   */
   class _rtnLoadJob : public _rtnBaseJob
   {
      public:
         _rtnLoadJob() {}
         virtual ~_rtnLoadJob() {}

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;
   };
   typedef _rtnLoadJob rtnLoadJob ;

   INT32 rtnStartLoadJob() ;

   typedef void (*RTN_ON_REBUILD_DONE_FUNC)( INT32 rc ) ;
   /*
      _rtnRebuildJob define
   */
   class _rtnRebuildJob : public _rtnBaseJob
   {
      public:
         _rtnRebuildJob() ;
         virtual ~_rtnRebuildJob() ;
      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         void    setInfo( RTN_ON_REBUILD_DONE_FUNC pFunc = NULL ) ;

     private:
         RTN_ON_REBUILD_DONE_FUNC   _pFunc ;
   } ;
   typedef _rtnRebuildJob rtnRebuildJob ;

   INT32    rtnStartRebuildJob( RTN_ON_REBUILD_DONE_FUNC pFunc = NULL ) ;

}

#endif //RTN_BACKGROUND_JOB_HPP_

