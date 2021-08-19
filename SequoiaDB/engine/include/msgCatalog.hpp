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

   Source File Name = msgCatalog.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MSGCATALOG_HPP__
#define MSGCATALOG_HPP__

#pragma warning( disable: 4200 )

#include "core.hpp"
#include <map>
#include <string>
#include "../bson/bsonelement.h"
#include "oss.hpp"
#include "msg.hpp"
#include "netDef.hpp"
#include "dpsLogDef.hpp"
#include "clsDef.hpp"
using namespace std ;

#include "msgCatalogDef.h"

#define CAT_INVALID_VERSION               -1

#pragma pack(4)

namespace engine
{

   /************************************************
   * node register request message
   * data is a BsonObject:
   * {"Role":0, "Host":"vmsvr1", "dbpath":"...",
   *    "Service":[{"Type":0,"Name":"cat"},
   *               {"Type":1,"Name":"repl"},
   *               {"Type":2,"Name":"shard"},
   *               {"Type":3,"Name":"cata"}],
   *    "IP":["xxxx",...,"127.0.0.1","localhost"]
   * }
   ************************************************/
   class _MsgCatRegisterMessage : public SDBObject
   {
   public :
      MsgHeader      header;
      BYTE           data[0];
      _MsgCatRegisterMessage()
      {
         header.messageLength = 0 ;
         header.TID = 0 ;
         header.opCode = MSG_CAT_REG_REQ ;
         header.requestID = 0 ;
         header.routeID.value = 0 ;
      }
   private :
      _MsgCatRegisterMessage ( _MsgCatRegisterMessage const & ) ;
      _MsgCatRegisterMessage& operator=(_MsgCatRegisterMessage const & ) ;
   } ;
   typedef _MsgCatRegisterMessage      MsgCatRegisterReq;

   /************************************************
   * response message of node register
   * the data is a BsonObject:
   * {"Role":0, "GroupID":1000, "GroupName":"db1", "HostName":"xxx",
   *  "NodeID":1000, "dbpath":"xxxx",
   *    "Service":[{"Type":0,"Name":"cat"},
   *               {"Type":1,"Name":"repl"},
   *               {"Type":2, "Name":"shard"}]}
   ************************************************/
   /*class _MsgCatRegisterRspV0 : public SDBObject
   {
   public :
      _MsgInternalReplyHeader    header;
      BYTE                       data[0];
      _MsgCatRegisterRspV0()
      {
         header.header.messageLength = 0 ;
         header.header.TID = 0 ;
         header.header.opCode = MSG_CAT_REG_RES ;
         header.header.requestID = 0 ;
         header.header.routeID.value = 0 ;
      }
   private :
      _MsgCatRegisterRspV0 ( _MsgCatRegisterRspV0 const & ) ;
      _MsgCatRegisterRspV0& operator=( _MsgCatRegisterRspV0 const & ) ;
   } ; */
   typedef MsgOpReply                  MsgCatRegisterRsp ;

   /// download group info
   /// may be :| -- _MsgCatGroupReq -- | -- char *name -- |
   //  or : | -- _MsgCatGroupReq -- |
   //  check msg len.
   class _MsgCatGroupReq : public SDBObject
   {
   public :
      MsgHeader      header;
      MsgRouteID     id ;
      _MsgCatGroupReq()
      {
         header.messageLength = sizeof( _MsgCatGroupReq ) ;
         header.opCode = MSG_CAT_GRP_REQ ;
         header.requestID = 0 ;
         header.routeID.value = 0 ;
         header.TID = 0 ;
      }
   private :
      _MsgCatGroupReq ( _MsgCatGroupReq const & ) ;
      _MsgCatGroupReq& operator=( _MsgCatGroupReq const & ) ;
   } ;
   typedef _MsgCatGroupReq       MsgCatGroupReq ;

   /// {"GroupID":2000, "Role":0,"Version":0, "PrimaryNode":1
   ///  "Group":[{"NodeID":"001", "Host":"vmsrv1","Service":
   ///                                            [{"Type":0, "Name":"repl1"},
   ///                                             {"Type":1, "Name":"Shard1"},
   ///                                             {"Type":2, "Name":"cat1"}]}]}
   /* class _MsgCatGroupResV0 : public SDBObject
   {
   public :
      MsgInternalReplyHeader header ;
      _MsgCatGroupResV0()
      {
         header.header.opCode = MSG_CAT_GRP_RES ;
         header.header.TID = 0 ;
         header.header.requestID = 0 ;
         header.header.routeID.value = 0 ;
      }
   private :
      _MsgCatGroupResV0 ( _MsgCatGroupResV0 const & ) ;
      _MsgCatGroupResV0& operator=( _MsgCatGroupResV0 const & ) ;
   } ; */
   typedef MsgOpReply            MsgCatGroupRes ;

