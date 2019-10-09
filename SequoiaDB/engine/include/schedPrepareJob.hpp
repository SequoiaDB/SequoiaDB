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

   Source File Name = schedPrepareJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SCHED_PREPARE_JOB_HPP__
#define SCHED_PREPARE_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   class _schedTaskAdapterBase ;

   /*
      _schedPrepareJob define
   */
   class _schedPrepareJob : public _rtnBaseJob
   {
      public:
         _schedPrepareJob( _schedTaskAdapterBase *pTaskAdapter ) ;
         virtual ~_schedPrepareJob() ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const { return TRUE ; }

      private:
         _schedTaskAdapterBase      *_pTaskAdapter ;

   } ;
   typedef _schedPrepareJob schedPrepareJob ;

   /*
      Gloable Functions Define
   */
   INT32 schedStartPrepareJob( _schedTaskAdapterBase *pTaskAdapter,
                               EDUID *pEduID ) ;

}

#endif // SCHED_PREPARE_JOB_HPP__
