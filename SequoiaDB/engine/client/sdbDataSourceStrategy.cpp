/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbDataSourceStrategy.cpp

   Descriptive Name = SDB Data Source Strategy Source File

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbDataSourceStrategy.hpp"
#include <algorithm>
#include "ossUtil.h"
#include <stdio.h>

#define HOSTNAMELEN 256
#if defined (_LINUX) || defined (_AIX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

using std::string;
using std::vector;
using std::set;
using std::map;

namespace sdbclient
{
   #define SDB_DS_LOCAL_IP       ("127.0.0.1")
   #define SDB_DS_LOCAL_IP1      ("127.0.1.1")
   
   void sdbDataSourceStrategy::addCoord( const string &coord )
   {
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         // if not find, add it
         _coordMutex.get() ;
         if ( ( _normalCoordList.end() == std::find( _normalCoordList.begin(), 
            _normalCoordList.end(), newcoord ) ) && 
            ( _abnormalCoordList.end() == std::find( _abnormalCoordList.begin(), 
            _abnormalCoordList.end(), newcoord ) ) )
         {
            _normalCoordList.push_back( newcoord ) ;  
         }
         _coordMutex.release() ;
      }
   }

   void sdbDataSourceStrategy::removeCoord( const string &coord )
   {
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         vector<string>::iterator iter ;
         _coordMutex.get() ;
         iter = std::find( _normalCoordList.begin(), 
            _normalCoordList.end(), newcoord ) ;
         // find it in normal list
         if ( iter != _normalCoordList.end() )
         {
            _normalCoordList.erase( iter ) ;
         }
         // not find in normal list
         else
         {
            iter = std::find( _abnormalCoordList.begin(), 
               _abnormalCoordList.end(), newcoord ) ;
            // find in abnormal list
            if ( iter != _abnormalCoordList.end() )
            {
               _abnormalCoordList.erase( iter ) ;  
            }
         }
         _coordMutex.release() ;
      }
   }

   INT32 sdbDataSourceStrategy::getNormalCoordNum()
   {
      INT32 retNum = 0 ;
      _coordMutex.get() ;
      retNum = _normalCoordList.size() ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbDataSourceStrategy::getAbnormalCoordNum()
   {
      INT32 retNum = 0 ;
      _coordMutex.get() ;
      retNum = _abnormalCoordList.size() ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbDataSourceStrategy::getLocalCoordNum()
   {
      return 0 ;
   }

   INT32 sdbDataSourceStrategy::getNextAbnormalCoord( string& nCoord )
   {
      INT32 rc = SDB_OK ;

      _coordMutex.get() ;
      INT32 size = _abnormalCoordList.size() ;
      if ( 0 == size )
      {
         rc = SDB_EOF ;
      }
      else
      { 
         if ( _abPos > size )
            _abPos = 0 ;
         nCoord = _abnormalCoordList[_abPos] ;
         _abPos = ( _abPos+1 )%size ;
      }
      _coordMutex.release() ;

      return rc ;
   }

   // move coord from abnormal list to normal list
   void sdbDataSourceStrategy::mvCoordToNormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _abnormalCoordList.begin(), 
         _abnormalCoordList.end(), coord ) ;
      if ( iter != _normalCoordList.end() )
      {
         _abnormalCoordList.erase( iter ) ;
         _normalCoordList.push_back( coord ) ;
      }
      _coordMutex.release() ;
   }

   // convert hostname to ip
   BOOLEAN sdbDataSourceStrategy::_converToIP( 
      const string &oldcoord, string& newcoord )
   {
      #if defined (_WINDOWS)
      WORD wVersionRequested = MAKEWORD( 2, 2 ) ;
      WSADATA wsaData ;
      WSAStartup( wVersionRequested, &wsaData ) ;
      #endif
      
      struct addrinfo* ailist = NULL ;
      struct addrinfo hints ;

      hints.ai_family = AF_INET ;
      hints.ai_flags = AI_CANONNAME ;
      hints.ai_socktype = SOCK_STREAM ;
      hints.ai_protocol = 0 ;
      hints.ai_addrlen = 0 ;
      hints.ai_addr = NULL ;
      hints.ai_canonname = NULL ;
      hints.ai_next = NULL ;

      INT32 pos = oldcoord.find_first_of( ":" ) ;
      if ( 0 != getaddrinfo( oldcoord.substr(0, pos).c_str(), 
         oldcoord.substr( pos+1, oldcoord.length() ).c_str(), &hints, &ailist ) )
         return FALSE ;

      struct addrinfo* aip = NULL ;
      struct sockaddr_in* sin_p = NULL ;
      CHAR addr_in_p[INET_ADDRSTRLEN] = {0} ;

      aip = ailist ;
      if ( aip->ai_family == AF_INET )
      {
         sin_p = ( struct sockaddr_in* )aip->ai_addr ;
         newcoord = inet_ntoa( sin_p->sin_addr ) ;
         newcoord.append( ":" ) ;
         CHAR portname[HOSTNAMELEN] = {0} ;
         sprintf( portname, "%d", ntohs( sin_p->sin_port ) ) ;
         newcoord.append( portname ) ;
         if ( ( 0 != ossStrcmp( SDB_DS_LOCAL_IP, addr_in_p ) ) )
         {
            if ( _isLocalIP( newcoord ) )
            {
               pos = newcoord.find_first_of( ":" ) ;
               newcoord.replace( 0, pos, SDB_DS_LOCAL_IP ) ;
            }
         }
      }
      freeaddrinfo( ailist ) ;
      #if defined (_WINDOWS)
      WSACleanup() ;
      #endif
      return TRUE ;
   }
      
   // move coord from normal list to abnormal list
   void sdbDataSourceStrategy::mvCoordToAbnormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _normalCoordList.begin(), 
         _normalCoordList.end(), coord ) ;
      if ( iter != _normalCoordList.end() )
      {
         _normalCoordList.erase( iter ) ;
         _abnormalCoordList.push_back( coord ) ;
      }
      _coordMutex.release() ;
   }

   BOOLEAN sdbDataSourceStrategy::_isLocalIP( const string &ipstr )
   {
      CHAR hostname[HOSTNAMELEN] = {0} ;

      INT32 pos = ipstr.find_first_of( ":" ) ;
      ossStrcpy( hostname, ipstr.substr(0, pos).c_str() ) ;
      if ( 0 == ossStrcmp( SDB_DS_LOCAL_IP1, hostname ) )
         return TRUE ;

#if defined (_LINUX) || defined (_AIX)
      struct ifaddrs* ifAddrStruct, *ifAddr ;
      void* tmpAddrPtr = NULL ;
      if ( 0 != getifaddrs( &ifAddrStruct ) )
         return FALSE ;
      ifAddr = ifAddrStruct ;
      while( ifAddrStruct != NULL )
      {
         if ( ifAddrStruct->ifa_addr->sa_family == AF_INET )
         {
            tmpAddrPtr = &( ( struct sockaddr_in* )ifAddrStruct->ifa_addr )->sin_addr ;
            char addressBuffer[INET_ADDRSTRLEN] ;
            inet_ntop( AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN ) ;
            if ( 0 == ossStrcmp( hostname, addressBuffer ) )
            {
               freeifaddrs( ifAddr ) ;
               return TRUE ;
            }   
         }
         ifAddrStruct = ifAddrStruct->ifa_next ;
      }
      freeifaddrs( ifAddr ) ;
      return FALSE ;
#else
      WORD wVersionRequested = MAKEWORD( 2, 2 ) ;
      WSADATA wsaData ;
      WSAStartup( wVersionRequested, &wsaData ) ;
      
      CHAR localname[HOSTNAMELEN] = {0} ;
      gethostname( localname, HOSTNAMELEN-1 ) ;
      hostent* ph = gethostbyname( localname ) ;
      if ( NULL == ph )
         return FALSE ;
      CHAR FAR* FAR* ptr = ph->h_addr_list ;
      INT32 count = 0 ;
      for ( count = 0 ; ptr[count] ; count++ ) ;
      for ( int i = 0 ; i < count ; ++i )
      {
         if ( 0 == ossStrcmp( hostname, inet_ntoa(*(struct in_addr*)ptr[i]) ) )
         {
            WSACleanup() ;
            return TRUE ;
         }
      }
      WSACleanup() ;
      return FALSE ;
#endif
   }


   /*****************************************************************************/
   INT32 sdbDSSerialStrategy::getNextCoord( string& nCoord )
   {
      INT32 ret = SDB_OK ;
      INT32 coordsSize = 0 ;
      _coordMutex.get() ;
      coordsSize = _normalCoordList.size() ;
      if ( 0 == coordsSize )
      {
         ret = SDB_DS_NO_REACHABLE_COORD ;
      }
      else 
      {
         if ( _curPos >= coordsSize)
            _curPos = 0 ;
         nCoord = _normalCoordList[_curPos] ;
         _curPos = ( _curPos+1 )%coordsSize ;    
      }   
      _coordMutex.release() ;

      return ret ;
   }


   /*****************************************************************************/
   INT32 sdbDSRandomStrategy::getNextCoord( string& nCoord )
   {
      INT32 ret = SDB_OK ;
      INT32 coordsSize = 0 ;
      INT32 sel = 0;
      
      _coordMutex.get() ;
      coordsSize = _normalCoordList.size() ;
      if ( 0 == coordsSize )
      {
         ret = SDB_DS_NO_REACHABLE_COORD ;
      }
      else 
      {
         sel = rand() % coordsSize ;
         nCoord = _normalCoordList[sel] ;
      }
      _coordMutex.release() ;
      
      return ret ;
   }

   /*****************************************************************************/
   INT32 sdbDSLocalStrategy::getLocalCoordNum()
   {
      _coordMutex.get() ;
      INT32 ret = _localCoordList.size() ;
      _coordMutex.release() ;

      return ret ;
   }

   void sdbDSLocalStrategy::addCoord( const string &coord )
   {
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         // if not find, add it
         _coordMutex.get() ;
         if ( ( _normalCoordList.end() == std::find( _normalCoordList.begin(), 
            _normalCoordList.end(), newcoord ) ) && 
            ( _abnormalCoordList.end() == std::find( _abnormalCoordList.begin(), 
            _abnormalCoordList.end(), newcoord ) ) )
         {
            _normalCoordList.push_back( newcoord ) ;
            if ( _isLocalCoord( newcoord ) )
            {
               _localCoordList.push_back( newcoord ) ;
            }
         }
         _coordMutex.release() ;
      }
   }

   void sdbDSLocalStrategy::removeCoord( const string &coord )
   {
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         vector<string>::iterator iter ;
         _coordMutex.get() ;
         iter = std::find( _normalCoordList.begin(), _normalCoordList.end(), newcoord ) ;
         // find it in normal list
         if ( iter != _normalCoordList.end() )
         {
            _normalCoordList.erase( iter ) ;
            if ( _isLocalCoord( newcoord ) )
            {
               iter = std::find( _localCoordList.begin(), 
                  _localCoordList.end(), newcoord ) ;
               // find it in normal list
               if ( iter != _localCoordList.end() )
               {
                  _localCoordList.erase( iter ) ;   
               }
            }
         }
         // not find in normal list
         else
         {
            iter = std::find( _abnormalCoordList.begin(), 
               _abnormalCoordList.end(), newcoord ) ;
            // find in abnormal list
            if ( iter != _abnormalCoordList.end() )
            {
               _abnormalCoordList.erase( iter ) ;  
            }
         }
         _coordMutex.release() ;
      }
   }

   // move coord from normal list to abnormal list, if local list exist,
   // delete it from local list
   void sdbDSLocalStrategy::mvCoordToAbnormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _normalCoordList.begin(), _normalCoordList.end(), coord ) ;
      if ( iter != _normalCoordList.end() )
      {
         _normalCoordList.erase( iter ) ;
         _abnormalCoordList.push_back( coord ) ;
         iter = std::find( _localCoordList.begin(), _localCoordList.end(), coord ) ;
         if ( iter != _localCoordList.end() )
         {
            _localCoordList.erase( iter ) ;
         }
      }
      _coordMutex.release() ;
   }

   // move coord from abnormal list to normal list, if coord is local coord,
   // add it to local list
   void sdbDSLocalStrategy::mvCoordToNormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _abnormalCoordList.begin(), 
         _abnormalCoordList.end(), coord ) ;
      if ( iter != _normalCoordList.end() )
      {
         _abnormalCoordList.erase( iter ) ;
         _normalCoordList.push_back( coord ) ;
         if ( _isLocalCoord( coord ) )
         {
            _localCoordList.push_back( coord ) ;
         }
      }
      _coordMutex.release() ;
   }

   INT32 sdbDSLocalStrategy::getNextCoord( string& nCoord )
   {
      INT32 rc        = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      
      _coordMutex.get() ;
      hasLock = TRUE ;
      INT32 coordsSize = _localCoordList.size() ;
      if ( 0 == coordsSize )
      {
         INT32 norSize = _normalCoordList.size() ;
         if ( 0 == norSize )
         {
            rc = SDB_DS_NO_REACHABLE_COORD ;
            goto error ;
         }
         else
         {
            if ( _normalPos >= norSize )
            {
               _normalPos = 0 ;
            }
            nCoord = _normalCoordList[_normalPos] ;
            _normalPos = ( _normalPos+1 ) % norSize ; 
         }
      }
      else 
      {
         if ( _localPos >= coordsSize )
         {
            _localPos = 0 ;
         }
         nCoord = _localCoordList[_localPos] ;
         _localPos = ( _localPos+1 )% coordsSize ;    
      }   

   done:
      if ( TRUE == hasLock )
      {
         _coordMutex.release() ;
      }
      return rc ;
   error:
      goto done  ;
   }

   // check coord is local coord or not
   BOOLEAN sdbDSLocalStrategy::_isLocalCoord( const string &coord )
   {
      // TODO: (new) local ips do not only mean "127.0.0.1",
      // we should scan all the netcards to get the local ips.
      CHAR hostname[HOSTNAMELEN] ;
      INT32 pos = coord.find_first_of( ":" ) ;
      ossStrcpy( hostname, coord.substr(0, pos).c_str() ) ;
      return ( 0 == ossStrcmp( SDB_DS_LOCAL_IP, hostname ) ) ? TRUE : FALSE ;
   }

   /*****************************************************************************/

   sdbDSBalanceStrategy::~sdbDSBalanceStrategy()
   {
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      coordInfo* coord ;
      for ( iter = _coordInfoSet.begin() ; iter != _coordInfoSet.end() ; ++iter )
      {
         coord = *iter ;
         SAFE_OSS_DELETE( coord ) ;
      }
      _coordInfoSet.clear() ;
   }

   set<coordInfo*, coordInfoCmp>::const_iterator 
      sdbDSBalanceStrategy::_findCoord( const string &coord ) const
   {
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      for ( iter = _coordInfoSet.begin() ; iter != _coordInfoSet.end() ; ++iter )
      {
         if ( 0 == coord.compare( (*iter)->coord ) )
            break ;
      }
      return iter ;
   }

   void sdbDSBalanceStrategy::addCoord( const string &coord )
   {  
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         _coordMutex.get() ;
         set<coordInfo*, coordInfoCmp>::const_iterator iter = _findCoord( newcoord ) ;
         // not found
         if ( iter == _coordInfoSet.end() )
         {
            coordInfo* pCoord = SDB_OSS_NEW coordInfo( newcoord ) ;
            if ( NULL != pCoord )
            {
               _coordInfoSet.insert( pCoord ) ;
            }
         }
         _coordMutex.release() ; 
      }
   }

   void sdbDSBalanceStrategy::removeCoord( const string &coord )
   {
      string newcoord ;
      if ( _converToIP(coord, newcoord) )
      {
         _coordMutex.get() ;
         set<coordInfo*, coordInfoCmp>::const_iterator iter = _findCoord( newcoord ) ;
         if ( iter != _coordInfoSet.end() )
         {
            coordInfo* pCoord = *iter ;
            SAFE_OSS_DELETE( pCoord ) ;
            _coordInfoSet.erase( iter ) ;
         }
         _coordMutex.release() ;
      }
   }

   // isNormal: TRUE get normal coord number
   // FALSE get abnormal coord number
   INT32 sdbDSBalanceStrategy::_getCoordNum( BOOLEAN isNormal )
   {
      INT32 availableNum = 0 ;
      INT32 retNum = 0 ;
      
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      for ( iter = _coordInfoSet.begin() ; iter != _coordInfoSet.end() ; ++iter )
      {
         if ( (*iter)->bAvailable )
         {
            ++availableNum ;
         }
         else
         {
            break ;
         }
      }
      retNum = isNormal ? availableNum : _coordInfoSet.size() - availableNum ;
      return retNum ;
   }

   INT32 sdbDSBalanceStrategy::getNormalCoordNum()
   {
      INT32 retNum ;

      _coordMutex.get() ;
      retNum = _getCoordNum( TRUE ) ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbDSBalanceStrategy::getAbnormalCoordNum()
   {
      INT32 retNum ;
      
      _coordMutex.get() ;
      retNum = _getCoordNum( FALSE ) ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbDSBalanceStrategy::getNextCoord( string& nCoord )
   {
      INT32 rc = SDB_OK ;
      
      _coordMutex.get() ;
      if ( _coordInfoSet.size() == 0 )
      {
         rc = SDB_DS_NO_REACHABLE_COORD ;
      }
      else
      {
         set<coordInfo*, coordInfoCmp>::const_iterator iter ;
         iter = _coordInfoSet.begin() ;
         if ( (*iter)->bAvailable )
         {
            nCoord = (*iter)->coord ;
         }
         else
         {
            rc = SDB_DS_NO_REACHABLE_COORD ;
         }
      }
      _coordMutex.release() ;

      return rc ;
   }

   INT32 sdbDSBalanceStrategy::getNextAbnormalCoord( string& nCoord )
   {
      INT32 rc   = SDB_OK ;
      INT32 size = 0 ;
      
      _coordMutex.get() ;
      size = _getCoordNum( FALSE ) ;
      if ( 0 == size )
      {
         rc = SDB_EOF ;
      }
      else
      {
         if ( _abPos > size )
         {
            _abPos = 0 ;
         }
         set< coordInfo*, coordInfoCmp >::reverse_iterator iter ;
         iter = _coordInfoSet.rbegin() ;
         INT32 j = 0 ;
         while ( j < _abPos )
         {
            ++j ;
            ++iter ;
         }
         nCoord = (*iter)->coord ;
         _abPos = ( _abPos+1 )%size ;
      }
      _coordMutex.release() ;

      return rc ;
   }

   // move coord from normal list to abnormal list
   void sdbDSBalanceStrategy::mvCoordToAbnormal( const string &coord )
   {
      _coordMutex.get() ;
      set<coordInfo*, coordInfoCmp>::const_iterator iter = _findCoord( coord ) ;
      // found
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* newCoord = *iter ;
         _coordInfoSet.erase( iter ) ;
         newCoord->bAvailable = FALSE ;
         _coordInfoSet.insert( newCoord ) ;
      }
      _coordMutex.release() ;
   }

   // move coord from abnormal list to normal list
   void sdbDSBalanceStrategy::mvCoordToNormal( const string &coord )
   {
      _coordMutex.get() ;
      set<coordInfo*, coordInfoCmp>::const_iterator iter = _findCoord( coord ) ;
      // found
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* newCoord = *iter ;
         _coordInfoSet.erase( iter ) ;
         newCoord->bAvailable = TRUE ;
         _coordInfoSet.insert( newCoord ) ;
      }
      _coordMutex.release() ;
   }

   void sdbDSBalanceStrategy::syncAddNewConn( sdb *conn, const string &coord )
   {
      // sync _coordInfoSet
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      _coordMutex.get() ;
      iter = _findCoord( coord ) ;
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* info = *iter ;
         _coordInfoSet.erase( iter ) ;
         info->totalNum = info->totalNum + 1 ;
         _coordInfoSet.insert( info ) ;
         // sync _connToCoord
         _connToCoord[conn] = info ;
      }
      else
      {
         SDB_ASSERT( FALSE, "coord info mush be in coordInfoSet" ) ;
      }
      _coordMutex.release() ;
   }

   // sync strategy
   void sdbDSBalanceStrategy::sync( sdb *conn, SYNC_CHOICE choice )
   {
      switch(choice)
      {
      case DELIDLECONN:
         _syncDelIdleConn( conn ) ;
         break ;
      case DELBUSYCONN:
         _syncDelBusyConn( conn ) ;
         break ;
      case ADDBUSYCONN:
         _syncAddBusyConn( conn ) ;
         break ;
      case ADDIDLECONN:
         _syncAddIdleConn( conn ) ;
         break ;   
      }
   }

   void sdbDSBalanceStrategy::_syncDelIdleConn( sdb *conn )
   {
      // sync _coordInfoSet
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      _coordMutex.get() ;
      iter = _coordInfoSet.find( _connToCoord[conn] ) ;
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* info = *iter ;
         _coordInfoSet.erase( iter ) ;
         info->totalNum = info->totalNum - 1 ;
         _coordInfoSet.insert( info ) ;
         // sync _connToCoord
         _connToCoord.erase( conn ) ;
      }      
      _coordMutex.release() ;
   }

   void sdbDSBalanceStrategy::_syncDelBusyConn( sdb *conn )
   {
      // sync _coordInfoSet
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      _coordMutex.get() ;
      iter = _coordInfoSet.find( _connToCoord[conn] ) ;
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* info = *iter ;
         _coordInfoSet.erase( iter ) ;
         info->usedNum = info->usedNum - 1 ;
         info->totalNum = info->totalNum - 1 ;
         _coordInfoSet.insert( info ) ;
         // sync _connToCoord
         _connToCoord.erase( conn ) ;
      }
      _coordMutex.release() ;
   }

   void sdbDSBalanceStrategy::_syncAddBusyConn( sdb *conn )
   {
      // sync _coordInfoSet
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      _coordMutex.get() ;
      iter = _coordInfoSet.find( _connToCoord[conn] ) ;
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* info = *iter ;
         _coordInfoSet.erase( iter ) ;
         info->usedNum = info->usedNum + 1 ;
         _coordInfoSet.insert( info ) ;
      }
      _coordMutex.release() ;
   }

   void sdbDSBalanceStrategy::_syncAddIdleConn( sdb *conn )
   {
      // sync _coordInfoSet
      set<coordInfo*, coordInfoCmp>::const_iterator iter ;
      _coordMutex.get() ;
      iter = _coordInfoSet.find( _connToCoord[conn] ) ;
      if ( iter != _coordInfoSet.end() )
      {
         coordInfo* info = *iter ;
         _coordInfoSet.erase( iter ) ;
         info->usedNum = info->usedNum - 1 ;
         _coordInfoSet.insert( info ) ;
      }
      _coordMutex.release() ;
   }
}
