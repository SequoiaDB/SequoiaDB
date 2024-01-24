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

   Source File Name = optPlanClearJob.hpp

   Descriptive Name = Optimizer Cached Plan Clear Job Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to clear
   cached access plans.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPTPLANCLEARJOB_HPP__
#define OPTPLANCLEARJOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   /*
    *  _optPlanClearJob define
    */
   class _optPlanClearJob : public _rtnBaseJob
   {
      public :
         _optPlanClearJob () ;

         virtual ~_optPlanClearJob () ;

      public :
         virtual RTN_JOB_TYPE type () const { return RTN_JOB_OPT_PLAN_CLEAR ; }

         virtual const CHAR* name () const { return "OptPlanClear" ; }

         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) { return FALSE ; }

         virtual INT32 doit () ;
   } ;

   typedef _optPlanClearJob optPlanClearJob ;

   INT32 startPlanClearJob ( EDUID *pEDUID ) ;

}

#endif //OPTPLANCLEARJOB_HPP__

