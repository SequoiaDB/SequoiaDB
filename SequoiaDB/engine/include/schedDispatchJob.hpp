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

   Source File Name = schedDispatch.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SCHED_DISPATCH_JOB_HPP__
#define SCHED_DISPATCH_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   class _schedTaskAdapterBase ;
   class _pmdAsycSessionMgr ;

   /*
      _schedDispatchJob define
   */
   class _schedDispatchJob : public _rtnBaseJob
   {
      public:
         _schedDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                            _pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_schedDispatchJob() ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const { return TRUE ; }

      private:
         _schedTaskAdapterBase   *_pTaskAdapter ;
         _pmdAsycSessionMgr      *_pSessionMgr ;

   } ;
   typedef _schedDispatchJob schedDispatchJob ;

   /*
      Gloable Functions Define
   */
   INT32 schedStartDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                _pmdAsycSessionMgr *pSessionMgr,
                                EDUID *pEduID ) ;

}

#endif // SCHED_DISPATCH_JOB_HPP__
