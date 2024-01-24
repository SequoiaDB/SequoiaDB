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

   Source File Name = sdbConnectionPoolImpl.hpp

   Descriptive Name = SDB Connection Pool Include Header

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         11/06/2021   LSQ  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_CONNECTIONPOOL_IMPL_HPP_
#define SDB_CONNECTIONPOOL_IMPL_HPP_

#include "sdbConnectionPool.hpp"
#include "sdbConnectionPoolStrategy.hpp"
#include "sdbConnectionPoolWorker.hpp"
#include <list>

#ifndef SDB_CLIENT
#define SDB_CLIENT
#endif

#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "ossAtomic.hpp"
#include "ossUtil.hpp"


/** \namespace sdbclient
    \brief SequoiaDB Driver for C++
*/

namespace sdbclient
{
   class sdbConnectionPoolImpl : public SDBObject
   {
      friend void createConnFunc( void *args ) ;
      friend void destroyConnFunc( void *args ) ;
      friend void bgTaskFunc( void *args ) ;

   public:
      sdbConnectionPoolImpl()
         : _idleList(),
         _idleSize(0),
         _busyList(),
         _busySize(0),
         _destroyList(),
         _conf(),
         _strategy(NULL),
         _connMutex(),
         _confShare(),
         _globalMutex(),
         _isInited(FALSE),
         _isClosed(FALSE),
         _isEnabled(FALSE),
         _toCreateConn(FALSE),
         _toDestroyConn(FALSE),
         _toStopWorkers(FALSE),
         _createConnWorker(NULL),
         _destroyConnWorker(NULL),
         _bgTaskWorker(NULL) {}

      ~sdbConnectionPoolImpl() ;

   private:
      sdbConnectionPoolImpl( const sdbConnectionPool &connPool ) ;
      sdbConnectionPoolImpl& operator=( const sdbConnectionPool &connPool ) ;

   public:
      INT32 init( const std::string &address, const sdbConnectionPoolConf &conf ) ;

      INT32 init(
         const std::vector<std::string> &addrs,
         const sdbConnectionPoolConf &conf ) ;

      INT32 getIdleConnNum()const  ;

      INT32 getUsedConnNum()const  ;

      INT32 getNormalAddrNum()const  ;

      INT32 getAbnormalAddrNum() const  ;

      INT32 getLocalAddrNum()const  ;

      INT32 getConnection( sdb*& conn, INT64 timeoutms = 5000 ) ;

      void releaseConnection( sdb*& conn ) ;

      INT32 close() ;

      void updateAuthInfo( const string &username, const string &passwd ) ;

      void updateAuthInfo( const string &username, const string &cipherFile,
                           const string &token ) ;

      INT32 updateAddress( const std::vector<std::string> &addrs ) ;

//#if defined (_DEBUG)
//   private:
//      void printCreateInfo(const std::string& coord) ;
//#endif


   private:
      // check address arguments, if valid, add it
      BOOLEAN _checkAddrArg( const string &address ) ;

      // new a strategy with config
      INT32 _buildStrategy() ;

      // enable connection pool
      INT32 _enable() ;

      // disable connection pool
      INT32 _disable() ;

      // clear connection pool
      void _clearConnPool() ;

      // try to get a connection
      BOOLEAN _tryGetConn( sdb*& conn ) ;

      // create connection by a number
      INT32 _createConnByNum( INT32 num, INT32 &crtNum ) ;

      // sync coord node info
      void _syncCoordNodes() ;

      // get back the coord address from the abnormal address list
      INT32 _retrieveAddrFromAbnormalList() ;

      // check keep alive time out or not
      BOOLEAN _keepAliveTimeOut( sdb *conn ) ;

      // check max connection number intervally
      void _checkMaxIdleConn() ;

      // add new connection and make sure not reach max connection count
      BOOLEAN _addNewConnSafely( sdb *conn, const std::string &coord );

   private:
      // create connections function
      void _createConn() ;

      //destroy connections function
      void _destroyConn() ;

      // connect to db
      INT32 _connect( sdb *conn, const std::string &address ) ;

      // background task function
      void _bgTask() ;

