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

   Source File Name = sdbConnectionPoolComm.hpp

   Descriptive Name = SDB Connection Pool Common Include Header

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

/** \file sdbConnectionPoolComm.hpp
    \brief C++ sdb connection pool configure
*/

#ifndef SDB_CONNECTIONPOOL_COMM_HPP_
#define SDB_CONNECTIONPOOL_COMM_HPP_

#include "ossTypes.h"
#include <string>
#include "client.hpp"

using std::string;
/** \namespace sdbclient
    \brief SequoiaDB Driver for C++
*/
namespace sdbclient
{
   /** to previous create threshold*/
   #define SDB_CONNPOOL_TOPRECREATE_THRESHOLD                2
   /** check unnormal coord interval, default:60 * 1000ms*/
   #define SDB_CONNPOOL_CHECKUNNORMALCOORD_INTERVAL          (60 * 1000)
   /** create connection retry time at a coord, default:3*/
   #define SDB_CONNPOOL_CREATECONN_RETRYTIME                 3

   enum SDB_CONN_STRATEGY
   {
      SDB_CONN_STY_SERIAL,             /**< serial strategy */
      SDB_CONN_STY_RANDOM,             /**< random strategy */
      SDB_CONN_STY_LOCAL,              /**< local strategy */
      SDB_CONN_STY_BALANCE             /**< balance strategy */
   } ;

   /** \class sdbConnectionPoolConf
       \brief The configure of sdb connection pool
   */
   class DLLEXPORT sdbConnectionPoolConf
   {
   public:
      /** \fn sdbConnectionPoolConf()
         brief The constructor of sdbConnectionPoolConf.
      */
      sdbConnectionPoolConf()
         :_userName(),
         _passwd(),
         _initConnCount(10),
         _deltaIncCount(10),
         _maxIdleCount(20),
         _maxCount(500),
         _checkInterval(60 * 1000),
         _keepAliveTimeout(0 * 1000),
         _syncCoordInterval(0 * 1000),
         _validateConnection(FALSE),
         _connectStrategy(SDB_CONN_STY_SERIAL),
         _useSSL(FALSE) {}

   private:
      // user info
      string               _userName ;
      string               _passwd ;
      string               _cipherFile ;
      string               _token ;
      // connection number info
      INT32                _initConnCount ;
      INT32                _deltaIncCount ;
      INT32                _maxIdleCount ;
      INT32                _maxCount ;
      // check idle connection interval
      INT32                _checkInterval ;
      INT32                _keepAliveTimeout ;
      // sync coord interval
      INT32                _syncCoordInterval ;
      // whether check validation when a connection out
      BOOLEAN              _validateConnection ;
      // strategy to create connections
      SDB_CONN_STRATEGY    _connectStrategy ;

      // if configure is valid
      BOOLEAN              _useSSL ;

   public:
      /** \fn void setAuthInfo( const string& username,
           const string& passwd)
         \brief Set authentication information
         \param [in] username The user name
         \param [in] passwd The password
      */
      void setAuthInfo(
         const string &username,
         const string &passwd ) ;

      /** \fn void setAuthInfo( const string& username,
                                const string& cipherFile,
                                const string& token )
         \brief Set authentication information, if a password has been set,
         the password is preferred over the cipherfile
         \param [in] username The user name
         \param [in] cipherFile The cipherfile location
         \param [in] token The password encryption token
      */
      void setAuthInfo( const string &username,
                        const string &cipherFile,
                        const string &token ) ;

      /** \fn string getUserName()
         \brief Get user name
         \retval string user name
      */
      string getUserName() const { return _userName ; }

      /** \fn string getPasswd()
         \brief Get password
         \retval string password
      */
      string getPasswd() const { return _passwd ; }

      /** \fn string getCipherFile()
         \brief Get the cipherfile location
         \retval string cipherFile
      */
      string getCipherFile() const { return _cipherFile ; }

      /** \fn string getToken()
         \brief Get the password encryption token
         \retval string encryption token
      */
      string getToken() const { return _token ; }

      /** \fn void setConnCntInfo(INT32 initCnt,
                                  INT32 deltaIncCnt,
                                  INT32 maxIdleCnt,
                                  INT32 maxCnt)
         \brief Set connection number parameters
         \param [in] initCnt The initial connection number
         \param [in] deltaIncCnt The increment of connection each time
         \param [in] maxIdleCnt The max idle connection number
         \param [in] maxCnt The max connection number
      */

      void setConnCntInfo(
         INT32 initCnt,
         INT32 deltaIncCnt,
         INT32 maxIdleCnt,
         INT32 maxCnt ) ;

      /** \fn INT32 getInitConnCount()
         \brief Get the initial connection number
         \retval INT32 The initial connection number
      */
      INT32 getInitConnCount() const { return _initConnCount ; }

