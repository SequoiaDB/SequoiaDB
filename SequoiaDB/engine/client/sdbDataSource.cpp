/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbDataSource.cpp

   Descriptive Name = SDB Data Source Source File

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbDataSource.hpp"
#include "sdbDataSourceStrategy.hpp"
#include "client.hpp"
#include "sdbDataSourceWorker.hpp"
#include <algorithm>
#include "ossMem.hpp"
#include <iostream>


using std::string;
using std::list;

namespace sdbclient
{
   #define SDB_DS_SLEEP_TIME        100
   #define SDB_DS_TRYGETCONN_TIME   3
   #define SDB_DS_MULTIPLE          1.2
   
   void createConnFunc( void *args )
   {
      ((sdbDataSource*)args)->_createConn() ;
   }
   
   void destroyConnFunc( void *args )
   {
      ((sdbDataSource*)args)->_destroyConn() ;
   }

   void bgTaskFunc( void *args )
   {
      ((sdbDataSource*)args)->_bgTask() ;
   }

   sdbDataSource::~sdbDataSource()
   {
      disable() ;
      SAFE_OSS_DELETE(_strategy) ;
   }
   
   INT32 sdbDataSource::init(
      const string &url, 
      const sdbDataSourceConf &conf )
   {
      std::vector<string> vUrl ;
      vUrl.push_back( url ) ;
      
      return init( vUrl, conf ) ;
   }

   INT32 sdbDataSource::init( 
      const std::vector<string> &vUrls,
      const sdbDataSourceConf &conf )
   {
      INT32 rc = SDB_OK ;
      int validUrlCnt = 0 ;

      if ( TRUE == _isInited )
      {
         goto done ;
      }

      _conf = conf ;
      if ( !_conf.isValid() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      
      rc = _buildStrategy();
      if ( SDB_OK != rc )
      {
         SAFE_OSS_DELETE(_strategy) ;
         goto error ;
      }
      
      for ( UINT32 i = 0 ; i < vUrls.size() ; ++i )
      {
         if ( _checkAddrArg( vUrls[i] ) )
         {   
            _strategy->addCoord( vUrls[i] ) ;
            validUrlCnt++ ;
         }
      }
      if ( 0 == validUrlCnt )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _isInited = TRUE ;
   done :
      return rc  ;
   error :
      goto done  ;
   }
   
   INT32 sdbDataSource::getIdleConnNum() const
   {
      return _idleSize.peek() ;
   }

   INT32 sdbDataSource::getUsedConnNum() const
   {
      return _busySize.peek() ;
   }

   INT32 sdbDataSource::getNormalCoordNum() const
   {
      SDB_ASSERT( _strategy, "_strategy is null" ) ;
      return _strategy->getNormalCoordNum() ;
   }

   INT32 sdbDataSource::getAbnormalCoordNum() const
   {  
      SDB_ASSERT( _strategy, "_strategy is null" ) ;
      return _strategy->getAbnormalCoordNum() ;
   }

   INT32 sdbDataSource::getLocalCoordNum() const
   {
      SDB_ASSERT( _strategy, "_strategy is null" ) ;
      return _strategy->getLocalCoordNum() ;
   }
   
   void sdbDataSource::addCoord( const string &url )
   {
      if ( _isInited && _checkAddrArg(url) )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         _strategy->addCoord( url ) ;
      }
   }

