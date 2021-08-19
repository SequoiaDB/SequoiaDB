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

//#if defined (_DEBUG)
//#include <iostream>
//#endif

using std::string;
using std::list;

namespace sdbclient
{
   // sleep time:0.1s
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
      // clear strategy
      SAFE_OSS_DELETE(_strategy) ;
   }
   
   // init data source with url(hostname:port) and conf
   INT32 sdbDataSource::init(
      const string &url, 
      const sdbDataSourceConf &conf )
   {
      std::vector<string> vUrl ;
      vUrl.push_back( url ) ;
      
      return init( vUrl, conf ) ;
   }

   // init data source with url vector and conf
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
      // check confiture
      if ( !_conf.isValid() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      
      // new strategy instance
      rc = _buildStrategy();
      if ( SDB_OK != rc )
      {
         SAFE_OSS_DELETE(_strategy) ;
         goto error ;
      }
      
      // check address vector
      for ( UINT32 i = 0 ; i < vUrls.size() ; ++i )
      {
         if ( _checkAddrArg( vUrls[i] ) )
         {   
            _strategy->addCoord( vUrls[i] ) ;
            validUrlCnt++ ;
         }
      }
      //if no address is valid, return SDB_INVALIDARG
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
   
   // get idle connection number
   INT32 sdbDataSource::getIdleConnNum() const
   {
      return _isInited ? _idleSize.peek() : -1 ;
   }

   // get used connection number
   INT32 sdbDataSource::getUsedConnNum() const
   {
      return _isInited ? _busySize.peek() : -1 ;
   }

   // get the number of normal coord node
   INT32 sdbDataSource::getNormalCoordNum() const
   {
      if ( _isInited ) 
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         return _strategy->getNormalCoordNum() ;
      } 
      else 
      {
         return -1 ;
      }
   }

   // get the number of abnormal coord node
   INT32 sdbDataSource::getAbnormalCoordNum() const
   {  
      if ( _isInited )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         return _strategy->getAbnormalCoordNum() ;
      }
      else
      {
         return -1 ;
      }
   }

   // get the number of local coord node
   INT32 sdbDataSource::getLocalCoordNum() const
   {
      if ( _isInited )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         return _strategy->getLocalCoordNum() ;
      }
      else
      {
         return -1 ;
      }
   }
   
   // add a coord node
   void sdbDataSource::addCoord( const string &url )
   {
      // check address argument
      if ( _isInited && _checkAddrArg(url) )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         _strategy->addCoord( url ) ;
      }
   }

   // remove a coord node
   void sdbDataSource::removeCoord( const string &url )
   {
      // check address argument
      if ( _isInited && _checkAddrArg(url) )
      {
         SDB_ASSERT( _strategy, "_strategy is null" ) ;
         _strategy->removeCoord( url ) ;
      }
   }

   // enable data source, start background task
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
               // prepare some connections
               _createConnByNum( _conf.getInitConnCount() ) ;
               // start create connection thread
               _createConnWorker = SDB_OSS_NEW sdbDSWorker( 
                  createConnFunc, this ) ;
               if ( NULL == _createConnWorker )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               // start destroy connection thread
               _destroyConnWorker = SDB_OSS_NEW sdbDSWorker( destroyConnFunc, 
                                                             this ) ;
               if ( NULL == _destroyConnWorker )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
               // start background task thread
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
      // TODO: (new) should we need to stop the 
      // background thread before we release it ?
      SAFE_OSS_DELETE(_createConnWorker) ;
      SAFE_OSS_DELETE(_destroyConnWorker) ;
      SAFE_OSS_DELETE(_bgTaskWorker) ;
      goto done  ;
   }

   // disable data source, stop background task
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
            // join
            rc = _createConnWorker->waitStop() ;
            if ( SDB_OK != rc )
            {
               ret = rc ;
               goto error ;// TODO:(new) when it failed, we leave the 
                           //other background thread alone?
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

            // clear data source
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
         // TODO: (new) before we call "close", "getConnection"
         // can return a connection in java,
         // but here, the behave is different
         rc = SDB_DS_NOT_ENABLE ;
         goto error ;
      }
      
      while ( !isGet )
      {
         // if has idle connection
         if ( _idleSize.peek() > 0 )
         {
            isGet = _tryGetConn( conn ) ;
         }
         // if has no idle connection
         else
         {
            // if reach max count, wait for timeout, try again
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
            // not reach max count
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
                  // get lock1
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
                     // release lock1
                     _connMutex.release() ;
                     conn = pConn ;
                     _strategy->sync( pConn, ADDBUSYCONN ) ;
                     isGet = TRUE ;
                  }
                  else
                  {
                     // release lock1
                     _connMutex.release() ;
                  }
               } // has reachable coord
            } // not reach max count
         } // _idleSize <= 0
         
         // if need pre create some connection
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

   // give back a connection
   void sdbDataSource::releaseConnection( sdb *conn )
   {
      // TODO:  in java, before we call "close", the current function
      // can release connections
      BOOLEAN              isLock = FALSE ;
      INT32                rc = SDB_OK ;
      sdb*                 tmp = NULL ;

      // TODO: (new) when _isEnabled is false, what will happen??
      if ( _isEnabled )
      {
         list<sdb*>::iterator iter ;
         _connMutex.get() ; 
         isLock = TRUE ;
         if ( _isEnabled )
         {
            iter = std::find( _busyList.begin(), _busyList.end(), conn ) ;
            // if find it
            if ( iter != _busyList.end() )
            {
               tmp = *iter ;
               _busyList.erase( iter ) ;
               _busySize.dec() ;
               // if check keep time out, drop it
               if ( _conf.getKeepAliveTimeout() > 0 &&
                  _keepAliveTimeOut( tmp ) )
               {
                  goto error ;
               }
               else
               {
                  // reset session attributions
                  bson::BSONObj obj ;
                  obj = BSON( "PreferedInstance" << "A" ) ;
                  rc = tmp->setSessionAttr( obj ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  // close all cursors
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

   // try to get a connection
   BOOLEAN sdbDataSource::_tryGetConn( sdb*& conn )
   {
      sdb* pConn    = NULL ;
      BOOLEAN isGet = FALSE ;

      // get lock1
      _connMutex.get() ;
      if ( 0 == _idleSize.peek() )
      {
         // release lock1
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
         // release lock1
         _busySize.inc() ;
         _connMutex.release() ;
         // update strategy
         _strategy->sync( pConn, ADDBUSYCONN ) ;
      }
         
      // if check valid
      if ( _conf.getValidateConnection() )
      {
         // if not valid, destroy it
         if ( !pConn->isValid() )
         {
            list<sdb*>::iterator iter ;

            // get lock2
            _connMutex.get() ;
            iter = std::find(_busyList.begin(), _busyList.end(), pConn) ;
            // if find it
            if (iter != _busyList.end())
            {
               _busyList.erase(iter) ;
               _busySize.dec() ;
            }
            // release lock2
            _connMutex.release() ;
            // update strategy
            _strategy->sync( pConn, DELBUSYCONN ) ;
            if ( pConn )
            {
               pConn->disconnect() ;
            }
            SAFE_OSS_DELETE( pConn ) ;
            goto error ;
         }
      }
      // on success
      conn = pConn ;
      isGet = TRUE ;
   done :
      return isGet  ;
   error :
      isGet = FALSE ;
      goto done ;
   }

   // check address arguments, if valid, add it
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

   // new a strategy with config
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


   // clear data source
   void sdbDataSource::_clearDataSource()
   {      
      // clear connection list
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

      // release thread worker
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


   // create connections function
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

   // create connection by a number
   // return the amount of connections we had created,
   // it may less than what we expect
   INT32 sdbDataSource::_createConnByNum( INT32 num )
   {
      INT32 rc     = SDB_OK ;
      INT32 crtNum = 0 ;
      sdb* conn    = NULL ;
      
      while( crtNum < num )
      {
         INT32 pos = 0 ;
         string coord ;

         // if no coord can be used
         rc = _strategy->getNextCoord(coord) ;
         if ( SDB_OK != rc )
         {
            INT32 cnt = _retrieveAddrFromAbnormalList() ;
            if ( 0 == cnt )
            {
               // if have no any normal node, let's stop
               break ;
            }
            else
            {
               // otherwise, let's keep working
               continue ;
            }
         }

         // TODO:
         // sdb is not inherit form SDBObject
         // so we can't use SDB_OSS_NEW. but, 
         // let's do it later.
         conn = new(std::nothrow) sdb( _conf.getUseSSL() ) ;
         if ( NULL == conn ) 
         {
            // OOM, let's stop working
            break ;
         }
         // when we get a coord address, let's build the connection
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
               //#if defined (_DEBUG)
               //printCreateInfo(coord) ;
               //#endif
            }
            // may be reach max connection count
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
            // if connect failed, we will retry 3 times. on success, 
            // let's save the connection; on error, let's discard
            // that coord address temporarily
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
                                                  // sleep?
                  continue ;
               } // retry failed
               else
               {
                  // add success
                  if ( _addNewConnSafely(conn, coord) )
                  {
                     ++crtNum ;
                     //#if defined (_DEBUG)
                     //printCreateInfo(coord) ;
                     //#endif
                  }
                  // fail, may be reach max connection count
                  else
                  {
                     conn->disconnect();
                     SAFE_OSS_DELETE( conn ) ;
                     toBreak = TRUE ;
                  }
                  // stop retrying
                  break ;
               } // retry success
            } // while for retry
            // after retry time, still failed
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

   // add new connection and make sure not reach max connection count
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
   //destroy connections function
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

   // background task function
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
         // try to sync coord address
         if ( syncCoordInterval > 0 && syncCoordTimeCnt >= syncCoordInterval )
         {
            _syncCoordNodes() ;
            syncCoordTimeCnt = 0 ;
         }
         // try to retrieve addr from abmornal addr list
         if ( ckAbnormalTimeCnt >= ckAbnormalInterval )
         {
            _retrieveAddrFromAbnormalList() ;
            ckAbnormalTimeCnt = 0 ;
         }
         // try to check the connections in idle list
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

   // sync coord node info
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
         // get coord node
         sdbReplicaGroup group ;
         rc = conn.getReplicaGroup( "SYSCoord", group ) ;
         if ( SDB_OK != rc )
         {
            conn.disconnect() ;
            return ;
         }
         bson::BSONObj obj ;
         group.getDetail( obj ) ;

         // get coord group
         bson::BSONElement eleGroup = obj.getField( "Group" ) ;
         bson::BSONObjIterator itr( eleGroup.embeddedObject() ) ;
         // loop through
         while( itr.more() )
         {
            string newcoord ;
            // get host name
            bson::BSONObj hostItem  ;
            bson::BSONElement hostElement = itr.next()  ;
            hostItem = hostElement.embeddedObject()  ;
            const CHAR* pname = hostItem.getField( "HostName" ).valuestr() ;
            newcoord.append(pname).append( ":" ) ;
            // port group
            bson::BSONElement serviceEle  = hostItem.getField( "Service" ) ;
            bson::BSONObjIterator itrPort( serviceEle.embeddedObject() )  ;
            if ( itrPort.more() )
            {
               // get port
               bson::BSONObj portItem  ;
               bson::BSONElement portEle = itrPort.next() ;
               portItem = portEle.embeddedObject() ;
               newcoord.append(portItem.getField( "Name" ).valuestr()) ;
               addCoord(newcoord) ;
            }
         }
      }
   }

   // get back the coord address from the abnormal address list
   // and return the amount of normal coord address which we
   // have retrieved
   INT32 sdbDataSource::_retrieveAddrFromAbnormalList()
   {
      // try abnormal coord
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
            // when have no addresses in abnormal coord address list,
            // let's stop
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
            // TODO: (new) waste of time, we should try to 
            // save this connection instead of disconnecting it
            conn.disconnect();
         }
      }
      return count ;
   }

   // check keep alive time out or not
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

   // check max connection number intervally
   void sdbDataSource::_checkMaxIdleConn()
   {
      INT32 freeNum       = 0 ;
      sdb* conn           = NULL ;
      INT32 maxIdleNum    = _conf.getMaxIdleCount() ;
      INT32 aliveTime     = _conf.getKeepAliveTimeout() ;
      INT32 checkInterval = _conf.getCheckInterval() ;
      
      _connMutex.get() ;
      // 1. destroy the idle connections which are out of date
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

      // 2. destroy the idle connections which is more than the idle num
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

   // close data source
   void sdbDataSource::close()
   {
      disable() ;
      // clear strategy
      if ( _strategy )
      {
         SAFE_OSS_DELETE( _strategy ) ;
      }
      _isInited = FALSE;
   }
}
