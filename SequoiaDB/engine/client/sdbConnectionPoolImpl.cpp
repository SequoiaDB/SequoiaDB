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

   Source File Name = sdbConnectionPool.cpp

   Descriptive Name = SDB Connection Pool Source File

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbConnectionPoolImpl.hpp"
#include "sdbConnectionPoolStrategy.hpp"
#include "client.hpp"
#include "sdbConnectionPoolWorker.hpp"
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
   #define SDB_CONNPOOL_SLEEP_TIME        100
   #define SDB_CONNPOOL_TRYGETCONN_TIME   3
   #define SDB_CONNPOOL_MULTIPLE          1.2

   void createConnFunc( void *args )
   {
      ((sdbConnectionPoolImpl*)args)->_createConn() ;
   }

   void destroyConnFunc( void *args )
   {
      ((sdbConnectionPoolImpl*)args)->_destroyConn() ;
   }

   void bgTaskFunc( void *args )
   {
      ((sdbConnectionPoolImpl*)args)->_bgTask() ;
   }

   sdbConnectionPoolImpl::~sdbConnectionPoolImpl()
   {
      close() ;
      // clear strategy
      SAFE_OSS_DELETE(_strategy) ;
   }

   // init connection pool with address(hostname:port) and conf
   INT32 sdbConnectionPoolImpl::init(
      const string &address,
      const sdbConnectionPoolConf &conf )
   {
      std::vector<string> addrs ;
      addrs.push_back( address ) ;

      return init( addrs, conf ) ;
   }

   // init connection pool with address vector and conf
   INT32 sdbConnectionPoolImpl::init(
      const std::vector<string> &addrs,
      const sdbConnectionPoolConf &conf )
   {
      INT32 rc = SDB_OK ;
      int validAddrCnt = 0 ;

      if ( _isInited )
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
         SAFE_OSS_DELETE( _strategy ) ;
         goto error ;
      }

      // check address vector
      for ( UINT32 i = 0 ; i < addrs.size() ; ++i )
      {
         if ( _checkAddrArg( addrs[i] ) )
         {
            _strategy->addCoord( addrs[i] ) ;
            validAddrCnt++ ;
         }
      }
      //if no address is valid, return SDB_INVALIDARG
      if ( 0 == validAddrCnt )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _enable() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _isInited = TRUE ;
      _isClosed = FALSE ;
   done :
      return rc  ;
   error :
      goto done  ;
   }

   // get idle connection number
   INT32 sdbConnectionPoolImpl::getIdleConnNum() const
   {
      return _isInited ? _idleSize.peek() : -1 ;
   }

   // get used connection number
   INT32 sdbConnectionPoolImpl::getUsedConnNum() const
   {
      return _isInited ? _busySize.peek() : -1 ;
   }

   // get the number of normal coord node
   INT32 sdbConnectionPoolImpl::getNormalAddrNum() const
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
   INT32 sdbConnectionPoolImpl::getAbnormalAddrNum() const
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
   INT32 sdbConnectionPoolImpl::getLocalAddrNum() const
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

   // enable connection pool, start background task
   INT32 sdbConnectionPoolImpl::_enable()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isLocked = FALSE ;

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
            INT32 retNum = 0 ;
            _createConnByNum( _conf.getInitConnCount(), retNum ) ;
            // start create connection thread
            _createConnWorker = SDB_OSS_NEW sdbConnPoolWorker(
               createConnFunc, this ) ;
            if ( NULL == _createConnWorker )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            // start destroy connection thread
            _destroyConnWorker = SDB_OSS_NEW sdbConnPoolWorker( destroyConnFunc,
                                                          this ) ;
            if ( NULL == _destroyConnWorker )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            // start background task thread
            _bgTaskWorker= SDB_OSS_NEW sdbConnPoolWorker( bgTaskFunc, this ) ;
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

   // disable connection pool, stop background task
   INT32 sdbConnectionPoolImpl::_disable()
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

            // clear connection pool
            _clearConnPool() ;

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

   INT32 sdbConnectionPoolImpl::getConnection( sdb*& conn, INT64 timeoutms )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isGet = FALSE ;

      if ( 0 > timeoutms )
      {
         timeoutms = 0 ;
      }

      if ( _isClosed )
      {
         rc = SDB_CLIENT_CONNPOOL_CLOSE ;
         goto error ;
      }
      if ( !_isInited )
      {
         rc = SDB_CLIENT_CONNPOOL_NOT_INIT ;
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
                  ossSleep( SDB_CONNPOOL_SLEEP_TIME ) ;
                  timeCnt += SDB_CONNPOOL_SLEEP_TIME ;
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
                  rc = SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD ;
                  goto error ;
               }
               else
               {
                  INT32 createNum = 0 ;
                  rc = _createConnByNum( 1, createNum );
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
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
         if ( _idleSize.peek() <= SDB_CONNPOOL_TOPRECREATE_THRESHOLD &&
              _conf.getMaxIdleCount() > 0 )
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
   void sdbConnectionPoolImpl::releaseConnection( sdb*& conn )
   {
      // TODO:  in java, before we call "close", the current function
      // can release connections
      BOOLEAN              isLock = FALSE ;
      INT32                rc = SDB_OK ;
      sdb*                 tmp = NULL ;

      if ( _isClosed || !_isInited )
      {
         goto done ;
      }

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
                  goto release ;
               }
               else if ( 0 == _conf.getMaxIdleCount() )
               {
                  goto release ;
               }
               else if ( !_strategy->checkAddress( tmp->getAddress() ) )
               {
                  goto release ;
               }
               else
               {
                  // close all cursors
                  rc = tmp->closeAllCursors() ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  _idleList.push_back( tmp ) ;
                  _idleSize.inc() ;
                  conn = NULL ;
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
   release :
      if ( tmp )
      {
         tmp->disconnect() ;
      }
      SAFE_OSS_DELETE( tmp ) ;
      conn = NULL ;
      goto done ;
   error :
      goto release ;
   }

   // try to get a connection
   BOOLEAN sdbConnectionPoolImpl::_tryGetConn( sdb*& conn )
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
   BOOLEAN sdbConnectionPoolImpl::_checkAddrArg( const string &address )
   {
      BOOLEAN rc = TRUE ;

      size_t pos = address.find_first_of( ":" ) ;
      size_t pos1 = address.find_last_of( ":" ) ;
      if ( string::npos == pos )
         rc = FALSE ;
      else if ( pos != pos1 )
         rc = FALSE ;

      return rc ;
   }

   // new a strategy with config
   INT32 sdbConnectionPoolImpl::_buildStrategy()
   {
      INT32 rc = SDB_OK ;

      if ( _strategy )
         SAFE_OSS_DELETE( _strategy ) ;
      switch( _conf.getConnectStrategy() )
      {
      case SDB_CONN_STY_SERIAL:
         _strategy = SDB_OSS_NEW sdbConnPoolSerialStrategy() ;
         break ;
      case SDB_CONN_STY_RANDOM:
         _strategy = SDB_OSS_NEW sdbConnPoolRandomStrategy() ;
         break ;
      case SDB_CONN_STY_LOCAL:
         _strategy = SDB_OSS_NEW sdbConnPoolLocalStrategy() ;
         break ;
      case SDB_CONN_STY_BALANCE:
         // balance strategy has been deprecated
         _strategy = SDB_OSS_NEW sdbConnPoolSerialStrategy() ;
         break ;

      }
      if (NULL == _strategy)
      {
         rc = SDB_OOM ;
      }
      return rc ;
   }


   // clear connection pool
   void sdbConnectionPoolImpl::_clearConnPool()
   {
      // clear connection list
      list<sdbclient::sdb*>::const_iterator iter ;
      sdbclient::sdb* conn = NULL ;

      _connMutex.get();
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
            SAFE_OSS_DELETE( conn ) ;
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
      _connMutex.release() ;

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
   void sdbConnectionPoolImpl::_createConn()
   {
      while ( !_toStopWorkers )
      {
         while ( !_toCreateConn )
         {
            if ( _toStopWorkers )
               return ;
            ossSleep( SDB_CONNPOOL_SLEEP_TIME ) ;
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
         INT32 retNum = 0 ;
         _createConnByNum( createNum, retNum );
         _toCreateConn = FALSE ;
      }
   }

   // create connection by a number
   // return the amount of connections we had created,
   // it may less than what we expect
   INT32 sdbConnectionPoolImpl::_createConnByNum( INT32 num,  INT32 &retNum )
   {
      INT32 rc     = SDB_OK ;
      INT32 count  = 0 ;
      sdb* conn    = NULL ;

      while( count < num )
      {
         string coordAddr ;

         // if no coord can be used
         rc = _strategy->getNextCoord(coordAddr) ;
         if ( SDB_OK != rc )
         {
            INT32 cnt = _retrieveAddrFromAbnormalList() ;
            if ( 0 == cnt )
            {
               // if have no any normal node, let's stop
               rc = SDB_CLIENT_CONNPOOL_NO_REACHABLE_COORD ;
               goto error ;
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
            rc = SDB_OOM ;
            goto error ;
         }
         // when we get a coord address, let's build the connection
         rc = _connect( conn, coordAddr ) ;
         if ( SDB_OK == rc )
         {
            if ( _addNewConnSafely(conn, coordAddr) )
            {
               ++count ;
               //#if defined (_DEBUG)
               //printCreateInfo(coord) ;
               //#endif
            }
            // may be reach max connection count
            else
            {
               conn->disconnect();
               SAFE_OSS_DELETE( conn ) ;
               goto done ;
            }
            continue ;
         } // connect success
         else if ( SDB_NETWORK == rc || SDB_NET_CANNOT_CONNECT == rc )
         {
            // if newtwork error, we will retry 3 times. on success,
            // let's save the connection; on error, let's discard
            // that coord address temporarily
            INT32 retryTime = 0 ;
            BOOLEAN toBreak = FALSE ;
            while ( retryTime < SDB_CONNPOOL_CREATECONN_RETRYTIME )
            {
               rc = _connect( conn, coordAddr ) ;
               if ( SDB_OK != rc )
               {
                  ++retryTime ;
                  ossSleep( SDB_CONNPOOL_SLEEP_TIME ) ; // TODO: (new) why we need to
                                                  // sleep?
                  continue ;
               } // retry failed
               else
               {
                  // add success
                  if ( _addNewConnSafely(conn, coordAddr) )
                  {
                     ++count ;
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
            if ( retryTime == SDB_CONNPOOL_CREATECONN_RETRYTIME )
            {
               SAFE_OSS_DELETE( conn ) ;
               _strategy->mvCoordToAbnormal( coordAddr ) ;
            }
            if ( toBreak )
            {
               goto done ;
            }
         } // connect failed
         else
         {
            SAFE_OSS_DELETE( conn ) ;
            goto error ;
         }
      } // while

   done:
      retNum = count ;
      return rc ;
   error:
      goto done ;
   }

   // add new connection and make sure not reach max connection count
   BOOLEAN sdbConnectionPoolImpl::_addNewConnSafely( sdb *conn,
                                             const string &coord )
   {
      BOOLEAN ret = FALSE ;

      INT32 maxNum = _conf.getMaxCount() ;

      _connMutex.get() ;
      INT32 idleNum = _idleSize.peek() ;
      INT32 busyNum = _busySize.peek() ;
      if ( idleNum + busyNum < maxNum && _strategy->checkAddress( coord ) )
      {
         _idleList.push_back( conn ) ;
         _idleSize.inc() ;
         ret = TRUE ;
      }
      _connMutex.release() ;

      return ret ;
   }
/*
#if defined (_DEBUG)
      void sdbConnectionPoolImpl::printCreateInfo(const string& coord)
      {
         std::cout << "create a connection at " << coord << std::endl ;
      }
#endif
*/
   //destroy connections function
   void sdbConnectionPoolImpl::_destroyConn()
   {
      while ( !_toStopWorkers )
      {
         while ( !_toDestroyConn )
         {
            if ( _toStopWorkers )
               return ;
            ossSleep( SDB_CONNPOOL_SLEEP_TIME ) ;
         }
         sdb* conn ;
         _connMutex.get() ;
         while( !_destroyList.empty() )
         {
            conn = _destroyList.front() ;
            _destroyList.pop_front() ;
            if ( conn )
            {
               conn->disconnect() ;
               SAFE_OSS_DELETE( conn ) ;
            }
         }
         _connMutex.release() ;
         _toDestroyConn = FALSE ;
      }
   }

   INT32 sdbConnectionPoolImpl::_connect( sdb *conn, const string &address )
   {
      INT32 rc = SDB_OK ;
      string username ;
      string pwd ;
      string token ;
      string cipherFile ;

      _confShare.get_shared() ;
      username = _conf.getUserName() ;
      pwd = _conf.getPasswd() ;
      token = _conf.getToken() ;
      cipherFile = _conf.getCipherFile() ;
      _confShare.release_shared() ;

      if ( "" == pwd && "" != cipherFile )
      {
         const INT32 size = 1;
         const CHAR *pConnAddrs[size] ;
         pConnAddrs[0] = address.c_str() ;
         rc = conn->connect( pConnAddrs, size,
                             username.c_str(),
                             token.c_str(),
                             cipherFile.c_str() ) ;
      }
      else
      {
         INT32 pos = address.find_first_of( ":" ) ;
         rc = conn->connect( address.substr( 0, pos ).c_str(),
                             address.substr( pos + 1, address.length() ).c_str(),
                             username.c_str(),
                             pwd.c_str() ) ;
      }
      return rc ;
   }

   // background task function
   void sdbConnectionPoolImpl::_bgTask()
   {
      INT64 syncCoordInterval = _conf.getSyncCoordInterval() ;
      INT64 ckAbnormalInterval = SDB_CONNPOOL_CHECKUNNORMALCOORD_INTERVAL ;
      INT64 ckConnInterval = _conf.getCheckInterval() ;
      INT64 syncCoordTimeCnt = 0 ;
      INT64 ckAbnormalTimeCnt = 0 ;
      INT64 ckConnTimeCnt = 0 ;
      while ( !_toStopWorkers )
      {
         ossSleep( SDB_CONNPOOL_SLEEP_TIME ) ;
         if ( syncCoordInterval > 0 )
         {
            syncCoordTimeCnt += SDB_CONNPOOL_SLEEP_TIME ;
         }
         ckAbnormalTimeCnt += SDB_CONNPOOL_SLEEP_TIME ;
         ckConnTimeCnt += SDB_CONNPOOL_SLEEP_TIME ;
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
   void sdbConnectionPoolImpl::_syncCoordNodes()
   {
      INT32 rc ;
      string tmp ;
      sdb conn ;
      rc = _strategy->getNextCoord( tmp ) ;
      if (SDB_OK != rc)
         return ;

      rc = _connect( &conn, tmp ) ;
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
               _strategy->addCoord( newcoord ) ;
            }
         }
      }
   }

   // get back the coord address from the abnormal address list
   // and return the amount of normal coord address which we
   // have retrieved
   INT32 sdbConnectionPoolImpl::_retrieveAddrFromAbnormalList()
   {
      // try abnormal coord
      INT32 rc          = SDB_OK ;
      INT32 j           = 0 ;
      INT32 count       = 0 ;
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
         rc = _connect( &conn, tmp ) ;
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
   BOOLEAN sdbConnectionPoolImpl::_keepAliveTimeOut( sdb *conn )
   {
      INT32 checkInterval = _conf.getCheckInterval() ;
      INT32 keepAlive = _conf.getKeepAliveTimeout() ;

      time_t nowTime ;
      time( &nowTime ) ;
      INT32 diffTime = difftime( nowTime, conn->getLastAliveTime() ) * 1000 ;
      if ( 0 > diffTime ||
           diffTime + SDB_CONNPOOL_MULTIPLE * checkInterval > keepAlive )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   // check max connection number intervally
   void sdbConnectionPoolImpl::_checkMaxIdleConn()
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
cout << ", diffTime + checkInterval * SDB_CONNPOOL_MULTIPLE is:" << diffTime + checkInterval * SDB_CONNPOOL_MULTIPLE << endl ;
#endif
*/
            if ( 0 > diffTime ||
               diffTime + checkInterval * SDB_CONNPOOL_MULTIPLE >= aliveTime )
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

   // close connection pool
   INT32 sdbConnectionPoolImpl::close()
   {
      INT32 rc = SDB_OK ;
      if ( _isClosed )
      {
         goto done ;
      }
	  if ( !_isInited )
      {
         rc = SDB_CLIENT_CONNPOOL_NOT_INIT ;
         goto error ;
      }

      rc = _disable() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _isInited = FALSE ;
      _isClosed = TRUE ;
   done :
      return rc  ;
   error :
      goto done  ;
   }

   void sdbConnectionPoolImpl::updateAuthInfo( const string &username,
                                           const string &passwd )
   {
      _confShare.get() ;
      // clear old user, cipherFile, token
      _conf.setAuthInfo( "", "", "" ) ;
      _conf.setAuthInfo( username, passwd ) ;
      _confShare.release() ;
   }

   void sdbConnectionPoolImpl::updateAuthInfo( const string &username,
                                           const string &cipherFile,
                                           const string &token )
   {
      _confShare.get() ;
      // clear old user, pwd
      _conf.setAuthInfo( "", "" ) ;
      _conf.setAuthInfo( username, cipherFile, token ) ;
      _confShare.release() ;
   }

   INT32 sdbConnectionPoolImpl::updateAddress( const std::vector<string> &addrs )
   {
      INT32 rc            = SDB_OK ;
      sdb* conn           = NULL ;
      string address      = "" ;
      BOOLEAN ret         = TRUE ;
      std::vector<string> delAddrs ;
      list<sdb*>::iterator iter ;

      if ( _isClosed )
      {
         rc = SDB_CLIENT_CONNPOOL_CLOSE ;
         goto error ;
      }
      if ( !_isInited )
      {
         rc = SDB_CLIENT_CONNPOOL_NOT_INIT ;
         goto error ;
      }

      // 1. check parameter
      for ( UINT32 i = 0 ; i < addrs.size() ; ++i )
      {
         if ( !_checkAddrArg( addrs[i] ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      // 2. update address
      _connMutex.get() ;
      ret = _strategy->updateAddress( addrs, delAddrs ) ;
      if ( !ret )
      {
         rc = SDB_INVALIDARG ;
         _connMutex.release() ;
         goto error ;
      }

      // 3. clear conn from idle pool
      for ( iter = _idleList.begin() ; iter != _idleList.end() ; )
      {
         conn = *iter ;
         address = conn->getAddress() ;
         if ( delAddrs.end() != std::find( delAddrs.begin(),
              delAddrs.end(), address ) )
         {
            iter = _idleList.erase( iter ) ;
            _idleSize.dec() ;
            _destroyList.push_back( conn ) ;
         }
         else
         {
            ++iter ;
         }
      }
      _connMutex.release() ;

   done:
      return rc ;
   error:
      goto done ;
   }
}