   INT32 msgParseCatGroupRes( const MsgCatGroupRes *msg,
                              CLS_GROUP_VERSION &version,
                              string &groupName,
                              map<UINT64, _netRouteNode> &group,
                              UINT32 *pPrimary = NULL,
                              UINT32 *pSecID = NULL ) ;

   INT32 msgParseCatGroupObj( const CHAR* objdata,
                              CLS_GROUP_VERSION &version,
                              UINT32 &groupID,
                              string &groupName,
                              map<UINT64, _netRouteNode> &group,
                              UINT32 *pPrimary = NULL,
                              UINT32 *pSecID = NULL ) ;

   const CHAR* getServiceName ( const bson::BSONElement &beService,
                                INT32 serviceType ) ;
   const CHAR* getShardServiceName ( const bson::BSONElement &beService ) ;

   //down catalog group info
   typedef MsgCatGroupReq        MsgCatCatGroupReq ;
   typedef MsgOpReply            MsgCatCatGroupRes ;

   class _MsgCatPrimaryChange : public SDBObject
   {
   public :
      MsgHeader      header;
//      DPS_LSN        newPrimaryLsn ;
      MsgRouteID     newPrimary ;
//      DPS_LSN        oldPrimaryLsn ;
      MsgRouteID     oldPrimary ;
      _MsgCatPrimaryChange()
      {
         header.messageLength = sizeof( _MsgCatPrimaryChange ) ;
         header.opCode = MSG_CAT_PAIMARY_CHANGE ;
         header.routeID.value = 0 ;
         header.requestID = 0 ;
         header.TID = 0 ;
         newPrimary.value = MSG_INVALID_ROUTEID ;
         oldPrimary.value = MSG_INVALID_ROUTEID ;
      }
   private :
      _MsgCatPrimaryChange ( _MsgCatPrimaryChange const & ) ;
      _MsgCatPrimaryChange& operator=( _MsgCatPrimaryChange const & ) ;
   } ;
   typedef _MsgCatPrimaryChange        MsgCatPrimaryChange ;

   /*class _MsgCatPrimaryChangeResV0 : public SDBObject
   {
   public :
      MsgInternalReplyHeader header ;
      _MsgCatPrimaryChangeResV0()
      {
         header.header.messageLength = sizeof( _MsgCatPrimaryChangeResV0 ) ;
         header.header.opCode = MSG_CAT_PAIMARY_CHANGE_RES ;
         header.header.TID = 0 ;
         header.header.requestID = 0 ;
         header.header.routeID.value = 0 ;
      }
   private :
      _MsgCatPrimaryChangeResV0 ( _MsgCatPrimaryChangeResV0 const & ) ;
      _MsgCatPrimaryChangeResV0& operator=( _MsgCatPrimaryChangeResV0 const & ) ;
   } ; */
   typedef MsgOpReply                  MsgCatPrimaryChangeRes ;

   typedef MsgOpQuery   MsgCatQueryCatReq;

   // the reply take a catalogue record which is a bson-obj:
   // {  name: "SpaceName.CollectionName", Version: 1,
   //    ShardingKey: { Key1: 1, Key2: -1 },
   //    CataInfo:
   //       [ { GroupID: 1000, LowBound:{"":MinKey,"":MaxKey }, UpBound:{"":Key1Value,"":Key2Value} },
   //         { GroupID: 1001, LowBound:{"":Key1Value,"":Key2Value}, UpBound:{"":MaxKey,"":MinKey } } ]
   typedef MsgOpReply   MsgCatQueryCatRsp;

   enum SDB_CAT_GROUP_STATUS
   {
      SDB_CAT_GRP_DEACTIVE = 0,
      SDB_CAT_GRP_ACTIVE
   } ;

   typedef MsgOpQuery MsgCatQueryTaskReq ;
   typedef MsgOpReply MsgCatQueryTaskRes ;

}
#pragma pack()

#endif // MSGCATALOG_HPP__