   void sdbDataSource::removeCoord( const string &url )
   {
      if ( _isInited && _checkAddrArg(url) )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         _strategy->removeCoord( url ) ;
      }
   }

   INT32 sdbDataSource::enable()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isLocked = FALSE ;
      
      if ( !_isInited )
      {
         rc = SDB_DS_NOT_INIT ;
         goto error ;
      }
      else
      {
         if ( _isEnabled )
         {
            goto done ;
         }
         else
         {
            _globalMutex.get() ;
            isLocked = TRUE ;
            if ( !_isEnabled )
            {
               _toStopWorkers = FALSE ;
               _createConnByNum( _conf.getInitConnCount() ) ;
               _createConnWorker = SDB_OSS_NEW sdbDSWorker( 
                  createConnFunc, this ) ;
               if ( NULL == _createConnWorker )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               _destroyConnWorker = SDB_OSS_NEW sdbDSWorker( destroyConnFunc, 
                                                             this ) ;
               if ( NULL == _destroyConnWorker )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               _bgTaskWorker= SDB_OSS_NEW sdbDSWorker( bgTaskFunc, this ) ;
               if ( NULL == _bgTaskWorker )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               rc = _createConnWorker->start() ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               rc = _destroyConnWorker->start() ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               rc = _bgTaskWorker->start() ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               _isEnabled = TRUE ;
            }
         }
      }

   done :
      if ( isLocked )
      {
         _globalMutex.release() ;
      }
      return rc ;
   error :
      SAFE_OSS_DELETE(_createConnWorker) ;
      SAFE_OSS_DELETE(_destroyConnWorker) ;
      SAFE_OSS_DELETE(_bgTaskWorker) ;
      goto done  ;
   }

   INT32 sdbDataSource::disable()
   {
      INT32 ret = SDB_OK ;
      BOOLEAN isLocked = FALSE ;
      
      if ( !_isEnabled )
      {
         goto done ;
      }
      else
      {
         _globalMutex.get() ;
         isLocked = TRUE ;
         if ( _isEnabled )
         {
            _toStopWorkers = TRUE ;

            INT32 rc = SDB_OK ;
            rc = _createConnWorker->waitStop() ;
            if ( SDB_OK != rc )
            {
               ret = rc ;
               goto error ;// TODO:(new) when it failed, we leave the 
            }
            rc = _destroyConnWorker->waitStop() ;
            if ( SDB_OK != rc )
            {
               ret = rc ;
               goto error ;
            }
            rc = _bgTaskWorker->waitStop() ;
            if ( SDB_OK != rc )
            {
               ret = rc ;
               goto error ;
            }

            _clearDataSource() ;

            _toCreateConn = FALSE ;
            _toDestroyConn = FALSE ;
            _toStopWorkers = FALSE ;
            _isEnabled = FALSE ;
         }
      }
   done :
      if ( isLocked )
      {
         _globalMutex.release() ;
      }
      return ret  ;
   error :
      goto done  ;
   }

   INT32 sdbDataSource::getConnection( sdb*& conn, INT64 timeoutms )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isGet = FALSE ;

      if ( 0 > timeoutms )
      {
         timeoutms = 0 ;
      }
      
      if ( !_isEnabled )
      {
         rc = SDB_DS_NOT_ENABLE ;
         goto error ;
      }
      
      while ( !isGet )
      {
         if ( _idleSize.peek() > 0 )
         {
            isGet = _tryGetConn( conn ) ;
         }
         else
         {
            if ( ( (UINT32)_conf.getMaxCount() ) <= _busySize.peek()) 
            {
               INT64 timeCnt = 0 ;
               if ( 0 == timeoutms )
               {
                  timeoutms = OSS_SINT64_MAX;
               } 
               while ( !isGet && 0 <= timeCnt && timeCnt < timeoutms )
               {
                  ossSleep( SDB_DS_SLEEP_TIME ) ;
                  timeCnt += SDB_DS_SLEEP_TIME ;
                  if ( _idleSize.peek() > 0 )
                  {
                     isGet = _tryGetConn( conn ) ;
                  }
               }
               if ( !isGet )
               {   
                  rc = SDB_DRIVER_DS_RUNOUT ;
                  goto error ;
               }
            }
            else
            {
               if ( 0 == _strategy->getNormalCoordNum() )
               {  
                  rc = SDB_DS_NO_REACHABLE_COORD ;
                  goto error ;
               }
               else
               {
                  INT32 createNum = _createConnByNum(1);
                  _connMutex.get() ;
                  if ( (0 < createNum) && (0 < _idleSize.peek()) )
                  {
                     sdb* pConn = _idleList.front() ;
                     SDB_ASSERT(pConn, 
                        "the connection got from idle list can't be null") ;
                     _idleList.pop_front() ;
                     _idleSize.dec() ;
                     _busyList.push_back( pConn ) ;
                     _busySize.inc() ;
                     _connMutex.release() ;
                     conn = pConn ;
                     _strategy->sync( pConn, ADDBUSYCONN ) ;
                     isGet = TRUE ;
                  }
                  else
                  {
                     _connMutex.release() ;
                  }
               } // has reachable coord
            } // not reach max count
         } // _idleSize <= 0
         
         if ( _idleSize.peek() <= SDB_DS_TOPRECREATE_THRESHOLD )
         {
            _toCreateConn = TRUE ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;   
   }

   void sdbDataSource::releaseConnection( sdb *conn )
   {
      BOOLEAN              isLock = FALSE ;
      INT32                rc = SDB_OK ;
      sdb*                 tmp = NULL ;

      if ( _isEnabled )
      {
         list<sdb*>::iterator iter ;
         _connMutex.get() ; 
         isLock = TRUE ;
         if ( _isEnabled )
         {
            iter = std::find( _busyList.begin(), _busyList.end(), conn ) ;
            if ( iter != _busyList.end() )
            {
               tmp = *iter ;
               _busyList.erase( iter ) ;
               _busySize.dec() ;
               if ( _conf.getKeepAliveTimeout() > 0 &&
                  _keepAliveTimeOut( tmp ) )
               {
                  goto error ;
               }
               else
               {
                  bson::BSONObj obj ;
                  obj = BSON( "PreferedInstance" << "A" ) ;
                  rc = tmp->setSessionAttr( obj ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  rc = tmp->closeAllCursors() ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  _idleList.push_back( tmp ) ;
                  _idleSize.inc() ;
                  _strategy->sync( tmp, ADDIDLECONN ) ;
               }
            }
         } // secondly check _isEnabled
         
      } // firstly check _isEnabled
   done :
      if ( TRUE == isLock )
      {
         _connMutex.release() ;
      }
      return ;
   error :
      _strategy->sync( tmp, DELBUSYCONN ) ;
      if ( tmp )
      {
         tmp->disconnect() ;
      }
      SAFE_OSS_DELETE( tmp ) ;
      goto done ;
   }

   BOOLEAN sdbDataSource::_tryGetConn( sdb*& conn )
   {
      sdb* pConn    = NULL ;
      BOOLEAN isGet = FALSE ;

      _connMutex.get() ;
      if ( 0 == _idleSize.peek() )
      {
         _connMutex.release() ;
         goto error ;
      }
      else
      {
         pConn = _idleList.front() ;// TODO: empty or has a null value
         SDB_ASSERT(pConn, "the connection got from idle list can't be null") ;
         _idleList.pop_front() ;
         _idleSize.dec() ;
         _busyList.push_back( pConn ) ;
         _busySize.inc() ;
         _connMutex.release() ;
         _strategy->sync( pConn, ADDBUSYCONN ) ;
      }
         
      if ( _conf.getValidateConnection() )
      {
         if ( !pConn->isValid() )
         {
            list<sdb*>::iterator iter ;

            _connMutex.get() ;
            iter = std::find(_busyList.begin(), _busyList.end(), pConn) ;
            if (iter != _busyList.end())
            {
               _busyList.erase(iter) ;
               _busySize.dec() ;
            }
            _connMutex.release() ;
            _strategy->sync( pConn, DELBUSYCONN ) ;
            if ( pConn )
            {
               pConn->disconnect() ;
            }
            SAFE_OSS_DELETE( pConn ) ;
            goto error ;
         }
      }
      conn = pConn ;
      isGet = TRUE ;
   done :
      return isGet  ;
   error :
      isGet = FALSE ;
      goto done ;
   }

   BOOLEAN sdbDataSource::_checkAddrArg( const string &url )
   {
      BOOLEAN rc = TRUE ;

      size_t pos = url.find_first_of( ":" ) ;
      size_t pos1 = url.find_last_of( ":" ) ;
      if ( string::npos == pos )
         rc = FALSE ;
      else if ( pos != pos1 )
         rc = FALSE ;
      
      return rc ;
   }

   INT32 sdbDataSource::_buildStrategy()
   {
      INT32 rc = SDB_OK ;
      
      if ( _strategy )
         SAFE_OSS_DELETE( _strategy ) ;
      switch( _conf.getConnectStrategy() )
      {
      case DS_STY_SERIAL:
         _strategy = SDB_OSS_NEW sdbDSSerialStrategy() ;
         break ;
      case DS_STY_RANDOM:
         _strategy = SDB_OSS_NEW sdbDSRandomStrategy() ;
         break ;
      case DS_STY_LOCAL:
         _strategy = SDB_OSS_NEW sdbDSLocalStrategy() ;
         break ;
      case DS_STY_BALANCE:
         _strategy = SDB_OSS_NEW sdbDSBalanceStrategy() ;
         break ;
      }
      if (NULL == _strategy)
      {
         rc = SDB_OOM ;
      }
      return rc ;
   }


   void sdbDataSource::_clearDataSource()
   {      
      list<sdbclient::sdb*>::const_iterator iter ;
      sdbclient::sdb* conn = NULL ;

      for ( iter = _idleList.begin() ; iter != _idleList.end() ; ++iter )
      {
         conn = *iter ;
         if ( conn )
         {
            conn->disconnect() ;
            SAFE_OSS_DELETE( conn ) ;
         }
      }  
      _idleList.clear() ;
      _idleSize.init( 0 ) ;

      for ( iter = _busyList.begin() ; iter != _busyList.end() ; ++iter )
      {
         conn = *iter ;
         if ( conn )
         {
            conn->disconnect() ;
            SAFE_OSS_DELETE( conn) ;
         }
      }
      _busyList.clear() ; 
      _busySize.init( 0 ) ;

      for ( iter = _destroyList.begin() ; iter != _destroyList.end() ; ++iter )
      {
         conn = *iter ;
         if (conn)
         {
            conn->disconnect() ;
            SAFE_OSS_DELETE( conn ) ;
         }
      }
      _destroyList.clear() ;

      if ( _createConnWorker )
      {
         SAFE_OSS_DELETE( _createConnWorker ) ;
      }   
      if ( _destroyConnWorker )
      {
         SAFE_OSS_DELETE( _destroyConnWorker ) ;
      }   
      if ( _bgTaskWorker )
      {
         SAFE_OSS_DELETE( _bgTaskWorker ) ;
      }
     
   }


   void sdbDataSource::_createConn()
   {
      while ( !_toStopWorkers )
      {
         while ( !_toCreateConn )
         {
            if ( _toStopWorkers )
               return ;
            ossSleep( SDB_DS_SLEEP_TIME ) ;
         }
         _connMutex.get() ;
         INT32 idleNum = _idleSize.peek() ;
         INT32 busyNum = _busySize.peek() ;
         _connMutex.release() ;
         INT32 maxNum = _conf.getMaxCount() ;
         INT32 createNum = 0 ;
         if ( idleNum + busyNum < maxNum )
         {
            INT32 incNum = _conf.getDeltaIncCount() ;
            if ( idleNum + busyNum + incNum < maxNum )
               createNum = incNum ;
            else
               createNum = maxNum - idleNum - busyNum ;
         }
         _createConnByNum( createNum ) ;         
         _toCreateConn = FALSE ;
      }
   }

   INT32 sdbDataSource::_createConnByNum( INT32 num )
   {
      INT32 rc     = SDB_OK ;
      INT32 crtNum = 0 ;
      sdb* conn    = NULL ;
      
      while( crtNum < num )
      {
         INT32 pos = 0 ;
         string coord ;

         rc = _strategy->getNextCoord(coord) ;
         if ( SDB_OK != rc )
         {
            INT32 cnt = _retrieveAddrFromAbnormalList() ;
            if ( 0 == cnt )
            {
               break ;
            }
            else
            {
               continue ;
            }
         }

         conn = new(std::nothrow) sdb( _conf.getUseSSL() ) ;
         if ( NULL == conn ) 
         {
            break ;
         }
         pos = coord.find_first_of( ":" ) ;
         rc = conn->connect( 
            coord.substr(0, pos).c_str(), 
            coord.substr(pos+1, coord.length()).c_str(), 
            _conf.getUserName().c_str(),
            _conf.getPasswd().c_str() ) ;

         if ( SDB_OK == rc )
         {
            if ( _addNewConnSafely(conn, coord) )
            {
               ++crtNum ;
            }
            else
            {
               conn->disconnect();
               SAFE_OSS_DELETE( conn ) ;
               break ;
            }
            continue ;
         } // connect success
         else
         {
            INT32 retryTime = 0 ;
            BOOLEAN toBreak = FALSE ;
            while ( retryTime < SDB_DS_CREATECONN_RETRYTIME )
            {
               rc = conn->connect(
                  coord.substr( 0, pos ).c_str(), 
                  coord.substr( pos+1, coord.length() ).c_str(), 
                  _conf.getUserName().c_str(),
                  _conf.getPasswd().c_str() ) ;
               if ( SDB_OK != rc )
               {
                  ++retryTime ;
                  ossSleep( SDB_DS_SLEEP_TIME ) ; // TODO: (new) why we need to
                  continue ;
               } // retry failed
               else
               {
                  if ( _addNewConnSafely(conn, coord) )
                  {
                     ++crtNum ;
                  }
                  else
                  {
                     conn->disconnect();
                     SAFE_OSS_DELETE( conn ) ;
                     toBreak = TRUE ;
                  }
                  break ;
               } // retry success
            } // while for retry
            if ( retryTime == SDB_DS_CREATECONN_RETRYTIME )
            {
               SAFE_OSS_DELETE( conn ) ;
               _strategy->mvCoordToAbnormal( coord ) ;
            }
            if ( toBreak )
            {
               break;
            }
         } // connect failed

      } // while

      return crtNum ;
   }

   BOOLEAN sdbDataSource::_addNewConnSafely( sdb *conn, 
                                             const string &coord )
   {
      BOOLEAN ret = FALSE ;
      
      INT32 maxNum = _conf.getMaxCount() ;
      
      _connMutex.get() ;
      INT32 idleNum = _idleSize.peek() ;
      INT32 busyNum = _busySize.peek() ;
      if ( idleNum + busyNum < maxNum )
      {
         _idleList.push_back( conn ) ;
         _idleSize.inc() ;
         _strategy->syncAddNewConn( conn, coord ) ;
         ret = TRUE ; 
      }
      _connMutex.release() ;

      return ret ;
   }
/*
#if defined (_DEBUG)
      void sdbDataSource::printCreateInfo(const string& coord)
      {
         std::cout << "create a connection at " << coord << std::endl ;
      }
#endif
*/
   void sdbDataSource::_destroyConn()
   {
      while ( !_toStopWorkers )
      {
         while ( !_toDestroyConn )
         {
            if ( _toStopWorkers )
               return ;
            ossSleep( SDB_DS_SLEEP_TIME ) ;
         }
         sdb* conn ;
         _connMutex.get() ;
         while( !_destroyList.empty() )
         {
            conn = _destroyList.front() ;
            _destroyList.pop_front() ;
            if ( conn )
            {
               _strategy->sync( conn, DELIDLECONN ) ;
               conn->disconnect() ;
               SAFE_OSS_DELETE( conn ) ;
            }
         }
         _connMutex.release() ;
         _toDestroyConn = FALSE ;
      }
   }

   void sdbDataSource::_bgTask()
   {
      INT64 syncCoordInterval = _conf.getSyncCoordInterval() ;
      INT64 ckAbnormalInterval = SDB_DS_CHECKUNNORMALCOORD_INTERVAL ;
      INT64 ckConnInterval = _conf.getCheckInterval() ;
      INT64 syncCoordTimeCnt = 0 ;
      INT64 ckAbnormalTimeCnt = 0 ;
      INT64 ckConnTimeCnt = 0 ; 
      while ( !_toStopWorkers )
      {
         ossSleep( SDB_DS_SLEEP_TIME ) ;
         if ( syncCoordInterval > 0 )
         {
            syncCoordTimeCnt += SDB_DS_SLEEP_TIME ;
         }
         ckAbnormalTimeCnt += SDB_DS_SLEEP_TIME ;
         ckConnTimeCnt += SDB_DS_SLEEP_TIME ;
         if ( syncCoordInterval > 0 && syncCoordTimeCnt >= syncCoordInterval )
         {
            _syncCoordNodes() ;
            syncCoordTimeCnt = 0 ;
         }
         if ( ckAbnormalTimeCnt >= ckAbnormalInterval )
         {
            _retrieveAddrFromAbnormalList() ;
            ckAbnormalTimeCnt = 0 ;
         }
         if ( ckConnTimeCnt >= ckConnInterval )
         {
/*
#if defined (_DEBUG)
cout << "ckConnTimeCnt is: " << ckConnTimeCnt << endl ;
#endif
*/
            _checkMaxIdleConn() ;
            ckConnTimeCnt = 0 ;
         }

      }
   }

   void sdbDataSource::_syncCoordNodes()
   {
      INT32 rc ;
      string tmp ;
      sdb conn ;
      rc = _strategy->getNextCoord( tmp ) ;
      if (SDB_OK != rc)
         return ;
      INT32 pos = tmp.find_first_of( ":" ) ;
      rc = conn.connect( 
         tmp.substr(0, pos).c_str(), 
         tmp.substr(pos+1, tmp.length()).c_str(), 
         _conf.getUserName().c_str(),
         _conf.getPasswd().c_str() ) ;
      if ( SDB_OK != rc )
      {
         conn.disconnect() ;
         return ;
      }
      else
      {
         sdbReplicaGroup group ;
         rc = conn.getReplicaGroup( "SYSCoord", group ) ;
         if ( SDB_OK != rc )
         {
            conn.disconnect() ;
            return ;
         }
         bson::BSONObj obj ;
         group.getDetail( obj ) ;

         bson::BSONElement eleGroup = obj.getField( "Group" ) ;
         bson::BSONObjIterator itr( eleGroup.embeddedObject() ) ;
         while( itr.more() )
         {
            string newcoord ;
            bson::BSONObj hostItem  ;
            bson::BSONElement hostElement = itr.next()  ;
            hostItem = hostElement.embeddedObject()  ;
            const CHAR* pname = hostItem.getField( "HostName" ).valuestr() ;
            newcoord.append(pname).append( ":" ) ;
            bson::BSONElement serviceEle  = hostItem.getField( "Service" ) ;
            bson::BSONObjIterator itrPort( serviceEle.embeddedObject() )  ;
            if ( itrPort.more() )
            {
               bson::BSONObj portItem  ;
               bson::BSONElement portEle = itrPort.next() ;
               portItem = portEle.embeddedObject() ;
               newcoord.append(portItem.getField( "Name" ).valuestr()) ;
               addCoord(newcoord) ;
            }
         }
      }
   }

   INT32 sdbDataSource::_retrieveAddrFromAbnormalList()
   {
      INT32 rc          = SDB_OK ;
      INT32 j           = 0 ;
      INT32 count       = 0 ;
      INT32 pos         = 0 ;
      INT32 abnormalNum = 0 ;
      string tmp ;
      sdb conn ;

      abnormalNum = _strategy->getAbnormalCoordNum() ;
      for ( j = 0 ; j < abnormalNum ; ++j ) 
      {
         if ( SDB_EOF == _strategy->getNextAbnormalCoord( tmp ) )
         {
            break ;
         }
         pos = tmp.find_first_of( ":" ) ;
         rc = conn.connect( 
            tmp.substr(0, pos).c_str(), 
            tmp.substr(pos+1, tmp.length()).c_str(), 
            _conf.getUserName().c_str(),
            _conf.getPasswd().c_str() ) ;
         if ( SDB_OK == rc )
         {
            ++count ;
            _strategy->mvCoordToNormal( tmp ) ;
            conn.disconnect();
         }
      }
      return count ;
   }

   BOOLEAN sdbDataSource::_keepAliveTimeOut( sdb *conn )
   {
      INT32 checkInterval = _conf.getCheckInterval() ;
      INT32 keepAlive = _conf.getKeepAliveTimeout() ;
      
      time_t nowTime ;
      time( &nowTime ) ;
      INT32 diffTime = difftime( nowTime, conn->getLastAliveTime() ) * 1000 ;
      if ( 0 > diffTime || 
           diffTime + SDB_DS_MULTIPLE * checkInterval > keepAlive )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   void sdbDataSource::_checkMaxIdleConn()
   {
      INT32 freeNum       = 0 ;
      sdb* conn           = NULL ;
      INT32 maxIdleNum    = _conf.getMaxIdleCount() ;
      INT32 aliveTime     = _conf.getKeepAliveTimeout() ;
      INT32 checkInterval = _conf.getCheckInterval() ;
      
      _connMutex.get() ;
      if ( 0 < aliveTime )
      {
         list<sdb*>::iterator iter ;
         time_t nowTime ;
         time( &nowTime ) ;
         for ( iter = _idleList.begin() ; iter != _idleList.end() ; )
         {
            INT32 diffTime = 
               difftime( nowTime, (*iter)->getLastAliveTime() ) * 1000 ;
/*
#if defined (_DEBUG)
cout << "aliveTime is: " << aliveTime << ", diffTime is: " << diffTime ;
cout << ", diffTime + checkInterval * SDB_DS_MULTIPLE is:" << diffTime + checkInterval * SDB_DS_MULTIPLE << endl ;
#endif
*/
            if ( 0 > diffTime ||
               diffTime + checkInterval * SDB_DS_MULTIPLE >= aliveTime )
            {
               conn = *iter ;
               _idleList.erase( iter++ ) ;
               _idleSize.dec() ;
               _destroyList.push_back( conn ) ; 
/*
#if defined (_DEBUG)
   cout << "destroy a connection because of time out" << endl ;
#endif
*/
            }
            else
            {
               ++iter ;
            }
         }

      }

      if ( (INT32)_idleSize.peek() > maxIdleNum )
      {
         freeNum = _idleSize.peek() - maxIdleNum ;
      }
      while ( freeNum > 0 )
      {
         conn = _idleList.front() ;
         _idleList.pop_front() ;
         _idleSize.dec() ;
         _destroyList.push_back( conn ) ;   
         --freeNum ;
      }
      _toDestroyConn = TRUE ;
      _connMutex.release() ;
   }

   void sdbDataSource::close()
   {
      disable() ;
      if ( _strategy )
      {
         SAFE_OSS_DELETE( _strategy ) ;
      }
      _isInited = FALSE;
   }
}
