/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
         _rtnIndexJob ( RTN_JOB_TYPE type,
                        const CHAR *pCLName,
                        const BSONObj &indexObj, SDB_DPSCB *dpsCB,
                        UINT64 offset, BOOLEAN isRollBack ) ;

         virtual ~_rtnIndexJob() ;

         INT32 init () ;
         const CHAR* getIndexName () const ;
         const CHAR* getCollectionName() const ;

         static INT32 checkIndexExist( const CHAR *pCLName,
                                       const CHAR *pIdxName,
                                       BOOLEAN &hasExist ) ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         RTN_JOB_TYPE            _type ;
         CHAR                    _clFullName[DMS_COLLECTION_FULL_NAME_SZ + 1] ;
         std::string             _indexName ;
         std::string             _jobName ;
         BSONObj                 _indexObj ;
         BSONElement             _indexEle ;
         BOOLEAN                 _hasAddUnique ;
         UINT32                  _csLID ;
         UINT32                  _clLID ;
         SDB_DPSCB*              _dpsCB ;
         SDB_DMSCB*              _dmsCB ;
         UINT64                  _lsn ;
         BOOLEAN                 _isRollback ;
         BOOLEAN                 _regCLJob ;
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
      INT32 regCLJob( const CHAR *collection ) ;

      // unregister collection index job
      void  unregCLJob( const CHAR *collection ) ;

      // has collection job
      BOOLEAN hasCLJob( const CHAR *collection ) ;

      // clear job holder
      void fini() ;

   protected:
      void _unregCLJob( const ossPoolString &collection ) ;
      void _unregCLJobIter( const CHAR *collection ) ;
      BOOLEAN _hasCLJob( const ossPoolString &collection ) ;
      BOOLEAN _hasCLJobIter( const CHAR *collection ) ;

   protected:
      typedef ossPoolMap< ossPoolString, UINT32 > CL_JOB_MAP ;
      ossSpinSLatch  _mapLatch ;
      CL_JOB_MAP     _clJobs ;
   } ;

   typedef class _rtnIndexJobHolder rtnIndexJobHolder ;

   rtnIndexJobHolder *rtnGetIndexJobHolder() ;

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

   /*
      Global function define
   */
   INT32    rtnStartLoadJob() ;
   INT32    rtnStartRebuildJob( RTN_ON_REBUILD_DONE_FUNC pFunc = NULL ) ;

}

#endif //RTN_BACKGROUND_JOB_HPP_

