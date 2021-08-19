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

   Source File Name = omConfigModel.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigModel.hpp"
#include "omDef.hpp"
#include "omConfigSdb.hpp"
#include "omConfigZoo.hpp"
#include "omConfigSsqlOlap.hpp"
#include "omConfigPostgreSQL.hpp"
#include "omConfigMySQL.hpp"
#include "pd.hpp"
#include <sstream>

namespace engine
{
   OmNode::OmNode()
   {
   }

   OmNode::~OmNode()
   {
   }

   INT32 OmNode::createObject( const string& businessType,
                               const string& deployMode,
                               OmNode*& node )
   {
      INT32 rc = SDB_OK ;
      OmNode* _node = NULL ;

      if ( businessType == OM_BUSINESS_SEQUOIADB )
      {
         _node = SDB_OSS_NEW OmSdbNode() ;
      }
      else if ( businessType == OM_BUSINESS_ZOOKEEPER )
      {
         _node = SDB_OSS_NEW OmZooNode() ;
      }
      else if ( businessType == OM_BUSINESS_SEQUOIASQL_OLAP )
      {
         _node = SDB_OSS_NEW OmSsqlOlapNode() ;
      }
      else if ( businessType == OM_BUSINESS_SEQUOIASQL_POSTGRESQL )
      {
         _node = SDB_OSS_NEW OmPostgreSQLNode() ;
      }
      else if ( businessType == OM_BUSINESS_SEQUOIASQL_MYSQL )
      {
         _node = SDB_OSS_NEW OmMySQLNode() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid business type: %s", businessType.c_str() ) ;
         goto error ;
      }

      if ( NULL == _node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create node because of out of memory, business type is %s",
                 businessType.c_str() ) ;
         goto error ;
      }

