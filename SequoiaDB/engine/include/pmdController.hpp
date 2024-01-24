/*******************************************************************************
   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = pmdController.hpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/05/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_CONTROLLER_HPP__
#define PMD_CONTROLLER_HPP__

#include "pmdEnv.hpp"
#include "ossSocket.hpp"
#include "sdbInterface.hpp"
#include "pmdRestSession.hpp"
#include "restAdaptor.hpp"
#include "pmdRemoteSession.hpp"
#include "pmdAccessProtocolBase.hpp"

#include <string>
#include <map>

using namespace std ;

namespace engine
{
   class _pmdModuleLoader ;
   /*
      _pmdController define
   */
   class _pmdController : public _IControlBlock
   {
      public:
         _pmdController () ;
         virtual ~_pmdController () ;

         virtual SDB_CB_TYPE cbType() const ;
         virtual const CHAR* cbName() const ;

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;

         void           onTimer( UINT32 interval ) ;

         void           setFixBuffSize( INT32 buffSize ) { _fixBufSize = buffSize ; }
         void           setMaxRestBodySize( INT32 bodySize ) { _maxRestBodySize = bodySize ; }
         void           setRestTimeout( INT32 timeout ) { _restTimeout = timeout ; }
         void           setRSManager( pmdRemoteSessionMgr *pRSManager ) ;

         INT32          registerNet( _netFrame *pNetFrame,
                                     INT32 serviceType = -1,
                                     BOOLEAN isActive = TRUE ) ;
         void           unregNet( _netFrame *pNetFrame ) ;

         restAdaptor*   getRestAdptor() { return &_restAdptor ; }
         RestToMSGTransfer* getRestTransfer() { return &_restTransfer ; }
         pmdRemoteSessionMgr* getRSManager() { return _pRSManager ; }

      public:
         void              detachSessionInfo( restSessionInfo *pSessionInfo ) ;
         restSessionInfo*  attachSessionInfo( const string &id ) ;
         restSessionInfo*  newSessionInfo( const string &userName,
                                           UINT32 localIP ) ;
         void              releaseSessionInfo ( const string &sessionID ) ;

         void              releaseFixBuf( CHAR *pBuff ) ;
         CHAR*             allocFixBuf() ;
         INT32             getFixBufSize() const { return _fixBufSize ; }

      protected:
         string            _makeID( restSessionInfo *pSessionInfo ) ;
         void              _add2UserMap( const string &user,
                                         restSessionInfo *pSessionInfo ) ;
         void              _delFromUserMap( const string &user,
                                            restSessionInfo *pSessionInfo ) ;
         void              _invalidSessionInfo( restSessionInfo *pSessionInfo ) ;

         void              _checkSession( UINT32 interval ) ;

      public:

         virtual void  registerCB( SDB_ROLE dbrole ) ;

      private:
         INT32 initForeignModule( const CHAR *moduleName ) ;
         INT32 activeForeignModule() ;
         void  finishForeignModule() ;

      private:
         ossSocket               *_pTcpListener ;
         ossSocket               *_pHttpListener ;
         ossSocket               *_pMongoListener ;

      private:
         _pmdModuleLoader        *_fapMongo ;
         IPmdAccessProtocol      *_protocol ;

      private:
         map<string, restSessionInfo*>          _mapSessions ;
         map<string, vector<restSessionInfo*> > _mapUser2Sessions ;
         ossSpinSLatch                          _ctrlLatch ;
         UINT32                                 _sequence ;
         UINT32                                 _timeCounter ;

         vector< CHAR* >                        _vecFixBuf ;
         INT32                                  _fixBufSize ;

         INT32                                  _maxRestBodySize ;
         INT32                                  _restTimeout ;

         restAdaptor                            _restAdptor ;
         RestToMSGTransfer                      _restTransfer ;
         pmdRemoteSessionMgr                    *_pRSManager ;

         map< _netFrame*, netFrameMon >         _mapMonNets ;

   } ;
   typedef _pmdController pmdController ;

   /*
      get global pointer
   */
   pmdController* sdbGetPMDController() ;

}

#endif //PMD_CONTROLLER_HPP__

