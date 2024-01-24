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

   Source File Name = rtnDetectDeadlock.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/15/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_DETECT_DEADLOCK_HPP_
#define RTN_DETECT_DEADLOCK_HPP_

#include "rtnFetchBase.hpp"
#include "dpsDeadlockDetector.hpp"

using namespace bson ;

namespace engine
{
   typedef ossPoolList< BSONObj >             RTN_DEADLOCK_RESULT_LIST ;
   typedef RTN_DEADLOCK_RESULT_LIST::iterator RTN_DEADLOCK_RESULT_LIST_IT ;

   /*
      _rtnDetectDeadlock 
   */
   class _rtnDetectDeadlock : public _IRtnMonProcessor
   {
   public:
      _rtnDetectDeadlock() ;
      virtual ~_rtnDetectDeadlock() ;
   
      virtual INT32   pushIn( const BSONObj &obj ) ;
      virtual INT32   output( BSONObj &obj, BOOLEAN &hasOut ) ;

      virtual INT32   done( BOOLEAN &hasOut ) ;
      virtual BOOLEAN eof() const ;

   protected:
      SDB_ROLE                 _dbRole ;
      DPS_TRANS_WAIT_SET       _waitInfoSet ;
      DPS_TRANS_RELATEDID_MAP  _relatedIDMap ;
      RTN_DEADLOCK_RESULT_LIST _deadlockResList ;
      BOOLEAN                  _eof ;
      BOOLEAN                  _hasDone ;
   };
   typedef _rtnDetectDeadlock rtnDetectDeadlock ; 
   typedef utilSharePtr< _rtnDetectDeadlock > rtnDetectDeadlockPtr ;
}

#endif // RTN_DETECT_DEADLOCK_HPP_
