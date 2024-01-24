/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbConnectionPoolStrategy.cpp

   Descriptive Name = SDB connection pool Strategy Source File

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbConnectionPoolStrategy.hpp"
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
   #define SDB_CONNPOOL_LOCAL_IP       ("127.0.0.1")
   #define SDB_CONNPOOL_LOCAL_IP1      ("127.0.1.1")
   
   void sdbConnPoolStrategy::addCoord( const string &coord )
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

   void sdbConnPoolStrategy::removeCoord( const string &coord )
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

   INT32 sdbConnPoolStrategy::getNormalCoordNum()
   {
      INT32 retNum = 0 ;
      _coordMutex.get() ;
      retNum = _normalCoordList.size() ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbConnPoolStrategy::getAbnormalCoordNum()
   {
      INT32 retNum = 0 ;
      _coordMutex.get() ;
      retNum = _abnormalCoordList.size() ;
      _coordMutex.release() ;

      return retNum ;
   }

   INT32 sdbConnPoolStrategy::getLocalCoordNum()
   {
      return 0 ;
   }

   INT32 sdbConnPoolStrategy::getNextAbnormalCoord( string& nCoord )
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
   void sdbConnPoolStrategy::mvCoordToNormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _abnormalCoordList.begin(), 
         _abnormalCoordList.end(), coord ) ;
      if ( iter != _abnormalCoordList.end() )
      {
         _abnormalCoordList.erase( iter ) ;
         _normalCoordList.push_back( coord ) ;
      }
      _coordMutex.release() ;
   }

   // convert hostname to ip
   BOOLEAN sdbConnPoolStrategy::_converToIP(
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
         if ( ( 0 != ossStrcmp( SDB_CONNPOOL_LOCAL_IP, addr_in_p ) ) )
         {
            if ( _isLocalIP( newcoord ) )
            {
               pos = newcoord.find_first_of( ":" ) ;
               newcoord.replace( 0, pos, SDB_CONNPOOL_LOCAL_IP ) ;
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
   void sdbConnPoolStrategy::mvCoordToAbnormal( const string &coord )
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

   BOOLEAN sdbConnPoolStrategy::_updateAddress( const vector<string> &addrs, vector<string> &delAddrs )
   {
      vector<string> newAddrs ;

      // 1. _converToIP
      for ( UINT32 i = 0 ; i < addrs.size() ; ++i )
      {
         string newAddr ;
         if ( !_converToIP( addrs[i], newAddr ) )
         {
            return FALSE ;
         }
         // to repeat
         if ( newAddrs.end() == std::find( newAddrs.begin(), newAddrs.end(), newAddr ) )
         {
            newAddrs.push_back( newAddr ) ;
         }
      }

      // 2. get diff
      for ( UINT32 i = 0 ; i < _normalCoordList.size() ; ++i )
      {
         if ( newAddrs.end() == std::find( newAddrs.begin(), newAddrs.end(), _normalCoordList[i] ) )
         {
            delAddrs.push_back( _normalCoordList[i] ) ;
         }
      }
      for ( UINT32 i = 0 ; i < _abnormalCoordList.size() ; ++i )
      {
         if ( newAddrs.end() == std::find( newAddrs.begin(), newAddrs.end(), _abnormalCoordList[i] ) )
         {
            delAddrs.push_back( _abnormalCoordList[i] ) ;
         }
      }

      // 3. update
      newAddrs.swap( _normalCoordList ) ;
      _abnormalCoordList.clear() ;
      return TRUE ;
   }

   BOOLEAN sdbConnPoolStrategy::updateAddress( const vector<string> &addrs, vector<string> &delAddrs )
   {
      BOOLEAN ret = FALSE ;
      _coordMutex.get() ;
      ret = _updateAddress( addrs, delAddrs ) ;
      _coordMutex.release() ;
      return ret ;
   }

   BOOLEAN sdbConnPoolStrategy::checkAddress( const string &address )
   {
      BOOLEAN result = FALSE ;
      _coordMutex.get() ;
      if ( ( _normalCoordList.end() != std::find( _normalCoordList.begin(),
          _normalCoordList.end(), address ) ) ||
          ( _abnormalCoordList.end() != std::find( _abnormalCoordList.begin(),
          _abnormalCoordList.end(), address ) ) )
      {
         result = TRUE ;
      }
      _coordMutex.release() ;
      return result ;
   }

   BOOLEAN sdbConnPoolStrategy::_isLocalIP( const string &ipstr )
   {
      CHAR hostname[HOSTNAMELEN] = {0} ;

      INT32 pos = ipstr.find_first_of( ":" ) ;
      ossStrcpy( hostname, ipstr.substr(0, pos).c_str() ) ;
      if ( 0 == ossStrcmp( SDB_CONNPOOL_LOCAL_IP1, hostname ) )
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
   INT32 sdbConnPoolSerialStrategy::getNextCoord( string& nCoord )
   {
      INT32 ret = SDB_OK ;
      INT32 coordsSize = 0 ;
      _coordMutex.get() ;
      coordsSize = _normalCoordList.size() ;
      if ( 0 == coordsSize )
      {
         ret = SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD ;
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
   INT32 sdbConnPoolRandomStrategy::getNextCoord( string& nCoord )
   {
      INT32 ret = SDB_OK ;
      INT32 coordsSize = 0 ;
      INT32 sel = 0;
      
      _coordMutex.get() ;
      coordsSize = _normalCoordList.size() ;
      if ( 0 == coordsSize )
      {
         ret = SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD ;
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
   INT32 sdbConnPoolLocalStrategy::getLocalCoordNum()
   {
      _coordMutex.get() ;
      INT32 ret = _localCoordList.size() ;
      _coordMutex.release() ;

      return ret ;
   }

   void sdbConnPoolLocalStrategy::addCoord( const string &coord )
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

   void sdbConnPoolLocalStrategy::removeCoord( const string &coord )
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
   void sdbConnPoolLocalStrategy::mvCoordToAbnormal( const string &coord )
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
   void sdbConnPoolLocalStrategy::mvCoordToNormal( const string &coord )
   {
      vector<string>::iterator iter ;
      _coordMutex.get() ;
      iter = std::find( _abnormalCoordList.begin(), 
         _abnormalCoordList.end(), coord ) ;
      if ( iter != _abnormalCoordList.end() )
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

   INT32 sdbConnPoolLocalStrategy::getNextCoord( string& nCoord )
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
            rc = SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD ;
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
   BOOLEAN sdbConnPoolLocalStrategy::_isLocalCoord( const string &coord )
   {
      // TODO: (new) local ips do not only mean "127.0.0.1",
      // we should scan all the netcards to get the local ips.
      CHAR hostname[HOSTNAMELEN] ;
      INT32 pos = coord.find_first_of( ":" ) ;
      ossStrcpy( hostname, coord.substr(0, pos).c_str() ) ;
      return ( 0 == ossStrcmp( SDB_CONNPOOL_LOCAL_IP, hostname ) ) ? TRUE : FALSE ;
   }

   BOOLEAN sdbConnPoolLocalStrategy::updateAddress( const vector<string> &addrs,
                                                    vector<string> &delAddrs )
   {
      BOOLEAN ret = FALSE ;

      _coordMutex.get() ;
      ret = _updateAddress( addrs, delAddrs ) ;
      if ( ret )
      {
         // generate local address list
         _localCoordList.clear() ;
         for ( UINT32 i = 0 ; i < _normalCoordList.size() ; ++i )
         {
            if ( _isLocalCoord( _normalCoordList[i] ) )
            {
               _localCoordList.push_back( _normalCoordList[i] ) ;
            }
         }
      }
      _coordMutex.release() ;
      return ret ;
   }
}
