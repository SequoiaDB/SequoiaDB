/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = seAdptDBAssist.hpp

   Descriptive Name = Database assistant for search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/14/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_ASSIST_HPP__
#define SEADPT_ASSIST_HPP__

#include "msgMessage.hpp"
#include "netRouteAgent.hpp"
#include "netMsgHandler.hpp"
#include "clsCatalogAgent.hpp"

using namespace engine;

namespace seadapter
{
   typedef clsGroupItem    CataGroupInfo ;
   /**
    * Assistant for communication with data/cata nodes. It holds the
    * netRouteAgent object, and is responsible for sending message with it.
    * Note:
    * Do not share the object of this class among multiple threads.
    */
   class _seAdptDBAssist : public SDBObject
   {
   public:
      _seAdptDBAssist( _netMsgHandler *msgHandler ) ;
      virtual ~_seAdptDBAssist() ;

      INT32 addTimer( UINT32 millsec, _netTimeoutHandler *handler,
                      UINT32 &timerID ) ;

      INT32 removeTimer( UINT32 timerID ) ;

      /**
       * @brief Update route information of the corresponding data node.
       */
      INT32 updateDataNodeRoute( const MsgRouteID &id, const CHAR *host,
                                 const CHAR *service ) ;

      INT32 updateGroupInfo( const BSONObj &obj ) ;

      MsgRouteID dataNodeID() const
      {
         return _dataNodeID ;
      }

      INT32 changeCataPrimary( UINT16 nodeID ) ;

      void setDataNetHandle( NET_HANDLE handle ) ;

      void setCataNetHandle( NET_HANDLE handle ) ;

      void invalidNetHandle( NET_HANDLE handle ) ;

      BOOLEAN dataNetHandleValid() const
      {
         return ( MSG_INVALID_ROUTEID != _dataNetHandle ) ;
      }

      BOOLEAN cataNetHandleValid() const
      {
         return ( MSG_INVALID_ROUTEID != _cataNetHandle ) ;
      }

      netRouteAgent* routeAgent()
      {
         return &_routeAgent ;
      }

      INT32 queryOnDataNode( const CHAR *clName,
                             UINT64 reqID,
                             UINT32 tid,
                             INT32 flag = 0,
                             INT64 numToSkip = 0,
                             INT64 numToReturn = -1,
                             const BSONObj *query = NULL,
                             const BSONObj *selector = NULL,
                             const BSONObj *orderBy = NULL,
                             const BSONObj *hint = NULL,
                             IExecutor *cb = NULL ) ;

      INT32 queryOnCataNode( const CHAR *clName,
                             UINT64 reqID,
                             UINT32 tid,
                             INT64 numToSkip = 0,
                             INT64 numToReturn = -1,
                             const BSONObj *query = NULL,
                             const BSONObj *selector = NULL,
                             const BSONObj *orderBy = NULL,
                             const BSONObj *hint = NULL,
                             IExecutor *cb = NULL ) ;

      /**
       * @brief Query catalogue information of the specified collection.
       * @param selector Field selector of the result set.
       */
      INT32 queryCLCataInfo( const CHAR *clName, UINT64 reqID, UINT32 tid,
                             const BSONObj *selector = NULL,
                             IExecutor *cb = NULL ) ;

      INT32 doCmdOnDataNode( const CHAR *cmd, const BSONObj &argObj,
                             UINT64 reqID, UINT32 tid, IExecutor *cb = NULL ) ;

      INT32 sendToDataNode( const MsgHeader *msg ) ;

      INT32 sendMsg( const MsgHeader *msg, NET_HANDLE handle ) ;

   private:
      INT32 _updateRouteInfo() ;
      INT32 _sendToCataNode( const MsgHeader *msg ) ;

   private:
      netRouteAgent _routeAgent ;
      MsgRouteID _dataNodeID ;
      NET_HANDLE _dataNetHandle ;
      NET_HANDLE _cataNetHandle ;
      CataGroupInfo _cataGrpInfo ;
   } ;
   typedef _seAdptDBAssist seAdptDBAssist ;
}

#endif /* SEADPT_ASSIST_HPP__ */

