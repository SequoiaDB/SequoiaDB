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

   Source File Name = omagent.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_HPP_
#define OMAGENT_HPP_

#include "core.hpp"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "sptApi.hpp"
#include "omagentMsgDef.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      add host
   */
   struct _AddHostCommon
   {
      string _sdbUser ;
      string _sdbPasswd ;
      string _userGroup ;
      string _installPacket ;
   } ;
   typedef struct _AddHostCommon AddHostCommon ;

   struct _AddHostItem
   {
      string _ip ;
      string _hostName ;
      string _user ;
      string _passwd ;
      string _sshPort ;
      string _agentService ;
      string _installPath ;
      string _version ;
   } ;
   typedef struct _AddHostItem AddHostItem ;
   
   struct _AddHostResultInfo
   {
      string         _ip ;
      string         _hostName ;
      INT32          _status ;
      string         _statusDesc ;
      INT32          _errno ;
      string         _detail ;
      string         _version ;
      vector<string> _flow ;
   } ;
   typedef struct _AddHostResultInfo AddHostResultInfo ;
   typedef AddHostResultInfo RemoveHostResultInfo ;

   struct _AddHostInfo
   {
      INT32             _serialNum ;
      BOOLEAN           _flag ;   // whether the host has been handled or not
      INT64             _taskID ;
      AddHostCommon     _common ; // add host common field
      AddHostItem       _item ;   // add host info
   } ;
   typedef struct _AddHostInfo AddHostInfo ;

   /*
      remove host
   */
   struct _RemoveHostItem
   {
      string _ip ;
      string _hostName ;
      string _user ;
      string _passwd ;
      string _sshPort ;
      string _clusterName ;
      BSONObj _packages ;
   } ;
   typedef struct _RemoveHostItem RemoveHostItem ;

   struct _RemoveHostInfo
   {
      INT32             _serialNum ;
      INT64             _taskID ;
      RemoveHostItem    _item ;
   } ;
   typedef struct _RemoveHostInfo RemoveHostInfo ;

   /*
      install db business host
   */
   struct _InstDBInfo
   {
      string _hostName ;
      string _svcName ;
      string _dbPath ;
      string _confPath ;
      string _dataGroupName ;
      string _sdbUser ;
      string _sdbPasswd ;
      string _sdbUserGroup ;
      string _user ;
      string _passwd ;
      string _sshPort ;
      BSONObj _conf ;
   } ;
   typedef struct _InstDBInfo InstDBInfo ;

   struct _InstDBResult
   {
      INT32          _errno ;
      string         _detail ;
      string         _hostName ;
      string         _svcName ;
      string         _role ;
      string         _groupName ;
      INT32          _status ;
      string         _statusDesc ;
      vector<string> _flow ;
   } ;
   typedef struct _InstDBResult InstDBResult ;
   typedef InstDBResult RemoveDBResult ;

   struct _InstDBBusInfo
   {
      INT32          _nodeSerialNum ;
      InstDBInfo     _instInfo ;
      InstDBResult   _instResult ;
   } ;
   typedef struct _InstDBBusInfo InstDBBusInfo ;


   /*
      remove db business
   */
   struct _RemoveDBInfo
   {
      string _hostName ;
      string _svcName ;
      string _role ;
      string _dataGroupName ;
      string _authUser ;
      string _authPasswd ;
   } ;
   typedef struct _RemoveDBInfo RemoveDBInfo ;

   struct _RemoveDBBusInfo
   {
      INT32            _nodeSerialNum ;
      RemoveDBInfo     _removeInfo ;
      RemoveDBResult   _removeResult ;
   } ;
   typedef _RemoveDBBusInfo RemoveDBBusInfo ;

   /*
      add zookeeper
   */
   struct _AddZNCommon
   {
      string         _clusterName ;
      string         _businessName ;
      string         _deployMod ;
      string         _sdbUser ;
      string         _sdbPasswd ;
      string         _userGroup ;
      string         _installPacket ;
      vector<string> _serverInfo ;
   } ;
   typedef struct _AddZNCommon AddZNCommon ;
   
   struct _AddZNItem
   {
      string _hostName ;
      string _user ;
      string _passwd ;
      string _sshPort ;
      string _installPath ;
      string _dataPath ;
      string _dataPort ;
      string _electPort ;
      string _clientPort ;
      string _syncLimit ;
      string _initLimit ;
      string _tickTime ;
      string _zooid ;      
   } ;
   typedef struct _AddZNItem AddZNItem ;

   struct _AddZNInfo
   {
      INT32       _serialNum ;
      BOOLEAN     _flag ;   // whether the znode has been handled or not
      INT64       _taskID ;
      AddZNCommon _common ; // common field
      AddZNItem   _item ;   // znode's conf info
   } ;
   typedef struct _AddZNInfo AddZNInfo ;
   typedef AddZNInfo RemoveZNInfo ;
   typedef AddZNInfo CheckZNInfo ;
   typedef AddZNInfo ZNInfo ;

   struct _AddZNResultInfo
   {
      string         _hostName ;
      string         _zooid ;
      INT32          _status ;
      string         _statusDesc ;
      INT32          _errno ;
      string         _detail ;
      vector<string> _flow ;
   } ;
   typedef struct _AddZNResultInfo AddZNResultInfo ;
   typedef AddZNResultInfo ZNResultInfo ;


   struct _SsqlExecInfo
   {
      INT64       _taskID ;
      string      _hostName ;
      string      _serviceName ;
      string      _sshUser ;
      string      _sshPasswd ;
      string      _installPath ;
      string      _dbUser ;
      string      _dbPasswd ;
      string      _dbName ;
      string      _sql ;
      string      _resultFormat ;
      _SsqlExecInfo()
      {
         _taskID       = 0 ;
         _hostName     = "" ;
         _serviceName  = "" ;
         _sshUser      = "" ;
         _sshPasswd    = "" ;
         _installPath  = "" ;
         _dbUser       = "" ;
         _dbPasswd     = "" ;
         _dbName       = "" ;
         _sql          = "" ;
         _resultFormat = "" ;
      }

      _SsqlExecInfo( const struct engine::_SsqlExecInfo &right )
      {
         _taskID       = right._taskID ;
         _hostName     = right._hostName ;
         _serviceName  = right._serviceName ;
         _sshUser      = right._sshUser ;
         _sshPasswd    = right._sshPasswd ;
         _installPath  = right._installPath ;
         _dbUser       = right._dbUser ;
         _dbPasswd     = right._dbPasswd ;
         _dbName       = right._dbName ;
         _sql          = right._sql ;
         _resultFormat = right._resultFormat ;
      }
   } ;

   typedef struct _SsqlExecInfo SsqlExecInfo ;
   
}





#endif // OMAGENT_HPP_