      /** \fn INT32 getDeltaIncCount()
         \brief Get the increment of connection each time
         \retval UINT32 The increment of connection each time
      */
      INT32 getDeltaIncCount() const { return _deltaIncCount ; }

      /** \fn INT32 getMaxIdleCount()
         \brief Get the max idle connection number
         \retval INT32 The max idle connection number
      */
      INT32 getMaxIdleCount() const { return _maxIdleCount ; }

      /** \fn INT32 getMaxCount()
         \brief Get the max connection number
         \retval INT32 The max connection number
      */
      INT32 getMaxCount() const { return _maxCount ; }

      /** \fn void setCheckIntervalInfo(INT32 interval, INT32 aliveTime = 0)
         \brief Set the interval time of check idle connection. And set the
         time in millisecond for abandoning a connection which keep alive
         time is up.
         \param [in] interval The interval time in millisecond of check idle
         connection
         \param [in] aliveTime If a connection has not be
         used(send and receive) for a long time(longer than "aliveTime"),
         the pool will not let it come back. The pool will also clean
         this kind of idle connections in the pool periodically. This value
         default to be 0ms. means not care about how long
         does a connection have not be used(send and receive).
         \note When "aliveTime" is not set to 0, it's better to set it
         greater than "interval" triple over. Besides,
         unless you know what you need, never enable this option.
      */
      void setCheckIntervalInfo( INT32 interval, INT32 aliveTime = 0 ) ;

      /** \fn INT32 getCheckInterval()
         \brief Get the interval time in millisecond of check idle connection
         \retval INT32 the interval time in millisecond of check idle connection
      */
      INT32 getCheckInterval() const { return _checkInterval ; }

      /** \fn INT32 getKeepAliveTimeout()
         \brief Get the keep alive time
         \retval INT32 the keep alive time in millisecond
      */
      INT32 getKeepAliveTimeout() const { return _keepAliveTimeout ; }

      /** \fn void setSyncCoordInterval (INT64 interval)
         \brief Set the interval time in seconds of synchronize coord node
         \param [in] interval The interval time in millisecond of synchronize coord
         node
      */
      void setSyncCoordInterval ( INT32 interval )
      {
         _syncCoordInterval = interval ;
      }
      /** \fn INT32 getSyncCoordInterval() const
         \brief Get the interval time in seconds of synchronize coord node
         \retval INT32 the interval time in seconds of synchronize coord node
      */
      INT32 getSyncCoordInterval() const { return _syncCoordInterval ; }

      /** \fn void setValidateConnection(BOOLEAN bCheck)
         \brief Set whether to check the validation of a connection when it's
         given out
         \param [in] bCheck If TURE check the validation, else not check the
         validation
      */
      void setValidateConnection( BOOLEAN bCheck )
      {
         _validateConnection = bCheck ;
      }

      /** \fn BOOLEAN getValidateConnection() const
         \brief Get whether to check the validation of a connection when it's
         given out
         \retval BOOLEAN if TRUE check the validation, else not check the
         validation
      */
      BOOLEAN getValidateConnection() const { return _validateConnection ; }

      /** \fn void setConnectStrategy(SDB_CONN_STRATEGY strategy)
         \brief Set the strategy of sdbConnectionPool
         \param [in] strategy The enum of strategy:
         SDB_CONN_STY_SERIAL, SDB_CONN_STY_RANDOM, SDB_CONN_STY_LOCAL, SDB_CONN_STY_BALANCE
      */
      void setConnectStrategy( SDB_CONN_STRATEGY strategy )
      {
         _connectStrategy = strategy ;
      }
      /** \fn SDB_CONN_STRATEGY getConnectStrategy()
         \brief Get the strategy of sdbConnectionPool
         \retval SDB_CONN_STRATEGY The strategy of sdbConnectionPool
      */
      SDB_CONN_STRATEGY getConnectStrategy() const
      {
         return _connectStrategy ;
      }

      /** \fn void setUseSSL( BOOLEAN useSSL )
         \brief Set whether use the SSL or not
         \param [in] useSSL If true, use SSL, else, not use SSL
      */
      void setUseSSL( BOOLEAN useSSL )
      {
         _useSSL = useSSL ;
      }
      /** \fn BOOLEAN getUseSSL()
         \brief Get whether use SSL or not
         \retval BOOLEAN Return use SSL or not
      */
      BOOLEAN getUseSSL() const { return _useSSL ; }


      /** \fn BOOLEAN isValid()
         \brief Check whether sdbConnectionPoolConf is valid
         \retval BOOLEAN The validation of sdbConnectionPoolConf
      */
      BOOLEAN isValid() ;

   } ;
}
#endif /* SDB_CONNECTIONPOOL_COMM_HPP_ */