   private:
      // idle connection list
      std::list<sdb*>         _idleList ;
      ossAtomic32             _idleSize ;
      // busy connection list
      std::list<sdb*>         _busyList ;
      ossAtomic32             _busySize ;
      // to be destroyed connection list
      std::list<sdb*>         _destroyList ;
      // connection pool confiture
      sdbConnectionPoolConf   _conf ;
      // connection pool strategy
      sdbConnPoolStrategy*    _strategy ;
      // lock for connection lists
      ossSpinXLatch           _connMutex ;
      // lock for _conf
      ossSpinSLatch           _confShare ;
      // lock for global commuincate
      ossSpinXLatch           _globalMutex ;
      // if has been inited
      BOOLEAN                 _isInited ;
      // if has been closed
      BOOLEAN                 _isClosed ;
      // if is enabled
      BOOLEAN                 _isEnabled ;

   private:
      BOOLEAN                 _toCreateConn ;
      BOOLEAN                 _toDestroyConn ;
      BOOLEAN                 _toStopWorkers ;
      sdbConnPoolWorker*      _createConnWorker ;
      sdbConnPoolWorker*      _destroyConnWorker ;
      sdbConnPoolWorker*      _bgTaskWorker ;
   } ;





   sdbConnectionPool::sdbConnectionPool(): _pImpl(NULL) {}

   sdbConnectionPool::~sdbConnectionPool()
   {
      SAFE_OSS_DELETE( _pImpl ) ;
   }

   INT32 sdbConnectionPool::init( const std::string &address,
                                  const sdbConnectionPoolConf &conf )
   {
      std::vector<std::string> addrs ;
      try
      {
         addrs.push_back( address ) ;
      }
      catch (std::exception &e)
      {
         return ossException2RC( &e ) ;
      }

      return init( addrs, conf ) ;
   }

   INT32 sdbConnectionPool::init(
         const std::vector<std::string> &addrs,
         const sdbConnectionPoolConf &conf )
   {
      if ( !_pImpl )
      {
         _pImpl = SDB_OSS_NEW sdbConnectionPoolImpl() ;
         if ( !_pImpl )
         {
            return SDB_OOM ;
         }
      }
      return _pImpl->init( addrs, conf ) ;
   }

   INT32 sdbConnectionPool::getIdleConnNum() const
   {
      if ( !_pImpl )
      {
         return -1 ;
      }
      return _pImpl->getIdleConnNum() ;
   }

   INT32 sdbConnectionPool::getUsedConnNum() const
   {
      if ( !_pImpl )
      {
         return -1 ;
      }
      return _pImpl->getUsedConnNum() ;
   }

   INT32 sdbConnectionPool::getNormalAddrNum() const
   {
      if ( !_pImpl )
      {
         return -1 ;
      }
      return _pImpl->getNormalAddrNum() ;
   }

   INT32 sdbConnectionPool::getAbnormalAddrNum() const
   {
      if ( !_pImpl )
      {
         return -1 ;
      }
      return _pImpl->getAbnormalAddrNum() ;
   }

   INT32 sdbConnectionPool::getLocalAddrNum() const
   {
      if ( !_pImpl )
      {
         return -1 ;
      }
      return _pImpl->getLocalAddrNum() ;
   }

   INT32 sdbConnectionPool::getConnection( sdb*& conn, INT64 timeoutms )
   {
      if ( !_pImpl )
      {
         return SDB_CLIENT_CONNPOOL_NOT_INIT ;
      }
      return _pImpl->getConnection( conn, timeoutms ) ;
   }

   void sdbConnectionPool::releaseConnection( sdb*& conn )
   {
      if ( !_pImpl )
      {
         return ;
      }
      return _pImpl->releaseConnection( conn ) ;
   }

   INT32 sdbConnectionPool::close()
   {
      if ( !_pImpl )
      {
         return SDB_CLIENT_CONNPOOL_NOT_INIT ;
      }
      return _pImpl->close() ;
   }

   void sdbConnectionPool::updateAuthInfo( const string &username,
                                           const string &passwd )
   {
      if ( !_pImpl )
      {
         return ;
      }
      return _pImpl->updateAuthInfo( username, passwd ) ;
   }

   void sdbConnectionPool::updateAuthInfo( const string &username,
                                           const string &cipherFile,
                                           const string &token )
   {
      if ( !_pImpl )
      {
         return ;
      }
      return _pImpl->updateAuthInfo( username, cipherFile, token ) ;
   }

   INT32 sdbConnectionPool::updateAddress( const std::vector<std::string> &addrs )
   {
      if ( !_pImpl )
      {
         return SDB_CLIENT_CONNPOOL_NOT_INIT ;
      }
      return _pImpl->updateAddress( addrs ) ;
   }

}

#endif /* SDB_CONNECTIONPOOL_IMPL_HPP_ */
