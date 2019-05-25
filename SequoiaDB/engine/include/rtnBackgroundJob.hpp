/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
         _rtnIndexJob ( RTN_JOB_TYPE type, const CHAR *pCLName,
                        const BSONObj &indexObj, SDB_DPSCB *dpsCB,
                        UINT64 offset, BOOLEAN isRollBack ) ;

         virtual ~_rtnIndexJob() ;

         INT32 init () ;
         const CHAR* getIndexName () const ;
         const CHAR* getCollectionName() const ;

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
         SDB_DPSCB*              _dpsCB ;
         SDB_DMSCB*              _dmsCB ;
         UINT64                  _lsn ;
         BOOLEAN                 _isRollback ;

   };
   typedef _rtnIndexJob rtnIndexJob ;

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

