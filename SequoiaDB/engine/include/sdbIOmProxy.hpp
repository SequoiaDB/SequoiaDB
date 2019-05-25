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

   Source File Name = sdbIOmProxy.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_OMPROXY_HPP__
#define SDB_I_OMPROXY_HPP__

#include "rtnContextBuff.hpp"
#include "msg.h"
#include "pmdEDU.hpp"
#include "rtnQueryOptions.hpp"
#include <vector>
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _IOmProxy define
   */
   class _IOmProxy
   {
      public:
         _IOmProxy() {}
         virtual ~_IOmProxy() {}

      public:
         virtual INT32 queryOnOm( MsgHeader *pMsg,
                                  INT32 requestType,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf ) = 0 ;

         virtual INT32 queryOnOm( const rtnQueryOptions &options,
                                  pmdEDUCB *cb,
                                  SINT64 &contextID,
                                  rtnContextBuf *buf ) = 0 ;

         virtual INT32 queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                              pmdEDUCB *cb,
                                              vector<BSONObj> &objs,
                                              rtnContextBuf *buf ) = 0 ;

         virtual void  setOprTimeout( INT64 timeout ) = 0 ;

   } ;
   typedef _IOmProxy IOmProxy ;

}

#endif //SDB_I_OMPROXY_HPP__

