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

   Source File Name = sdbConnectionPool.hpp

   Descriptive Name = SDB Connection Pool Include Header

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016  LXJ  Initial Draft

   Last Changed =

*******************************************************************************/

/** \file sdbConnectionPool.hpp
    \brief C++ sdb connection pool
*/


#ifndef SDB_CONNECTIONPOOL_HPP_
#define SDB_CONNECTIONPOOL_HPP_

#include "sdbConnectionPoolComm.hpp"

#ifndef SDB_CLIENT
#define SDB_CLIENT
#endif

/** \namespace sdbclient
    \brief SequoiaDB Driver for C++
*/

namespace sdbclient
{
   class sdbConnectionPoolImpl ;

   /** \class sdbConnectionPool
       \brief The sdb connection pool
   */
   class DLLEXPORT sdbConnectionPool
   {
   public:
      /** \fn sdbConnectionPool()
         \brief The constructor of sdbConnectionPool
      */
      sdbConnectionPool() ;

      /** \fn ~sdbConnectionPool()
         \brief The destructor of sdbConnectionPool
      */
      ~sdbConnectionPool() ;

   private:
      sdbConnectionPool( const sdbConnectionPool &connPool ) ;
      sdbConnectionPool& operator=( const sdbConnectionPool &connPool ) ;

   public:
      /** \fn INT32 init(const std::string &address,
         const sdbConnectionPoolConf &conf)
         \brief Initialize connection pool
         \param [in] address A coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbConnectionPoolConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init( const std::string &address, const sdbConnectionPoolConf &conf ) ;

      /** \fn INT32 init(const std::vector<std::string> &addrs,
         const sdbConnectionPoolConf &conf)
         \brief Initialize connection pool
         \param [in] addrs A list of coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbConnectionPoolConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init(
         const std::vector<std::string> &addrs,
         const sdbConnectionPoolConf &conf ) ;

      /** \fn INT32 getIdleConnNum()const
         \brief Get idle connection number or -1 for connection pool
                has not been initialized yet.
         \retval The number of idle connection
      */
      INT32 getIdleConnNum() const  ;

      /** \fn INT32 getUsedConnNum()const
         \brief Get used connection number or -1 for connection pool
                has not been initialized yet.
         \retval The number of used connection
      */
      INT32 getUsedConnNum() const  ;

      /** \fn INT32 getNormalAddrNum()const
         \brief Get the number of reachable address or -1 for connection pool
                has not been initialized yet.
         \retval The number of reachable coord nodes
      */
      INT32 getNormalAddrNum() const  ;

      /** \fn INT32 getAbnormalAddrNum()const
         \brief Get the number of unreachable address or -1 for connection pool
                has not been initialized yet.
         \retval The number of unreachable coord nodes
      */
      INT32 getAbnormalAddrNum() const  ;

      /** \fn INT32 getLocalAddrNum()const
         \brief Get the number of local address or -1 for connection pool
                has not been initialized yet.
         \retval The number of local coord nodes
      */
      INT32 getLocalAddrNum() const  ;

      /** \fn INT32 getConnection(sdb*& conn, INT64 timeoutsec = 5000)
         \brief Get a connection form connection pool
         \param [out] conn A connection
         \param [in] timeoutms The time to wait when connection number reach to
         max connection number,default:5000ms. when timeoutms is set to 0,
         means waiting until a connection is available. when timeoutms is less
         than 0, set it to be 0.
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 getConnection( sdb*& conn, INT64 timeoutms = 5000 ) ;

      /** \fn INT32 releaseConnection( sdb*& conn )
         \brief Give back a connection to connection pool. If the connection pool had
                been closed, do nothing.
         \param [in] conn A connection
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      void releaseConnection( sdb*& conn ) ;

      /** \fn void close()
         \brief Close connection pool
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 close() ;

      /** \fn void updateAuthInfo( const string &username, const string &passwd )
         \brief update authentication information
         \param [in] username The new user name
         \param [in] passwd The new password
      */
      void updateAuthInfo( const string &username, const string &passwd ) ;

      /** \fn void updateAuthInfo( const string &username,
                                   const string &cipherFile,
                                   const string &token )
         \brief update authentication information
         \param [in] username The new user name
         \param [in] cipherFile The new cipherfile location
         \param [in] token The new password encryption token
      */
      void updateAuthInfo( const string &username, const string &cipherFile,
                           const string &token ) ;

      /** \fn INT32 updateAddress( const std::vector<std::string> &addrs )
         \brief update the addresses of the connection pool, the new address list will
                replace the old address list, and idle connections whose addresses are
                not in the new address list will be cleared.
         \param [in] coordAddrs A list of coord node
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 updateAddress( const std::vector<std::string> &addrs ) ;

   private:
      sdbConnectionPoolImpl *_pImpl ;
   } ;
}

#endif /* SDB_CONNECTIONPOOL_HPP_ */
