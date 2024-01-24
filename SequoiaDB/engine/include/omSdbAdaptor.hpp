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

   Source File Name = omSdbAdaptor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_SDB_PROXY_HPP__
#define OM_SDB_PROXY_HPP__

#include "pmdEDU.hpp"
#include <string>
#include <vector>
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{
   /*
      _omSdbNodeInfo define
   */
   struct _omSdbNodeInfo
   {
      string            _host ;
      string            _svcname ;
      string            _userName ;
      string            _password ;
   } ;
   typedef _omSdbNodeInfo omSdbNodeInfo ;
   typedef vector< omSdbNodeInfo >        VEC_SDBNODES ;

   /*
      _omBizInfo define
   */
   struct _omBizInfo
   {
      string            _clsName ;
      string            _bizName ;
      string            _userName ;
      string            _password ;
      VEC_SDBNODES      _vecNodes ;
   } ;
   typedef _omBizInfo omBizInfo ;

   /*
      _omSdbAdaptor define
   */
   class _omSdbAdaptor : public SDBObject
   {
      public:
         _omSdbAdaptor() ;
         ~_omSdbAdaptor() ;

         INT32    notifyStrategyChanged( const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb ) ;

      protected:

         INT32    getBizNodeInfo( omBizInfo &bizInfo,
                                  pmdEDUCB *cb ) ;

         INT32    notifyStrategyChange2Node( const omSdbNodeInfo *nodeInfo,
                                             const string &userName,
                                             const string &password,
                                             pmdEDUCB *cb ) ;

      private:

         INT32    _parseCoordNodeInfo( const BSONObj &obj,
                                       omSdbNodeInfo &nodeInfo ) ;

         INT32    _fillAuthInfo( omBizInfo &bizInfo,
                                 pmdEDUCB *cb ) ;

         INT32    _parseAuthInfo( const BSONObj &obj,
                                  string &userName,
                                  string &password ) ;

   } ;
   typedef _omSdbAdaptor omSdbAdaptor ;

}

#endif //OM_SDB_PROXY_HPP__
