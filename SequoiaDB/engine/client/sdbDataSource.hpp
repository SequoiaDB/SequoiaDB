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

   Source File Name = sdbDataSource.hpp

   Descriptive Name = SDB Data Source Include Header

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016  LXJ  Initial Draft

   Last Changed =

*******************************************************************************/

/** \file sdbDataSource.hpp
    \brief C++ sdb data source
*/


#ifndef SDB_DATA_SOURCE_HPP_
#define SDB_DATA_SOURCE_HPP_

#include "sdbDataSourceComm.hpp"
#include <list>

#ifndef SDB_CLIENT
#define SDB_CLIENT
#endif

#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "ossAtomic.hpp"


/** \namespace sdbclient
    \brief SequoiaDB Driver for C++
*/

namespace sdbclient
{
   class sdbDataSourceStrategy ;
   class sdbDSWorker ;
   /** \class sdbDataSource
       \brief The sdb data source
   */
   class DLLEXPORT sdbDataSource : public SDBObject
   {
      friend void createConnFunc( void *args ) ;
      friend void destroyConnFunc( void *args ) ;
      friend void bgTaskFunc( void *args ) ;
      
   public:
      /** \fn sdbDataSource()
         \brief The constructor of sdbDataSource
      */
      sdbDataSource()
         : _idleList(),
         _idleSize(0),
         _busyList(),
         _busySize(0),
         _destroyList(),
         _conf(),
         _strategy(NULL),
         _connMutex(),
         _globalMutex(),
         _isInited(FALSE),
         _isEnabled(FALSE),
         _toCreateConn(FALSE),
         _toDestroyConn(FALSE),
         _toStopWorkers(FALSE),
         _createConnWorker(NULL),
         _destroyConnWorker(NULL),
         _bgTaskWorker(NULL) {}

      /** \fn ~sdbDataSource()
         \brief The destructor of sdbDataSource
      */
      ~sdbDataSource() ;

   private:
      sdbDataSource( const sdbDataSource &datasource ) ;
      sdbDataSource& operator=( const sdbDataSource &datasource ) ;

   public:
      /** \fn INT32 init(const std::string &url, 
         const sdbDataSourceConf &conf)
         \brief Initialize sdbDataSource
         \param [in] url A coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbDataSourceConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init( const std::string &url, const sdbDataSourceConf &conf ) ;

      /** \fn INT32 init(const std::vector<std::string> &vUrls,
         const sdbDataSourceConf &conf)
         \brief Initialize sdbDataSource
         \param [in] vUrls A list of coord node("ubuntu-xxx:11810")
         \param [in] conf The sdbDataSourceConf
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 init( 
         const std::vector<std::string> &vUrls, 
         const sdbDataSourceConf &conf ) ;

      /** \fn INT32 getIdleConnNum()const
         \brief Get idle connection number
         \retval The number of idle connection
      */
      INT32 getIdleConnNum()const  ;

      /** \fn INT32 getUsedConnNum()const
         \brief Get used connection number
         \retval The number of used connection
      */
      INT32 getUsedConnNum()const  ;

      /** \fn INT32 getNormalCoordNum()const
         \brief Get the number of reachable coord nodes
         \retval The number of reachable coord nodes
      */
      INT32 getNormalCoordNum()const  ;

      /** \fn INT32 getAbnormalCoordNum()const
         \brief Get the number of unreachable coord nodes
         \retval The number of unreachable coord nodes
      */
      INT32 getAbnormalCoordNum() const  ;

      /** \fn INT32 getLocalCoordNum()const
         \brief Get the number of local coord nodes
         \retval The number of local coord nodes
      */
      INT32 getLocalCoordNum()const  ;

      /** \fn INT32 addCoord(const string &url)
         \brief Add a coord node
         \param [in] url A coord node("ubuntu-xxx:11810")
      */
      void addCoord( const string &url ) ;

      /** \fn INT32 removeCoord(const string &url)
         \brief Remove a coord node
         \param [in] url A coord node("ubuntu-xxx:11810")
      */
      void removeCoord( const string &url ) ;

      /** \fn INT32 enable()
         \brief Enable sdbDataSource
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 enable() ;

      /** \fn INT32 disable()
         \brief Disable sdbDataSource
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 disable() ;

      /** \fn INT32 getConnection(sdb*& conn, INT64 timeoutsec = 5000)
         \brief Get a connection form sdbDataSource
         \param [out] conn A connection
         \param [in] timeoutms The time to wait when connection number reach to 
         max connection number,default:5000ms. when timeoutms is set to 0,
         means waiting until a connection is available. when timeoutms is less
         than 0, set it to be 0.
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      INT32 getConnection( sdb*& conn, INT64 timeoutms = 5000 ) ;

      /** \fn INT32 releaseConnection(sdb *conn)
         \brief Give back a connection to sdbDataSource
         \param [in] conn A connection
         \retval SDB_OK Operation Success
         \retval Others Operation Fail
      */
      void releaseConnection(sdb *conn) ;

      /** \fn void close()
         \brief Close sdbDataSource
      */
      void close() ;

   


   private:
      BOOLEAN _checkAddrArg( const string &url ) ;

      INT32 _buildStrategy() ;

      void _clearDataSource() ;

      BOOLEAN _tryGetConn( sdb*& conn ) ;

      INT32 _createConnByNum( INT32 num ) ;

      void _syncCoordNodes() ;

      INT32 _retrieveAddrFromAbnormalList() ;

      BOOLEAN _keepAliveTimeOut( sdb *conn ) ;

      void _checkMaxIdleConn() ;

      BOOLEAN _addNewConnSafely( sdb *conn, const std::string &coord );

   private:
      void _createConn() ;

      void _destroyConn() ;

      void _bgTask() ;

   private:
      std::list<sdb*>         _idleList ;
      ossAtomic32             _idleSize ;
      std::list<sdb*>         _busyList ;
      ossAtomic32             _busySize ;
      std::list<sdb*>         _destroyList ;
      sdbDataSourceConf       _conf ;
      sdbDataSourceStrategy*  _strategy ;
      ossSpinXLatch           _connMutex ;
      ossSpinXLatch           _globalMutex ;
      BOOLEAN                 _isInited ;
      BOOLEAN                 _isEnabled ;

   private:
      BOOLEAN                 _toCreateConn ;
      BOOLEAN                 _toDestroyConn ;
      BOOLEAN                 _toStopWorkers ;
      sdbDSWorker*            _createConnWorker ;
      sdbDSWorker*            _destroyConnWorker ;
      sdbDSWorker*            _bgTaskWorker ;
   } ;
}

#endif
