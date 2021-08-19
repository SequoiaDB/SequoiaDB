/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = omManagerJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/21/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMMANAGER_JOB_HPP_
#define OMMANAGER_JOB_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnBackgroundJob.hpp"
#include "ossLatch.hpp"
#include "omManager.hpp"
#include "omTaskManager.hpp"

namespace engine
{
   class omHostVersion : public SDBObject
   {
   public:
      omHostVersion() ;
      ~omHostVersion() ;

   public:
      void    incVersion( string clusterName ) ;
      void    setPrivilege( string clusterName, BOOLEAN privilege ) ;
      BOOLEAN getPrivilege( string clusterName ) ;
      void    removeVersion( string clusterName ) ;
      void    getVersionMap( map< string, UINT32 > &mapClusterVersion ) ;
      UINT32  getVersion( string clusterName ) ;

   private:
      ossSpinSLatch        _lock ;
      map<string, UINT32>  _mapClusterVersion ;
      map<string, BOOLEAN> _mapClusterPrivilege ;
      typedef map<string, UINT32 >::iterator _MAP_CV_ITER ;
      typedef map<string, UINT32 >::value_type _MAP_CV_VALUETYPE ;
   } ;

   struct omHostContent
   {
      string hostName ;
      string ip ;
      string serviceName ;
      string user ;
      string passwd ;
   } ;

   class omClusterNotifier : public SDBObject
   {
      public:
         omClusterNotifier( pmdEDUCB *cb, omManager *om, string clusterName ) ;
         ~omClusterNotifier() ;

      public:
         INT32            notify( UINT32 newVersion ) ;

      private:
         INT32            _updateNotifier() ;
         INT32            _notifyAgent() ;
         INT32            _addUpdateHostReq( pmdRemoteSession *remoteSession,
                                             const CHAR *localHostName ) ;
         void             _clearSession( pmdRemoteSession *remoteSession ) ;
         void             _getAgentService( string &serviceName ) ;

      private:
         pmdEDUCB                *_cb ;
         omManager               *_om ;
         string                  _clusterName ;
         UINT32                  _version ;

         // store all the host's info
         vector< omHostContent > _vHostTable ;
         // store the agent's info that need to update hostname
         map< string, omHostContent > _mapTargetAgents ;

         typedef map< string, omHostContent >::iterator _MAPAGENT_ITER ;
         typedef map< string, omHostContent >::value_type _MAPAGENT_VALUE ;
   } ;

   /*
      hostNotifier job
   */
   class omHostNotifierJob : public _rtnBaseJob
   {
      public:
         omHostNotifierJob( omManager *om, 
                            omHostVersion *version ) ;
         virtual ~omHostNotifierJob() ;

      public:
         virtual RTN_JOB_TYPE type() const ;
         virtual const CHAR*  name() const ;
         virtual BOOLEAN      muteXOn( const _rtnBaseJob *pOther ) ;
         virtual INT32        doit() ;

      private:
         void                 _checkUpdateCluster( 
                                      map<string, UINT32> &mapClusterVersion ) ;
         void                 _checkDeleteCluster(
                                      map<string, UINT32> &mapClusterVersion ) ;

      private:
         omManager            *_om ;
         omHostVersion        *_shareVersion ;

         map<string, omClusterNotifier*> _mapClusters ;
         typedef map<string, omClusterNotifier*>::iterator _MAP_CLUSTER_ITER ;
         typedef map<string, omClusterNotifier*>::value_type _MAP_CLUSTER_VALUE ;
   } ;
}

#endif  /* OMMANAGER_JOB_HPP_ */


