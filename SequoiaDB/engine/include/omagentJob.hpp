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

   Source File Name = omagentJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_JOB_HPP_
#define OMAGENT_JOB_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "omagent.hpp"
#include "omagentTask.hpp"
#include "omagentSubTask.hpp"
#include "omagentBackgroundCmd.hpp"
#include "rtnBackgroundJob.hpp"
#include <string>
#include <vector>

using namespace bson ;

namespace engine
{

   /*
      omagent job
   */
   class _omagentJob : public _rtnBaseJob
   {
      public:
         _omagentJob ( omaTaskPtr pTask, const BSONObj &info, void *ptr ) ;
         virtual ~_omagentJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR*  name () const ;
         virtual BOOLEAN      muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32        doit () ;

      private:
         omaTaskPtr _taskPtr ;
         BSONObj    _info ;
         void       *_pointer ;
         string     _jobName ;
   } ;


   INT32 startOmagentJob ( OMA_TASK_TYPE taskType, INT64 taskID,
                           const BSONObj &info, omaTaskPtr &taskPtr,
                           void *ptr = NULL ) ;
   
   _omaTask* getTaskByType( OMA_TASK_TYPE taskType, INT64 taskID ) ;


}



#endif // OMAGENT_JOB_HPP_