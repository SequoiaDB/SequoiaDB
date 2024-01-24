/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordDSChecker.cpp

   Descriptive Name = Data source checker

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordDSChecker.hpp"
#include "coordRemoteConnection.hpp"
#include "utilAddress.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "netRoute.hpp"
#include "rtnContextBuff.hpp"
#include "coordTrace.hpp"

#define SDB_DS_VERSION_SIZE      (31)

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__INITDSCONNECTION, "_initDSConnection" )
   static INT32 _initDSConnection( const CHAR *addresses,
                                   const CHAR *user,
                                   const CHAR *password,
                                   pmdEDUCB *cb,
                                   coordRemoteConnection &connection )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__INITDSCONNECTION ) ;
      utilAddrContainer addrContainer ;

      SDB_ASSERT( addresses, "addresses is NULL" ) ;

      rc = utilParseAddrList( addresses, addrContainer ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse address[%s] failed[%d]",
                   addresses, rc ) ;

      {
         const _utilArray<_utilAddrPair>& addrArray =
               addrContainer.getAddresses() ;

         for ( UINT32 i = 0; i < addrArray.size(); ++i )
         {
            // Find the first address which can be connected.
            const utilAddrPair& addr = addrArray[ i ] ;
            rc = connection.init( addr.getHost(), addr.getService() ) ;
            if ( SDB_OK == rc )
            {
               break ;
            }
            // If the address cannot be connected, try the next one.
         }
         if ( !connection.isConnected() )
         {
            rc = SDB_NET_CANNOT_CONNECT ;
            PD_LOG( PDERROR, "None of the address[%s] can be connected[%d]",
                    addresses, rc ) ;
            goto error ;
         }

         if ( user && password )
         {
            rc = connection.authenticate( user, password, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Authentication failed on data "
                         "source[%d]", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__INITDSCONNECTION, rc ) ;
      return rc ;
   error:
      if ( connection.isConnected() )
      {
         connection.disconnect( cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSINFOCHECKER_CHECK, "_coordDSInfoChecker::check" )
   INT32 _coordDSInfoChecker::check( const BSONObj &infoObj, pmdEDUCB *cb,
                                     BSONObj *dsMeta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSINFOCHECKER_CHECK ) ;
      const CHAR *name = NULL ;
      const CHAR *addresses = NULL ;
      const CHAR *user = NULL ;
      const CHAR *password = NULL ;
      coordRemoteConnection connection ;

      try
      {
         BSONObjIterator itr( infoObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            const CHAR* fieldName = e.fieldName() ;
            if ( 0 ==  ossStrcmp( fieldName, FIELD_NAME_NAME ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               name = e.valuestr() ;
               rc = checkDSName( name ) ;
               PD_RC_CHECK( rc, PDERROR, "Check data source name failed[%d]",
                            rc ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ADDRESS ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               addresses = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_USER ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               user = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_PASSWD ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               password = e.valuestr() ;
            }
         }
         // User and/or password is not given by the user.
         if ( !user )
         {
            user = "" ;
         }
         if ( !password )
         {
            password = "" ;
         }

         if ( addresses )
         {
            coordDSAddrChecker addrChecker ;
            rc = addrChecker.check( addresses, user, password, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Validate data source service information "
                                      "failed[%d]", rc ) ;
         }

         if ( dsMeta && addresses )
         {
            rc = _initDSConnection( addresses, user, password, cb, connection ) ;
            PD_RC_CHECK( rc, PDERROR, "Initialize connection with data source "
                         "failed[%d]", rc ) ;
            rc = _getDSMeta( connection, *dsMeta, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get data source metadata failed[%d]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( connection.isConnected() )
      {
         connection.disconnect( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDRDSINFOCHECKER_CHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSINFOCHECKER_CHECKDSNAME, "_coordDSInfoChecker::checkDSName" )
   INT32 _coordDSInfoChecker::checkDSName( const CHAR *name )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSINFOCHECKER_CHECKDSNAME ) ;
      INT32 nameLen = 0 ;

      if ( !name )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Data source name can't be NULL" ) ;
         goto error ;
      }

      nameLen = ossStrlen ( name ) ;
      if ( DATASOURCE_MAX_NAME_SZ < nameLen || 0 >= nameLen )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Data source name length is invalid: %s",
                      name ) ;
         goto error ;
      }
      if ( '$' == name[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Data source name shouldn't start with '$': %s",
                      name ) ;
         goto error ;
      }
      if ( ossStrchr ( name, '.' ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Data source name shouldn't contain '.': %s",
                      name ) ;
         goto error ;
      }

      if ( _isSysName( name ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Data source name shouldn't start with 'SYS': "
                              "%s", name ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__COORDRDSINFOCHECKER_CHECKDSNAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSINFOCHECKER__GETDSMETA, "_coordDSInfoChecker::_getDSMeta" )
   INT32 _coordDSInfoChecker::_getDSMeta( coordRemoteConnection &connection,
                                          BSONObj &dsMeta, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSINFOCHECKER__GETDSMETA ) ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      pmdEDUEvent recvEvent ;
      MsgHeader *msg = NULL ;
      INT32 buffSize = 0 ;

      try
      {
         BSONObj dummyObj ;
         BSONObj versionSubObj ;
         BSONObjBuilder builder ;
         vector<BSONObj> objList ;
         BSONElement versionEle ;
         INT32 versionMajor = 0 ;
         INT32 versionMinor = 0 ;
         INT32 versionFix = 0 ;
         CHAR version[ SDB_DS_VERSION_SIZE + 1 ] = { 0 } ;
         // Get version from the coordinator node.
         BSONObj condition = BSON( FIELD_NAME_RAWDATA << true <<
                                   FIELD_NAME_GLOBAL << false  ) ;
         BSONObj selector = BSON( FIELD_NAME_VERSION << "" ) ;

         rc = msgBuildQueryCMDMsg( (CHAR **)&msg, &buffSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                                   condition, selector, dummyObj, dummyObj,
                                   0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build snapshot message failed[%d]", rc ) ;
         ((MsgOpQuery *)msg)->numToReturn = -1 ;

         rc = connection.syncSend( msg, recvEvent, cb, OSS_SOCKET_DFT_TIMEOUT,
                                   COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
         PD_RC_CHECK( rc, PDERROR, "Sync send snapshot database to data source "
                      "failed[%d]", rc ) ;

         rc = msgExtractReply( (CHAR *)recvEvent._Data, &flag, &contextID,
                               &startFrom, &numReturned, objList ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract snapshot respond failed[%d]", rc ) ;
         if ( flag )
         {
            rc = flag ;
            PD_RC_CHECK( rc, PDERROR, "Get snapshot of data source node "
                         "failed[%d]", rc ) ;
            goto error ;
         }

         if ( 0 == numReturned )
         {
            if ( -1 == contextID )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get metadata of data source failed[%d]", rc ) ;
               goto error ;
            }
            rc = msgBuildGetMoreMsg( (CHAR **)&msg, &buffSize, 1,
                                     contextID, 0, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Build getmore message failed[%d]", rc ) ;

            // Release the original event.
            pmdEduEventRelease( recvEvent, cb ) ;
            rc = connection.syncSend( msg, recvEvent, cb,
                                      OSS_SOCKET_DFT_TIMEOUT,
                                      COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            PD_RC_CHECK( rc, PDERROR, "Sync send getmore message to data "
                                      "source failed[%d]", rc ) ;

            rc = msgExtractReply( (CHAR *)recvEvent._Data, &flag, &contextID,
                                  &startFrom, &numReturned, objList ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract snapshot respond failed[%d]",
                         rc ) ;
            if ( flag )
            {
               rc = flag ;
               PD_LOG( PDERROR, "Get data source metadata failed[%d]", rc ) ;
               goto error ;
            }
         }

         SDB_ASSERT( numReturned >= 1, "No record returned" ) ;
         // Currently we only get the version.
         versionEle = objList[0].getField( FIELD_NAME_VERSION ) ;
         if ( versionEle.eoo() || ( Object != versionEle.type() ) )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Get version from data source failed. Please "
                        "check the data source service" ) ;
            PD_LOG( PDERROR, "Reply from data source on version query: %s",
                    objList[0].toString().c_str() ) ;
            goto error ;
         }

         versionSubObj = versionEle.embeddedObject() ;
         if ( ( !versionSubObj.hasField( FIELD_NAME_MAJOR ) ) ||
               ( !versionSubObj.hasField( FIELD_NAME_MINOR ) ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The major/minor version of data source is "
                    "missing[%d]", rc ) ;
            goto error ;
         }

         versionMajor = versionSubObj.getIntField( FIELD_NAME_MAJOR ) ;
         versionMinor = versionSubObj.getIntField( FIELD_NAME_MINOR ) ;
         if ( versionSubObj.hasField( FIELD_NAME_FIX ) )
         {
            versionFix = versionSubObj.getIntField( FIELD_NAME_FIX ) ;
            ossSnprintf( version, SDB_DS_VERSION_SIZE, "%d.%d.%d",
                         versionMajor, versionMinor, versionFix ) ;
         }
         else
         {
            ossSnprintf( version, SDB_DS_VERSION_SIZE, "%d.%d",
                         versionMajor, versionMinor ) ;
         }

         builder.append( FIELD_NAME_VERSION, version ) ;
         dsMeta = builder.done().getOwned() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         INT32 rcTemp = msgBuildKillContextsMsg( (CHAR **)&msg, &buffSize, 0, 1,
                                                 &contextID, cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Build kill context[%lld] message failed[%d]",
                    contextID, rc ) ;
            // Do not goto error
         }
         else
         {
            pmdEduEventRelease( recvEvent, cb ) ;
            rcTemp = connection.syncSend( msg, recvEvent, cb,
                                          OSS_SOCKET_DFT_TIMEOUT,
                                          COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            if ( rcTemp )
            {
               PD_LOG( PDERROR, "Send kill context[%lld] message to data "
                       "source failed[%d]", contextID, rc ) ;
               // Do not goto error
            }
         }
      }
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, cb ) ;
      }
      pmdEduEventRelease( recvEvent, cb ) ;
      PD_TRACE_EXITRC( SDB__COORDRDSINFOCHECKER__GETDSMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordDSInfoChecker::_isSysName( const CHAR *name )
   {
      if ( name && ossStrlen ( name ) >= 3 &&
           'S' == name[0] &&
           'Y' == name[1] &&
           'S' == name[2] )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   _coordDSAddrChecker::_coordDSAddrChecker()
   : _user( NULL ),
     _password( NULL )
   {
   }

   _coordDSAddrChecker::~_coordDSAddrChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER_CHECK, "_coordDSAddrChecker::check" )
   INT32 _coordDSAddrChecker::check( const CHAR *addrList, const CHAR *user,
                                     const CHAR *password, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER_CHECK ) ;
      utilAddrPair address ;
      utilAddrContainer addrContainer ;
      coordRemoteConnection connection ;
      BOOLEAN connected = FALSE ;
      _utilArray<UINT16> pendingAddr ;

      if ( !addrList )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Address list is empty" ) ;
         goto  error ;
      }

      _user = user ;
      _password = password ;

      rc = utilParseAddrList( addrList, addrContainer ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Parse address list[%s] failed", addrList ) ;
         goto error ;
      }

      if ( addrContainer.size() > CLS_REPLSET_MAX_NODE_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "At most %d addresses can be configured for a "
                     "data source", CLS_REPLSET_MAX_NODE_SIZE ) ;
         goto error ;
      }

      {
         // Traverse the address list, find the first one which can be connected
         // and autherized successfully. Get SYSCoord group nodes and catalogue
         // group information for later checking.
         const _utilArray<utilAddrPair>& addrArray = addrContainer.getAddresses() ;

         for ( UINT32 i = 0; i < addrArray.size(); ++i )
         {
            address = addrArray[ i ] ;
            if ( 0 == ossStrcmp( address.getHost(), OSS_LOCALHOST ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "'%s' is not allowed in data source "
                           "address", OSS_LOCALHOST ) ;
               goto error ;
            }

            if ( connected )
            {
               // Already found a address which can be connected, skip the
               // remaining.
               rc = pendingAddr.append( i ) ;
               PD_RC_CHECK( rc, PDERROR, "Append address index to pending list "
                                         "failed[%d]", rc ) ;
               continue ;
            }

            rc = connection.init( address.getHost(), address.getService() ) ;
            if ( rc )
            {
               // Not able to connect to the address. Try the next one.
               PD_LOG( PDDEBUG, "Not able to connect to data source "
                       "service[%s:%s]", address.getHost(),
                       address.getService() ) ;
               rc = pendingAddr.append( i ) ;
               PD_RC_CHECK( rc, PDERROR, "Append address index to pending list "
                            "failed[%d]", rc ) ;
               continue ;
            }

            rc = connection.authenticate( user, password, cb ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Authenticate with service[%s:%s] "
                           "failed", address.getHost(), address.getService() ) ;
               goto error ;
            }
            connected = TRUE ;
         }

         if ( pendingAddr.size() == addrContainer.size() )
         {
            rc = SDB_NET_CANNOT_CONNECT ;
            PD_LOG_MSG( PDERROR, "None of the addresses in the list is "
                    "accessible: [%s]", addrList ) ;
            goto error ;
         }

         rc = _checkAddrConflict( connection, addrContainer, pendingAddr, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check data source address[%s] conflict "
                      "failed[%d]", addrList, rc ) ;
      }

   done:
      if ( connected )
      {
         INT32 rcTemp = connection.disconnect( cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Disconnect from data source failed[%d]",
                    rcTemp ) ;
            // Do not goto error
         }
      }
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER_CHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER_CHECKADDRCONFLICT, "_coordDSAddrChecker::_checkAddrConflict" )
   INT32 _coordDSAddrChecker::_checkAddrConflict( coordRemoteConnection &connection,
                                                  utilAddrContainer &addrContainer,
                                                  _utilArray<UINT16> &pendingAddr,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER_CHECKADDRCONFLICT ) ;
      const CHAR *coordGroupName = "SYSCoord" ;

      UINT16 index = 0 ;
      utilAddrPair address ;
      _utilArray<utilAddrPair> myAddresses ;
      utilAddrContainer dsCoordAddrs ;
      _utilArray<UINT16> unknownAddr ;

      rc = _getMyAddresses( myAddresses ) ;
      PD_RC_CHECK( rc, PDERROR, "Get self address failed[%d]", rc ) ;

      rc = _getDSGroupAddresses( connection, coordGroupName,
                                 dsCoordAddrs, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Get data source service addresses failed[%d]",
                   rc ) ;

      {
         // Check if I myself is in the data source service addresses.
         _utilArray<utilAddrPair>::iterator itr( myAddresses ) ;
         while ( itr.next( address ) )
         {
            if ( dsCoordAddrs.contains( address ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Data source can not point to local "
                                    "cluster" ) ;
               goto error ;
            }
         }
      }
      {
         // Check if all addresses in the address list are all in data source
         // service group.
         _utilArray<UINT16>::iterator itr( pendingAddr ) ;
         while ( itr.next( index ) )
         {
            // Collect the addresses which are not int the real service
            // addresses.
            if ( !dsCoordAddrs.contains( addrContainer.getAddresses()[ index ] ) )
            {
               rc = unknownAddr.append( index ) ;
               PD_RC_CHECK( rc, PDERROR, "Append index of address to list "
                            "failed[%d]", rc ) ;
            }
         }
         if ( 0 == unknownAddr.size() )
         {
            // All addresses have been validated.
            goto done ;
         }
      }

      // Check if the remainning address belong to the same cluster with the
      // one which has been connected.
      rc = _ensureSameClusterAddresses( connection, addrContainer,
                                        unknownAddr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Check addresses in same cluster failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER_CHECKADDRCONFLICT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER__ENSURESAMECLUSTERADDRESSES, "_coordDSAddrChecker::_ensureSameClusterAddresses" )
   INT32 _coordDSAddrChecker::_ensureSameClusterAddresses( coordRemoteConnection &connection,
                                                           utilAddrContainer &addrContainer,
                                                           _utilArray<UINT16> &unkownAddr,
                                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER__ENSURESAMECLUSTERADDRESSES ) ;
      UINT16 index = 0 ;
      const CHAR *groupName = "SYSCatalogGroup" ;

      // There are still some possible wrong addresses. Create connection by
      // them, retrieve the catalogue OID, and check if all of them are the
      // same.
      _utilArray<UINT16>::iterator itr( unkownAddr ) ;
      utilAddrContainer addrBase ;
      rc = _getDSGroupAddresses( connection, groupName, addrBase, cb  ) ;
      PD_RC_CHECK( rc, PDERROR, "Get data source catalogue group identifier "
                                "failed[%d]", rc ) ;

      while ( itr.next( index ) )
      {
         coordRemoteConnection connectionTemp ;
         utilAddrContainer addrCompare ;
         const utilAddrPair &address = addrContainer.getAddresses()[ index ] ;

         rc = connectionTemp.init( address.getHost(), address.getService() ) ;
         PD_RC_CHECK( rc, PDERROR, "Initialize connection to data source "
                      "service[%s:%s] failed[%d]",
                      address.getHost(), address.getService(), rc ) ;
         rc = connectionTemp.authenticate( _user, _password, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Authentication on data source "
                      "service[%s:%s] failed[%d]",
                      address.getHost(), address.getService(), rc ) ;
         rc = _getDSGroupAddresses( connectionTemp, groupName,
                                    addrCompare, cb  ) ;
         PD_RC_CHECK( rc, PDERROR, "Get data source catalogue group identifier "
                                   "failed[%d]", rc ) ;
         SDB_ASSERT( addrCompare.size() > 0, "Group info size is 0" ) ;
         if ( !addrBase.contains( addrCompare.getAddresses()[0] ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Addresses in the address list do not belong "
                        "to the same cluster" ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER__ENSURESAMECLUSTERADDRESSES,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER__GETMYADDRESSES, "_coordDSAddrChecker::_getMyAddresses" )
   INT32 _coordDSAddrChecker::_getMyAddresses( _utilArray<utilAddrPair> &addresses )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER__GETMYADDRESSES ) ;

      netTCPEndPoint endPoint ;
      utilAddrPair myAddr ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      rc = _netRoute::getTCPEndPoint( krcb->getHostName(), krcb->getSvcname(),
                                      endPoint ) ;
      PD_RC_CHECK( rc, PDERROR, "Get TCP endpoint by[%s:%s] failed[%d]",
                   krcb->getHostName(), krcb->getSvcname(), rc ) ;
      myAddr.setHost( krcb->getHostName() ) ;
      myAddr.setService( krcb->getSvcname() ) ;
      rc = addresses.append( myAddr ) ;
      PD_RC_CHECK( rc, PDERROR, "Append self address to list failed[%d]", rc ) ;
      myAddr.setHost( endPoint.address().to_string().c_str() ) ;
      rc = addresses.append( myAddr ) ;
      PD_RC_CHECK( rc, PDERROR, "Append self address to list failed[%d]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER__GETMYADDRESSES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER__GETDSGROUPADDRESSES, "_coordDSAddrChecker::_getDSGroupAddresses" )
   INT32 _coordDSAddrChecker::_getDSGroupAddresses(
                                             coordRemoteConnection &connection,
                                             const CHAR *groupName,
                                             utilAddrContainer &addrContainer,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER__GETDSGROUPADDRESSES ) ;
      MsgHeader *msg = NULL ;
      INT32 buffLen = 0 ;
      INT32 flag = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = -1 ;
      MsgOpReply *reply = NULL ;
      pmdEDUEvent recvEvent ;
      INT64 contextID = -1 ;

      SDB_ASSERT( groupName, "groupName is NULL" ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj groupInfo ;
         vector<BSONObj> resultSet ;
         utilAddrContainer dsSvcAddresses ;
         BSONObj query = BSON( FIELD_NAME_GROUPNAME << groupName ) ;
         BSONObj selector = BSON( FIELD_NAME_GROUP << "" ) ;


         rc = msgBuildQueryCMDMsg( (CHAR **)&msg, &buffLen,
                                   CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPS,
                                   query, selector,
                                   dummyObj, dummyObj, 0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build list groups message failed[%d]",
                      rc ) ;
         ((MsgOpQuery *)msg)->numToReturn = -1 ;

         rc = connection.syncSend( (MsgHeader *)msg, recvEvent, cb,
                                   OSS_SOCKET_DFT_TIMEOUT,
                                   COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute list groups on data source "
                      "failed[%d]", rc ) ;

         reply = (MsgOpReply *)recvEvent._Data ;
         rc = msgExtractReply( (CHAR *)reply, &flag, &contextID,
                               &startFrom, &numReturned, resultSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract reply of list groups failed[%d]",
                      rc ) ;
         if ( flag )
         {
            rc = flag ;
            PD_LOG( PDERROR, "Execute list groups on data source failed[%d]",
                    rc ) ;
            goto error ;
         }

         if ( -1 != contextID )
         {
            rc = msgBuildGetMoreMsg( (CHAR **)&msg, &buffLen, -1, contextID,
                                     0, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Build get more request failed[%d]",
                         rc ) ;
            pmdEduEventRelease( recvEvent, cb ) ;
            rc = connection.syncSend( (MsgHeader *)msg, recvEvent, cb,
                                      OSS_SOCKET_DFT_TIMEOUT,
                                      COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            PD_RC_CHECK( rc, PDERROR, "Send getmore message to data source "
                         "failed[%d]", rc ) ;

            reply = (MsgOpReply *)recvEvent._Data ;
            rc = msgExtractReply( (CHAR *)reply, &flag, &contextID,
                                  &startFrom, &numReturned, resultSet ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract reply of list groups failed[%d]",
                         rc ) ;
            if ( flag )
            {
               rc = flag ;
               PD_LOG( PDERROR, "Get result of list groups on data source "
                       "failed[%d]", rc ) ;
               goto error ;
            }
            if ( 1 == resultSet.size() )
            {
               groupInfo = resultSet.front() ;
               rc = _parseDSSvcAddresses( groupInfo, addrContainer ) ;
               PD_RC_CHECK( rc, PDERROR, "Parse data source service addresses "
                                         "failed[%d]", rc ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         INT32 rcTemp = msgBuildKillContextsMsg( (CHAR **)&msg, &buffLen, 0, 1,
                                                 &contextID, cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Build kill context message failed[%d]", rcTemp ) ;
         }
         else
         {
            pmdEduEventRelease( recvEvent, cb ) ;
            rcTemp = connection.syncSend( msg, recvEvent, cb,
                                          OSS_SOCKET_DFT_TIMEOUT,
                                          COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            if ( rcTemp )
            {
               PD_LOG( PDERROR, "Send kill context message to data source "
                                "failed[%d]", rcTemp ) ;
            }
         }
      }
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, cb ) ;
      }
      pmdEduEventRelease( recvEvent, cb ) ;
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER__GETDSGROUPADDRESSES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSADDRCHECKER__PARSEDSSVCADDRESSES, "_coordDSAddrChecker::_parseDSSvcAddresses" )
   INT32 _coordDSAddrChecker::_parseDSSvcAddresses( const BSONObj &groupInfo,
                                                    utilAddrContainer &addresses )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSADDRCHECKER__PARSEDSSVCADDRESSES ) ;
      utilAddrPair addrItem ;

      try
      {
         BSONElement grpEle = groupInfo.getField( FIELD_NAME_GROUP ) ;
         BSONObjIterator itr( grpEle.embeddedObject() ) ;
         while ( itr.more() )
         {
            netTCPEndPoint endPoint ;
            BSONObj nodeObj = itr.next().embeddedObject() ;
            const CHAR *hostName = nodeObj.getStringField( FIELD_NAME_HOST ) ;
            const CHAR *svcName = NULL ;
            BSONElement svcEle = nodeObj.getField( FIELD_NAME_SERVICE ) ;
            BSONObjIterator subItr( svcEle.embeddedObject() ) ;
            while ( subItr.more() )
            {
               BSONObj svcObj = subItr.next().embeddedObject() ;
               if ( 0 == svcObj.getIntField( FIELD_NAME_SERVICE_TYPE ) )
               {
                  svcName = svcObj.getStringField( FIELD_NAME_NAME ) ;
                  break ;
               }
            }

            // Append both host name and ip address.
            addrItem.setHost( hostName ) ;
            addrItem.setService( svcName ) ;
            rc = addresses.append( addrItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Append data source service address "
                         "to address list failed[%d]", rc ) ;

            rc = _netRoute::getTCPEndPoint( hostName, svcName, endPoint ) ;
            if ( rc )
            {
               // If not able to translate from host name to IP, just skip.
               rc = SDB_OK ;
               continue ;
            }

            addrItem.setHost( endPoint.address().to_string().c_str() ) ;
            addrItem.setService( svcName ) ;
            rc = addresses.append( addrItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Append data source service address "
                                      "to address list failed[%d]", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDRDSADDRCHECKER__PARSEDSSVCADDRESSES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _coordDSCSChecker::_coordDSCSChecker()
   {
   }

   _coordDSCSChecker::~_coordDSCSChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSCSCHECKER_CHECK, "_coordDSCSChecker::check" )
   INT32 _coordDSCSChecker::check( CoordDataSourcePtr dsPtr, const CHAR *name,
                                   pmdEDUCB *cb, BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSCSCHECKER_CHECK ) ;
      MsgHeader *msg = NULL ;
      INT32 buffLen = 0 ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> resultSet ;
      pmdEDUEvent recvEvent ;
      coordRemoteConnection connection ;
      utilAddrPair address ;
      const _utilArray<utilAddrPair>& addrArray =
            ( (_utilAddrContainer *)dsPtr->getAddressList() )->getAddresses() ;
      BOOLEAN connected = FALSE ;

      for ( UINT32 i = 0; i < addrArray.size(); ++i )
      {
         address = addrArray[ i ] ;
         rc = connection.init( address.getHost(), address.getService() ) ;
         if ( SDB_OK == rc )
         {
            connected = TRUE ;
            break ;
         }
         // If the address cannot be connected, try the next one.
      }

      if ( !connected )
      {
         rc = SDB_NET_CANNOT_CONNECT ;
         PD_LOG( PDERROR, "None of the data source node can be connected[%d]",
                 rc ) ;
         goto error ;
      }

      rc = connection.authenticate( dsPtr->getUser(), dsPtr->getPassword(),
                                    cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Authentication on data source[%s] failed[%d]",
                   dsPtr->getName(), rc ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj query = BSON( FIELD_NAME_NAME << name ) ;
         rc = msgBuildQueryCMDMsg( ( CHAR **)&msg, &buffLen,
                                   CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTIONSPACE,
                                   query, dummyObj, dummyObj, dummyObj,
                                   0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build test collectionspace command "
                                   "failed[%d]", rc ) ;

         ((MsgOpQuery *)msg)->numToReturn = -1 ;

         rc = connection.syncSend( msg, recvEvent, cb, OSS_SOCKET_DFT_TIMEOUT,
                                   COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
         PD_RC_CHECK( rc, PDERROR, "Sync send test collectionspace command to "
                      "data source node[%s:%s] failed[%d]",
                      address.getHost(), address.getService(), rc ) ;

         rc = msgExtractReply( (CHAR *)(recvEvent._Data), &flag, &contextID,
                               &startFrom, &numReturned, resultSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract test collectionspace reply "
                                   "failed[%d]", rc ) ;
         if ( SDB_OK == flag )
         {
            exist = TRUE ;
            goto done ;
         }
         else if ( SDB_DMS_CS_NOTEXIST == flag )
         {
            // Checking is done successfully, and the collection space dose not
            // exist on data source.
            exist = FALSE ;
            goto done ;
         }
         else
         {
            // Other errors on data source.
            rc = flag ;
            PD_LOG( PDERROR, "Test collection space[%s] on data source "
                             "failed[%d]", name, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         INT32 rcTemp = msgBuildKillContextsMsg( (CHAR **)&msg, &buffLen, 0, 1,
                                                 &contextID, cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Build kill context message failed[%d]", rcTemp ) ;
         }
         else
         {
            pmdEduEventRelease( recvEvent, cb ) ;
            rcTemp = connection.syncSend( msg, recvEvent, cb,
                                          OSS_SOCKET_DFT_TIMEOUT,
                                          COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            if ( rcTemp )
            {
               PD_LOG( PDERROR, "Send kill context message to data source "
                                "failed[%d]", rcTemp ) ;
            }
         }
      }
      if ( connected )
      {
         INT32 rcTemp = connection.disconnect( cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Disconnect from data source[%s] failed[%d]",
                    dsPtr->getName(), rcTemp ) ;
            // Do not go to error.
         }
      }
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, cb ) ;
      }
      pmdEduEventRelease( recvEvent, cb ) ;
      PD_TRACE_EXITRC( SDB__COORDRDSCSCHECKER_CHECK, rc ) ;
      return rc ;
  error:
      goto done ;
   }

   _coordDSCLChecker::_coordDSCLChecker()
   {
   }

   _coordDSCLChecker::~_coordDSCLChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDRDSCLCHECKER_CHECK, "_coordDSCLChecker::check" )
   INT32 _coordDSCLChecker::check( CoordDataSourcePtr dsPtr, const CHAR *name,
                                   pmdEDUCB *cb, BOOLEAN &exist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDRDSCLCHECKER_CHECK ) ;
      MsgHeader *msg = NULL ;
      INT32 buffLen = 0 ;
      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> resultSet ;
      pmdEDUEvent recvEvent ;
      coordRemoteConnection connection ;
      utilAddrPair address ;
      BOOLEAN connected = FALSE ;
      const _utilArray<utilAddrPair>& addrArray =
            ( (_utilAddrContainer *)dsPtr->getAddressList() )->getAddresses() ;

      for ( UINT32 i = 0; i < addrArray.size(); ++i )
      {
         address = addrArray[ i ] ;
         rc = connection.init( address.getHost(), address.getService() ) ;
         if ( SDB_OK == rc )
         {
            connected = TRUE ;
            break ;
         }
         // If the address cannot be connected, try the next one.
      }
      if ( !connected )
      {
         rc = SDB_NOT_CONNECTED ;
         PD_LOG( PDERROR, "No service in data source[%s] can be connected[%d]",
                 dsPtr->getName(), rc ) ;
         goto error ;
      }

      rc = connection.authenticate( dsPtr->getUser(), dsPtr->getPassword(), cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Authentication on data source[%s] failed[%d]",
                   dsPtr->getName(), rc ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj query = BSON( FIELD_NAME_NAME << name ) ;
         rc = msgBuildQueryCMDMsg( ( CHAR **)&msg, &buffLen,
                                   CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION,
                                   query, dummyObj, dummyObj, dummyObj,
                                   0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build test collection command failed[%d]",
                      rc ) ;

         ((MsgOpQuery *)msg)->numToReturn = -1 ;
         rc = connection.syncSend( msg, recvEvent, cb, OSS_SOCKET_DFT_TIMEOUT,
                                   COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
         PD_RC_CHECK( rc, PDERROR, "Send test collection command to data "
                      "source node[%s:%s] failed[%d]",
                      address.getHost(), address.getService(), rc ) ;

         rc = msgExtractReply( (CHAR *)(recvEvent._Data), &flag, &contextID,
                               &startFrom, &numReturned, resultSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract test collection reply failed[%d]",
                      rc ) ;
         if ( SDB_OK == flag )
         {
            exist = TRUE ;
            goto done  ;
         }
         else if ( SDB_DMS_NOTEXIST == flag )
         {
            exist = FALSE ;
            goto done ;
         }
         else
         {
            rc = flag ;
            PD_LOG( PDERROR, "Test collection[%s] on data source failed[%d]",
                    name, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         INT32 rcTemp = msgBuildKillContextsMsg( (CHAR **)&msg, &buffLen, 0, 1,
                                                 &contextID, cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Build kill context message failed[%d]", rcTemp ) ;
         }
         else
         {
            pmdEduEventRelease( recvEvent, cb ) ;
            rcTemp = connection.syncSend( msg, recvEvent, cb,
                                          OSS_SOCKET_DFT_TIMEOUT,
                                          COORD_SDB_CONNECTION_FORCE_TIMEOUT ) ;
            if ( rcTemp )
            {
               PD_LOG( PDERROR, "Send kill context message to data source "
                                "failed[%d]", rcTemp ) ;
            }
         }
      }
      if ( connected )
      {
         INT32 rcTemp = connection.disconnect( cb ) ;
         if ( rcTemp )
         {
            PD_LOG( PDERROR, "Disconnect from data source[%s] failed[%d]",
                    dsPtr->getName(), rcTemp ) ;
            // Do not go to error.
         }
      }
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, cb ) ;
      }
      pmdEduEventRelease( recvEvent, cb ) ;
      PD_TRACE_EXITRC( SDB__COORDRDSCLCHECKER_CHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