      node = _node ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _node ) ;
      goto done ;
   }

   INT32 OmNode::init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster )
   {
      INT32 rc = SDB_OK ;
      OmBusiness* business = NULL ;

      _setHostName( host.getHostName() ) ;

      string businessName = bsonNode.getStringField( OM_BSON_BUSINESS_NAME ) ;
      rc = cluster.getBusiness( businessName, business ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get business of node, business name is %s",
                 businessName.c_str() );
         goto error ;
      }

      _setBusinessInfo( business->getBusinessInfo() ) ;

      rc = _init( bsonNode, host, cluster ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to _init, business name is %s, host name is %s",
                 businessName.c_str(), _hostName.c_str() );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   bool OmNode::isDiskUsed( const string& diskName ) const
   {
      return _disks.end() != _disks.find( diskName ) ;
   }

   bool OmNode::isPortUsed( const string& port ) const
   {
      return _ports.end() != _ports.find( port ) ;
   }

   INT32 OmNode::_addPath( const string& path, OmHost& host, bool ignoreDisk )
   {
      INT32 rc = SDB_OK ;

      const simpleDiskInfo* disk = host.getDisk( path ) ;
      if ( NULL == disk )
      {
         if ( !ignoreDisk )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "failed to get disk of path[%s]", path.c_str() ) ;
            goto error ;
         }
      }
      else
      {
         string diskName = disk->diskName ;
         SDB_ASSERT( diskName != "", "empty disk name" ) ;

         rc = _addUsedDisk( diskName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add used disk to node" );
            goto error ;
         }
      }

      rc = _addUsedPath( path ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used path to node" );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmNode::_addUsedDisk( const string& diskName )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _disks.insert( diskName ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert disk to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmNode::_addUsedPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _ports.insert( port ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert port to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmNode::_addUsedPath( const string& path )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _pathes.insert( path ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert path to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmNode::_setHostName( const string& hostName )
   {
      _hostName = hostName ;
   }

   void OmNode::_setBusinessInfo( const OmBusinessInfo& businessInfo )
   {
      _businessInfo = businessInfo ;
   }

   OmNodes::OmNodes( bool manageNode ):
      _manageNode( manageNode )
   {
   }

   OmNodes::~OmNodes()
   {
      if ( _manageNode )
      {
         for ( OmNodesIterator it = _nodes.begin() ; it != _nodes.end() ; )
         {
            OmNode* node = *it ;
            SDB_OSS_DEL node ;
            _nodes.erase( it++ ) ;
         }
      }
   }

   OmNodes::ConstIterator OmNodes::begin() const
   {
      return _nodes.begin() ;
   }

   OmNodes::ConstIterator OmNodes::end() const
   {
      return _nodes.end() ;
   }

   INT32 OmNodes::nodeNum() const
   {
      return _nodes.size() ;
   }

   bool OmNodes::_hasNode( OmNode* node ) const
   {
      OmNodesConstIterator it ;

      SDB_ASSERT( NULL != node, "node can't be null" ) ;

      for ( it = _nodes.begin() ; it != _nodes.end() ; it++ )
      {
         if ( node == *it )
         {
            return true ;
         }
      }

      return false ;
   }

   INT32 OmNodes::_addNode( OmNode* node )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != node, "node can't be null" ) ;
      SDB_ASSERT( !_hasNode( node ), "node already exists" ) ;

      try
      {
         _nodes.push_back( node ) ;
      }
      catch (exception &e)
      {
         PD_LOG(PDERROR, "unexpected error happened when insert node to list:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmNodes::_delNode( OmNode* node )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != node, "node can't be null" ) ;
      SDB_ASSERT( _hasNode( node ), "node doesn't exist" ) ;

      try
      {
         _nodes.remove( node ) ;
      }
      catch (exception &e)
      {
         PD_LOG( PDERROR, "unexpected error happened when insert node to list:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   inline bool _isNumber( char c )
   {
      return c >= '0' && c <= '9';
   }

   // for convenience, '{' is greater than anything and stops number parsing
   int _cmpByMountPath::_lexNumCmp( const char *s1, const char *s2,
                                    bool pointend )
   {
      bool p1, p2, n1, n2 ;
      const char *e1 ;
      const char *e2 ;
      int len1, len2, result ;

      //cout << "START : " << s1 << "\t" << s2 << endl;
      while( ( *s1 && ( !pointend || '.' != *s1 ) ) &&
      ( *s2 && ( !pointend || '.' != *s2 ) ) )
      {
         p1 = ( *s1 == (char)255 );
         p2 = ( *s2 == (char)255 );
         //cout << "\t\t " << p1 << "\t" << p2 << endl;
         if ( p1 && !p2 )
            return 1 ;
         if ( p2 && !p1 )
            return -1 ;

         n1 = _isNumber( *s1 );
         n2 = _isNumber( *s2 );

         if ( n1 && n2 )
         {
            int zerolen1 = 0 ;
            int zerolen2 = 0 ;
            // get rid of leading 0s
            while ( *s1 == '0' )
            {
               s1++ ;
               ++zerolen1 ;
            }
            while ( *s2 == '0' )
            {
               s2++ ;
               ++zerolen2 ;
            }

            e1 = s1 ;
            e2 = s2 ;

            // find length
            // if end of string, will break immediately ('\0')
            while ( _isNumber (*e1) ) e1++;
            while ( _isNumber (*e2) ) e2++;

            len1 = (int)(e1-s1);
            len2 = (int)(e2-s2);

            // if one is longer than the other, return
            if ( len1 > len2 ) {
               return 1;
            }
            else if ( len2 > len1 ) {
               return -1;
            }
            // if the lengths are equal, just strcmp
            else if ( (result = strncmp(s1, s2, len1)) != 0 ) {
               return result;
            }
            // compare the zero len
            else if ( zerolen1 != zerolen2 ) {
               return zerolen1 < zerolen2 ? 1 : -1 ;
            }

            // otherwise, the numbers are equal
            s1 = e1;
            s2 = e2;
            continue;
         }

         if ( n1 )
            return 1;

         if ( n2 )
            return -1;

         if ( *s1 > *s2 )
            return 1;

         if ( *s2 > *s1 )
            return -1;

         s1++; s2++;
      }

      if ( *s1 && ( !pointend || '.' != *s1 ) )
         return 1;
      if ( *s2 && ( !pointend || '.' != *s2 ) )
         return -1;
      return 0;
   }

   OmHost::OmHost( const string& hostName ):
      _hostName( hostName )
   {
   }

   OmHost::~OmHost()
   {
   }

   INT32 OmHost::nodeNum() const
   {
      return _nodes.nodeNum() ;
   }

   INT32 OmHost::diskNum() const
   {
      return _disks.size() ;
   }

   INT32 OmHost::unusedDiskNum() const
   {
      return _disks.size() - _usedDisks.size() ;
   }

   bool OmHost::isPortUsed( const string& port ) const
   {
      return _usedPorts.end() != _usedPorts.find( port ) ;
   }

   bool OmHost::isPortUsed( INT32 port ) const
   {
      stringstream ss ;
      ss << port ;
      return isPortUsed( ss.str() ) ;
   }

   bool OmHost::isPathUsed( const string& path ) const
   {
      return _usedPathes.end() != _usedPathes.find( path ) ;
   }

   INT32 OmHost::_addNode( OmNode* node )
   {
      SDB_ASSERT( NULL != node, "node can't be NULL") ;
      SDB_ASSERT( _hostName == node->getHostName(), "different host name") ;
      return _nodes._addNode( node ) ;
   }

   INT32 OmHost::_addUsedPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      if ( isPortUsed( port ) )
      {
         PD_LOG_MSG(PDERROR, "port[%s] already exists", port.c_str() );
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         _usedPorts.insert( port ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert used port to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmHost::_addUsedDisk( const string& diskName )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _usedDisks.insert( diskName ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert used disk to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmHost::_addUsedPath( const string& path )
   {
      INT32 rc = SDB_OK ;

      if ( isPathUsed( path ) )
      {
         PD_LOG_MSG(PDERROR, "path[%s] already exists", path.c_str() );
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         _usedPathes.insert( path ) ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when insert used path to set:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmHost::addNode( OmNode* node )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != node, "node can't be NULL") ;

      rc = _addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to add node to host[%s]",
                 node->getHostName().c_str() );
         goto error ;
      }

      {
         const set<string>& usedDisks = node->getDisks() ;
         for ( set<string>::const_iterator it = usedDisks.begin() ;
               it != usedDisks.end() ; it++ )
         {
            rc = _addUsedDisk( *it ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add used disk" );
               goto error ;
            }
         }
      }

      {
         const set<string>& usedPathes = node->getPathes() ;
         for ( set<string>::const_iterator it = usedPathes.begin() ;
               it != usedPathes.end() ; it++ )
         {
            rc = _addUsedPath( *it ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add used path" );
               goto error ;
            }
         }
      }

      {
         const set<string>& usedPorts = node->getPorts() ;
         for ( set<string>::const_iterator it = usedPorts.begin() ;
               it != usedPorts.end() ; it++ )
         {
            rc = _addUsedPort( *it ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add used port" );
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmHost::addDisk( const simpleDiskInfo& disk )
   {
      INT32 rc = SDB_OK ;

      if ( _disks.end() != _disks.find( disk.diskName ) )
      {
         PD_LOG_MSG(PDERROR, "disk already exists: %s", disk.diskName.c_str() );
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         _disks[ disk.diskName ] = disk ;
      }
      catch (exception& e)
      {
         PD_LOG_MSG( PDERROR, "unexpected error happened when add disk: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmHost::appendDeployPath( const string &packageName,
                                  string &deployPath )
   {
      _deployPathes.insert( make_pair( packageName, deployPath ) ) ;
   }

   string OmHost::getDeployPath( const string &packageName )
   {
      map<const string, string>::iterator it ;
      string deployPath ;

      it = _deployPathes.find( packageName ) ;
      if ( it != _deployPathes.end() )
      {
         deployPath = it->second ;
      }

      return deployPath ;
   }

   const simpleDiskInfo* OmHost::getDisk( const string path )
   {
      INT32 maxFitSize = 0 ;
      const simpleDiskInfo* disk = NULL ;

      for ( OmDiskConstIterator iter = _disks.begin() ; iter != _disks.end(); iter++ )
      {
         const simpleDiskInfo* tmpDisk = &iter->second ;
         string::size_type pathPos = path.find( tmpDisk->mountPath ) ;
         if ( pathPos != string::npos )
         {
            INT32 tmpFitSize = tmpDisk->mountPath.length() ;
            if ( NULL == disk )
            {
               disk  = tmpDisk ;
               maxFitSize = tmpFitSize ;
            }
            else
            {
               if ( maxFitSize < tmpFitSize )
               {
                  disk  = tmpDisk ;
                  maxFitSize = tmpFitSize ;
               }
            }
         }
      }

      return disk ;
   }

   OmBusiness::OmBusiness( const OmBusinessInfo& businessInfo ):
      _businessInfo( businessInfo )
   {
   }

   OmBusiness::~OmBusiness()
   {
   }

   INT32 OmBusiness::nodeNum() const
   {
      return _nodes.nodeNum() ;
   }

   INT32 OmBusiness::addNode( OmNode* node )
   {
      SDB_ASSERT( NULL != node, "node can't be NULL") ;
      SDB_ASSERT( _businessInfo == node->getBusinessInfo(), "different business") ;
      return _nodes._addNode( node ) ;
   }

   OmCluster::OmCluster():
      _nodes( OmNodes( true ) )
   {
   }

   OmCluster::~OmCluster()
   {
      for ( OmHostsIterator it = _hosts.begin() ; it != _hosts.end() ; )
      {
         OmHost* host = it->second ;
         SDB_OSS_DEL host ;
         _hosts.erase( it++ ) ;
      }

      for ( OmBusinessesIterator it = _businesses.begin() ; it != _businesses.end(); )
      {
         OmBusiness* business = it->second ;
         SDB_OSS_DEL business ;
         _businesses.erase( it++ ) ;
      }
   }

   INT32 OmCluster::init( const BSONObj& bsonBusiness, const BSONObj& bsonHost )
   {
      INT32 rc = SDB_OK ;

      rc = _initBusiness( bsonBusiness ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init business: rc=%d", rc ) ;
         goto error ;
      }

      rc = _initHostAndNode( bsonHost ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init host: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   bsonBusiness:
   {
      "BusinessInfo": [
         {
            "BusinessName": "myModule",
            "BusinessType": "sequoiadb",
            "DeployMod": "distribution",
            "ClusterName": "myCluster",
            ...
         }
         ,...
      ]
   }
   */
   INT32 OmCluster::_initBusiness( const BSONObj& bsonBusiness )
   {
      INT32 rc = SDB_OK ;

      BSONElement bsonBusinessInfo = bsonBusiness.getField( OM_BSON_BUSINESS_INFO ) ;
      if ( bsonBusinessInfo.eoo() || Array != bsonBusinessInfo.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "BusinessInfo is not Array: field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_INFO, bsonBusinessInfo.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator iter( bsonBusinessInfo.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( Object == ele.type() )
            {
               OmBusinessInfo businessInfo ;
               OmBusiness* business = NULL ;

               BSONObj businessInfoObj = ele.embeddedObject() ;
               businessInfo.clusterName = 
                  businessInfoObj.getStringField( OM_BSON_CLUSTER_NAME ) ;
               businessInfo.businessType = 
                  businessInfoObj.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
               businessInfo.businessName = 
                  businessInfoObj.getStringField( OM_BUSINESS_FIELD_NAME ) ;
               businessInfo.deployMode = 
                  businessInfoObj.getStringField( OM_BSON_DEPLOY_MOD ) ;

               business = SDB_OSS_NEW OmBusiness( businessInfo ) ;
               if ( NULL == business )
               {
                  rc = SDB_OOM ;
                  PD_LOG_MSG( PDERROR, "failed to alloc new OmBusiness: %s, out of memory",
                          businessInfo.businessName.c_str() ) ;
                  goto error ;
               }

               rc = addBusiness( business ) ;
               if ( SDB_OK != rc )
               {
                  SAFE_OSS_DELETE( business ) ;
                  PD_LOG( PDERROR, "failed to add business: %s, rc=%d",
                          businessInfo.businessName.c_str(), rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   bsonHost:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","dbpath":"", svcname:"", 
                                 "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 OmCluster::_initHostAndNode( const BSONObj& bsonHost )
   {
      INT32 rc = SDB_OK ;

      BSONElement hostInfo = bsonHost.getField( OM_BSON_FIELD_HOST_INFO ) ;
      if ( hostInfo.eoo() || Array != hostInfo.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "HostInfo is not Array: field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_INFO, hostInfo.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator iter( hostInfo.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( Object == ele.type() )
            {
               BSONObj hostNodeInfo = ele.embeddedObject() ;
               rc = _addHostAndNode( hostNodeInfo ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "add host failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::nodeNum() const
   {
      return _nodes.nodeNum() ;
   }

   INT32 OmCluster::hostNum() const
   {
      return _hosts.size() ;
   }

   INT32 OmCluster::getBusiness( const string& businessName, OmBusiness*& business )
   {
      INT32 rc = SDB_OK ;
      OmBusinessesIterator it ;

      it = _businesses.find( businessName ) ;
      if ( _businesses.end() == it )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      business = it->second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::getHost( const string& hostName, OmHost*& host )
   {
      INT32 rc = SDB_OK ;
      OmHostsConstIterator it ;

      it = _hosts.find( hostName ) ;
      if ( _hosts.end() == it )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      host = it->second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::_addHostAndNode( const BSONObj& hostNodeInfo )
   {
      INT32 rc = SDB_OK ;
      OmHost* host = NULL ;

      string hostName = hostNodeInfo.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      if ( hostName == "" )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid host name: %s",
                     hostNodeInfo.toString().c_str() ) ;
         goto error ;
      }

      host = SDB_OSS_NEW OmHost( hostName ) ;
      if ( NULL == host )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to alloc new OmHost: %s, out of memory",
                 hostName.c_str() ) ;
         goto error ;
      }

      {
         BSONObj packages = hostNodeInfo.getObjectField(
                                                      OM_HOST_FIELD_PACKAGES ) ;

         {
            BSONObjIterator iter( packages ) ;

            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               BSONObj packageInfo = ele.embeddedObject() ;
               string packageName = packageInfo.getStringField(
                                                   OM_HOST_FIELD_PACKAGENAME ) ;
               string installPath = packageInfo.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;

               host->appendDeployPath( packageName, installPath ) ;
            }
         }
      }

      {
         BSONObj disks = hostNodeInfo.getObjectField( OM_BSON_FIELD_DISK ) ;
         BSONObjIterator i( disks ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj bsonDisk = ele.embeddedObject() ;
               rc = _buildDisk( bsonDisk, *host ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build disk of host [%s], rc=%d",
                          hostName.c_str(), rc ) ;
                  goto error ;
               }
            }
         }
      }

      {
         BSONObj nodes = hostNodeInfo.getObjectField( OM_BSON_FIELD_CONFIG ) ;
         BSONObjIterator i( nodes ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj bsonNode = ele.embeddedObject() ;
               rc = _buildNode( bsonNode, *host ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build node of host [%s], rc=%d",
                          hostName.c_str(), rc ) ;
                  goto error ;
               }
            }
         }
      }

      rc = _addHost( host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add host: %s, rc=%d",
                 hostName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( host ) ;
      goto done ;
   }

   INT32 OmCluster::_buildDisk( const BSONObj& bsonDisk, OmHost& host )
   {
      INT32 rc = SDB_OK ;
      simpleDiskInfo disk ;

      try
      {
         disk.diskName  = bsonDisk.getStringField( OM_BSON_FIELD_DISK_NAME ) ;
         if ( disk.diskName == "" )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "invalid disk name, host: %s",
                    host.getHostName().c_str() ) ;
            goto error ;
         }
         disk.mountPath = bsonDisk.getStringField( OM_BSON_FIELD_DISK_MOUNT ) ;
         if ( disk.mountPath == "" )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "invalid mount path, disk name: %s",
                    disk.diskName.c_str() ) ;
            goto error ;
         }
         disk.totalSize = bsonDisk.getField( OM_HOST_FIELD_DISK_SIZE ).numberLong() ;
         disk.freeSize = bsonDisk.getField( OM_HOST_FIELD_DISK_FREE_SIZE ).numberLong() ;
      }
      catch ( exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "unexpected error happened when add disk: %s", e.what() ) ;
         goto error ;
      }

      rc = host.addDisk( disk ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add disk to host, disk: %s, host: %s",
                 disk.diskName.c_str(), host.getHostName().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::_buildNode( const BSONObj& bsonNode, OmHost& host )
   {
      INT32 rc = SDB_OK ;
      string businessType ;
      string deployMode ;
      OmNode* node = NULL ;
      OmBusiness* business = NULL ;

      businessType = bsonNode.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
      deployMode = bsonNode.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;

      rc = OmNode::createObject( businessType, deployMode, node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create OmNode object: rc=%d", rc ) ;
         goto error ;
      }

      rc = node->init( bsonNode, host, *this ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init OmNode: rc=%d", rc ) ;
         goto error ;
      }

      rc = getBusiness( node->getBusinessInfo().businessName, business ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get business[%s]: rc=%d",
                 node->getBusinessInfo().businessName.c_str(), rc ) ;
         goto error ;
      }

      rc = business->addNode (node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to add node to business: rc=%d", rc ) ;
         goto error ;
      }

      rc = host.addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add node to host: rc=%d", rc ) ;
         goto error ;
      }

      // add node to cluser
      // must be at the last, because the cluster manages the node
      rc = _addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to add node to cluster: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmCluster::_addHost( OmHost* host )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != host, "host can't be NULL") ;

      if ( _hosts.end() != _hosts.find( host->getHostName() ) )
      {
         PD_LOG_MSG(PDERROR, "host already exists: %s", host->getHostName().c_str() );
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         _hosts[ host->getHostName() ] = host ;
      }
      catch (exception& e)
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when add host:%s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::addNode( OmNode* node )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != node, "node can't be NULL") ;

      // add to business
      {
         OmBusiness* business = NULL ;
         rc = getBusiness( node->getBusinessInfo().businessName, business ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get business [%s] from cluster", 
                        node->getBusinessInfo().businessName.c_str() ) ;
            goto error ;
         }

         rc = business->addNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to add node to business" ) ;
            goto error ;
         }
      }

      // add to host
      {
         OmHost* host = NULL ;
         rc = getHost( node->getHostName(), host ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get host [%s] from cluster", 
                    node->getHostName().c_str() ) ;
            goto error ;
         }

         rc = host->addNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to host" ) ;
            goto error ;
         }
      }

      // must be at the last, because of the node memory is managed by cluster
      // add to nodes
      rc = _addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to add node to cluster" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::addBusiness( OmBusiness* business )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != business, "business can't be NULL") ;

      string businessName = business->getBusinessInfo().businessName ;

      if ( 0 != _businesses.size() &&
           _businesses.end() != _businesses.find( businessName ) )
      {
         PD_LOG_MSG(PDERROR, "business already exists: %s", businessName.c_str() );
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         _businesses[ businessName ] = business ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when add business: %s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmCluster::_addNode( OmNode* node )
   {
      SDB_ASSERT( NULL != node, "node can't be NULL") ;

      return _nodes._addNode( node ) ;
   }
}

