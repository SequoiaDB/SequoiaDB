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

   Source File Name = omRestSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/29/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_REST_SESSION_HPP_
#define OM_REST_SESSION_HPP_

#include "pmdRestSession.hpp"
#include "omCommandInterface.hpp"
#include "omTransferProcessor.hpp"

using namespace bson ;

namespace engine
{
   /*
      _omRestSession define
   */
   class _omRestSession : public _pmdRestSession
   {
      public:
         _omRestSession( SOCKET fd ) ;
         virtual ~_omRestSession() ;

         virtual SDB_SESSION_TYPE sessionType() const ;

      protected:
         virtual INT32     _processMsg( restRequest &request,
                                        restResponse &response ) ;

      protected:
         INT32             _processOMRestMsg( restRequest &request,
                                              restResponse &response ) ;

         INT32             _setSpecifyNode( const string &sdbHostName,
                                            const string &sdbSvcName,
                                            list<omNodeInfo> &nodeList ) ;

         INT32 _processSdbTransferMsg( restRequest &request,
                                       restResponse &response,
                                       const CHAR *pClusterName,
                                       const CHAR *pBusinessName ) ;

         INT32 _getBusinessAccessNode( restRequest &request,
                                       const CHAR *pClusterName,
                                       const CHAR *pBusinessName,
                                       list<omNodeInfo> &nodeList ) ;

         INT32             _getBusinessInfo( const CHAR *pClusterName,
                                             const CHAR *pBusinessName,
                                             string &businessType, 
                                             string &deployMode ) ;
         INT32             _queryTable( const string &tableName, 
                                        const BSONObj &selector, 
                                        const BSONObj &matcher,
                                        const BSONObj &order, 
                                        const BSONObj &hint, SINT32 flag,
                                        SINT64 numSkip, SINT64 numReturn, 
                                        list<BSONObj> &records ) ;
         BOOLEAN           _isClusterExist( const CHAR *pClusterName ) ;

         INT32 _registerPlugin( restRequest &request, restResponse &response ) ;

      private:
         INT32 _actionGetFile( restRequest &request,
                               restResponse &response ) ;

         INT32 _forwardPlugin( restRequest &request,
                               restResponse &response ) ;

         INT32 _actionCmd( restRequest &request,
                           restResponse &response ) ;

         omRestCommandBase* _createCommand( restRequest &request,
                                            restResponse &response ) ;

   } ;
   typedef _omRestSession omRestSession ;

}

#endif //OM_REST_SESSION_HPP_



