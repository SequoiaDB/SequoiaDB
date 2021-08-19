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

   Source File Name = dmsLightJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LIGHT_JOB_HPP__
#define DMS_LIGHT_JOB_HPP__

#include "utilLightJobBase.hpp"
#include "dms.hpp"

namespace engine
{

   /*
      _dmsDeleteRecordJob define
   */
   class _dmsDeleteRecordJob : public _utilLightJob
   {
      public:
         _dmsDeleteRecordJob( INT32 csID, UINT16 clID,
                              UINT32 csLID, UINT32 clLID,
                              const dmsRecordID &rid ) ;
         virtual ~_dmsDeleteRecordJob() ;

         virtual const CHAR*     name() const ;
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      protected:
         INT32             _csID ;
         UINT16            _clID ;
         UINT32            _csLID ;
         UINT32            _clLID ;
         dmsRecordID       _rid ;
   } ;
   typedef _dmsDeleteRecordJob dmsDeleteRecordJob ;

   void dmsStartAsyncDeleteRecord( INT32 csID, UINT16 clID,
                                   UINT32 csLID, UINT32 clLID,
                                   const dmsRecordID &rid) ;

}

#endif //DMS_LIGHT_JOB_HPP__

