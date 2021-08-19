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

   Source File Name = rtnPrefetchJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/09/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_PREFETCH_JOB_HPP_
#define RTN_PREFETCH_JOB_HPP_

#include "rtnBackgroundJob.hpp"

#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   /*
      _rtnPrefetchJob define
   */
   class _rtnPrefetchJob : public _rtnBaseJob
   {
      public:
         _rtnPrefetchJob ( INT32 timeout = -1 ) ;
         virtual ~_rtnPrefetchJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      private:
         INT32    _timeout ;

   } ;
   typedef _rtnPrefetchJob  rtnPrefetchJob ;

   INT32 startPrefetchJob ( EDUID *pEDUID, INT32 timeout = -1 ) ;

}

#endif //RTN_PREFETCH_JOB_HPP_

