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

   Source File Name = pmdProcessorBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/12/2014  Lin Youbin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_PROCESSORBASE_HPP_
#define PMD_PROCESSORBASE_HPP_

#include "oss.hpp"
#include "msg.h"
#include "sdbInterface.hpp"
#include "rtnContextBuff.hpp"

namespace engine
{
   /*
      SDB_PROCESSOR_TYPE define
   */
   enum SDB_PROCESSOR_TYPE
   {
      SDB_PROCESSOR_DATA        = 1,      // data node processor
      SDB_PROCESSOR_COORD,                // coord node processor
      SDB_PROCESSOR_CATA,
      SDB_PROCESSOR_OM,

      SDB_PROCESSOR_MAX         
   } ;

   /*
      _IProcessor define
   */
   class _IProcessor : public SDBObject
   {
      public:
         _IProcessor() {}
         virtual ~_IProcessor() {}

      public:

         virtual INT32                 processMsg( MsgHeader *msg,
                                                   rtnContextBuf &contextBuff,
                                                   INT64 &contextID,
                                                   BOOLEAN &needReply ) = 0 ;

         virtual const CHAR*           processorName() const = 0 ;
         virtual SDB_PROCESSOR_TYPE    processorType() const = 0 ;

         virtual ISession*             getSession() = 0 ;

      protected:
         virtual void                  _onAttach () {}
         virtual void                  _onDetach () {}

   } ;
   typedef _IProcessor IProcessor ;

}

#endif /*PMD_PROCESSORBASE_HPP_*/

