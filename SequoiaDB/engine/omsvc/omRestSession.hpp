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
         virtual INT32     _processMsg( HTTP_PARSE_COMMON command, 
                                        const CHAR *pFilePath ) ;

      protected:
         INT32             _processOMRestMsg( const CHAR *pFilePath ) ;
         INT32             _setSpecifyNode( const CHAR *pSdbHostName,
                                            const CHAR *pSdbSvcName,
                                            list<omNodeInfo> &nodeList ) ;
         INT32             _processSdbTransferMsg( restAdaptor *pAdaptor,
                                                const CHAR *pClusterName,
                                                const CHAR *pBusinessName ) ;
         INT32             _getBusinessAccessNode( const CHAR *pClusterName,
                                                   const CHAR *pBusinessName,
                                                   const CHAR *pSdbUser,
                                                   const CHAR *pSdbPasswd,
                                                  list<omNodeInfo> &nodeList ) ;
         INT32             _getBusinessAuth( const CHAR *pClusterName,
                                             const CHAR *pBusinessName,
                                             string &user, string &passwd ) ;
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

         INT32 _registerPlugin( restAdaptor *pAdaptor ) ;

      private:
         INT32 _actionGetFile( const CHAR *pFilePath ) ;

         INT32 _forwardPlugin( restAdaptor *pAdptor,
                               const string &businessType ) ;

         INT32 _actionCmd( const CHAR *pFilePath ) ;

         omRestCommandBase *_createCommand( const CHAR *pFilePath ) ;

   } ;
   typedef _omRestSession omRestSession ;

}

#endif //OM_REST_SESSION_HPP_



