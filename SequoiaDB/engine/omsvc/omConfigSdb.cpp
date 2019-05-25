/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = omConfigSdb.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigSdb.hpp"
#include "omDef.hpp"
#include <sstream>

namespace engine
{
   #define OM_SVCNAME_STEP                (5)
   #define OM_SDB_PORT_NUM                (6)
   #define OM_INT32_MAXVALUE_STR          "2147483647"

   #define OM_DG_NAME_PATTERN             "group"

   OmSdbNode::OmSdbNode()
   {
   }

   OmSdbNode::~OmSdbNode()
   {
   }

   INT32 OmSdbNode::_init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster )
   {
      INT32 rc = SDB_OK ;

      string dbPath = bsonNode.getStringField( OM_CONF_DETAIL_DBPATH ) ;
      string role = bsonNode.getStringField( OM_CONF_DETAIL_ROLE ) ;
      string serviceName = bsonNode.getStringField( OM_CONF_DETAIL_SVCNAME ) ;
      string groupName = bsonNode.getStringField(
                                                OM_CONF_DETAIL_DATAGROUPNAME ) ;
      string transaction = bsonNode.getStringField(
                                                OM_CONF_DETAIL_TRANSACTIONON ) ;

      rc = _setDBPath( dbPath, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used path to node" );
         goto error ;
      }

      rc = _setServiceName( serviceName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      _setRole( role ) ;
      _setGroupName( groupName ) ;
      _setTransaction( transaction ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSdbNode::_setServiceName( const string& serviceName )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( serviceName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      rc = _addUsedPort( strPlus( serviceName, 1 ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      rc = _addUsedPort( strPlus( serviceName, 2 ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      rc = _addUsedPort( strPlus( serviceName, 3 ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      rc = _addUsedPort( strPlus( serviceName, 4 ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      rc = _addUsedPort( strPlus( serviceName, 5 ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }

      _serviceName = serviceName ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSdbNode::_setDBPath( const string& dbPath, OmHost& host )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( dbPath, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add db path to node" );
         goto error ;
      }

      _dbPath = dbPath ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmSdbNode::_setRole( const string& role )
   {
      _role = role ;
   }

   void OmSdbNode::_setGroupName( const string& groupName )
   {
      _groupName = groupName ;
   }

   void OmSdbNode::_setTransaction( const string& transaction )
   {
      _transaction = transaction ;
   }

   OmSdbConfTemplate::OmSdbConfTemplate()
                  :_replicaNum( -1 ), _dataNum( 0 ), 
                   _catalogNum( -1 ), _dataGroupNum( -1 ), _coordNum( -1 )
   {
   }

   OmSdbConfTemplate::~OmSdbConfTemplate()
   {
      reset() ;
   }

   INT32 OmSdbConfTemplate::_init()
   {
      _dataNum = _dataGroupNum * _replicaNum ;
      return SDB_OK ;
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"sequoiadb", "BusinessName":"b1",
      "DeployMod": "standalone", 
      "Property":[{"Name":"replicanum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 OmSdbConfTemplate::_setPropery( const string& name, const string& value )
   {
      if ( name.compare( OM_TEMPLATE_REPLICA_NUM ) == 0 )
      {
         _replicaNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_DATAGROUP_NUM ) == 0 )
      {
         _dataGroupNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_CATALOG_NUM ) == 0 )
      {
         _catalogNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_COORD_NUM ) == 0 )
      {
         _coordNum = ossAtoi( value.c_str() ) ;
      }

      return SDB_OK ;
   }

   void OmSdbConfTemplate::_reset()
   {
      _replicaNum   = -1 ;
      _dataNum      = -1 ;
      _dataGroupNum = -1 ;
      _catalogNum   = -1 ;
      _coordNum     = -1 ;
   }

   bool OmSdbConfTemplate::_isAllProperySet()
   {
      if ( _replicaNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_REPLICA_NUM ) ;
         return false ;
      }
      else if ( _dataGroupNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_DATAGROUP_NUM ) ;
         return false ;
      }
      else if ( _catalogNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_CATALOG_NUM ) ;
         return false ;
      }
      else if ( _coordNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", OM_TEMPLATE_COORD_NUM ) ;
         return false ;
      }

      return true ;
   }

   void OmSdbConfTemplate::setCoordNum( INT32 coordNum )
   {
      _coordNum = coordNum ;
   }

   OmSdbConfProperties::OmSdbConfProperties()
   {
   }

   OmSdbConfProperties::~OmSdbConfProperties()
   {
   }

   bool OmSdbConfProperties::isPrivateProperty( const string& name ) const
   {
      if ( OM_CONF_DETAIL_DBPATH == name ||
           OM_CONF_DETAIL_ROLE == name ||
           OM_CONF_DETAIL_SVCNAME == name )
      {
         return true ;
      }

      return false ;
   }

   bool OmSdbConfProperties::isAllPropertySet()
   {
      if ( !isPropertySet( OM_CONF_DETAIL_DBPATH ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_DBPATH ) ;
         return false ;
      }

      if ( !isPropertySet( OM_CONF_DETAIL_ROLE ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_ROLE ) ;
         return false ;
      }

      if ( !isPropertySet( OM_CONF_DETAIL_SVCNAME ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_SVCNAME ) ;
         return false ;
      }

      return true ;
   }

   class bySdbRole
   {
   public:
      bySdbRole( const string& role ):
         _role( role )
      {
      }

      bool operator() ( const OmNode* node ) const
      {
         if ( OM_BUSINESS_SEQUOIADB != node->getBusinessInfo().businessType )
         {
            return false ;
         }

         const OmSdbNode* sdbNode = dynamic_cast<const OmSdbNode*>( node ) ;
         if ( _role == sdbNode->getRole() )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _role ;
   } ;

   /*
      get best host rule:
          rule1: the less the better which host contains specify role's count
          rule2: the more the better which host contains unused disk's count
          rule3: the less the better which host contains node's count
                 ( all the roles )
   */
   class getBestHostForSdb
   {
   public:
      getBestHostForSdb( const string& role ):
         _role( role ),
         _bySdbRole( role )
      {
      }

      int operator() ( const OmHost* host1, const OmHost* host2 )
      {
         SDB_ASSERT( NULL != host1, "host1 can't be NULL" ) ;
         SDB_ASSERT( NULL != host2, "host2 can't be NULL" ) ;

         INT32 roleNum1 = host1->count( _bySdbRole ) ;
         INT32 roleNum2 = host2->count( _bySdbRole ) ;
         if ( roleNum1 != roleNum2 )
         {
            if ( roleNum1 < roleNum2 )
            {
               return 1 ;
            }
            else
            {
               return -1 ;
            }
         }

         INT32 unusedDiskNum1 = host1->unusedDiskNum() ;
         INT32 unusedDiskNum2 = host2->unusedDiskNum() ;
         if ( unusedDiskNum1 != unusedDiskNum2 )
         {
            if ( unusedDiskNum1 > unusedDiskNum2 )
            {
               return 1 ;
            }
            else
            {
               return -1 ;
            }
         }

         INT32 nodeNum1 = host1->nodeNum() ;
         INT32 nodeNum2 = host2->nodeNum() ;
         if ( nodeNum1 != nodeNum2 )
         {
            if ( nodeNum1 < nodeNum2 )
            {
               return 1 ;
            }
            else
            {
               return -1 ;
            }
         }

         return 0 ;
      }

   private:
      string _role ;
      bySdbRole _bySdbRole ;
   } ;

   class byDiskAndSdbRole
   {
   public:
      byDiskAndSdbRole( const string& diskName, const string& role ):
         _diskName( diskName ), _role( role )
      {
      }

      bool operator() ( const OmNode* node ) const
      {
         if ( OM_BUSINESS_SEQUOIADB != node->getBusinessInfo().businessType )
         {
            return false ;
         }

         if ( !node->isDiskUsed( _diskName ) )
         {
            return false ;
         }

         const OmSdbNode* sdbNode = dynamic_cast<const OmSdbNode*>( node ) ;
         if ( _role == sdbNode->getRole() )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _diskName ;
      string _role ;
   } ;

   class getBestDiskForSdb
   {
   public:
      getBestDiskForSdb( const OmHost& host, const string& role ):
         _host ( host ), _role( role )
      {
      }

      int operator() ( const simpleDiskInfo* disk1, const simpleDiskInfo* disk2 )
      {
         SDB_ASSERT( NULL != disk1, "disk1 can't be NULL" ) ;
         SDB_ASSERT( NULL != disk2, "disk2 can't be NULL" ) ;

         INT32 roleDiskNum1 = _host.count( byDiskAndSdbRole( disk1->diskName, _role ) ) ;
         INT32 roleDiskNum2 = _host.count( byDiskAndSdbRole( disk2->diskName, _role ) ) ;
         if ( roleDiskNum1 != roleDiskNum2 )
         {
            if ( roleDiskNum1 < roleDiskNum2 )
            {
               return 1 ;
            }
            else
            {
               return -1 ;
            }
         }

         INT32 diskNum1 = _host.count( byDisk( disk1->diskName ) ) ;
         INT32 diskNum2 = _host.count( byDisk( disk2->diskName ) ) ;
         if ( diskNum1 != diskNum2 )
         {
            if ( diskNum1 < diskNum2 )
            {
               return 1 ;
            }
            else
            {
               return -1 ;
            }
         }

         return 0 ;
      }

   private:
      const OmHost& _host ;
      string _role ;
   } ;

   OmSdbConfigBuilder::OmSdbConfigBuilder( const OmBusinessInfo& businessInfo ):
      OmConfigBuilder( businessInfo )
   {
   }

   OmSdbConfigBuilder::~OmSdbConfigBuilder()
   {
   }

   INT32 OmSdbConfigBuilder::_build( BSONArray& nodeConfig )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _business, "_business can't be NULL" ) ;
      SDB_ASSERT( 0 == _business->nodeNum(), "_business already has nodes") ;

      _setLocal() ;

      if ( OM_DEPLOY_MOD_STANDALONE == _businessInfo.deployMode )
      {
         rc = _buildStandalone() ;
      }
      else if ( OM_DEPLOY_MOD_DISTRIBUTION == _businessInfo.deployMode )
      {
         rc = _buildCluster() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "unknown deploy mode:type=%s", 
                     _businessInfo.deployMode.c_str() ) ;
         goto error ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "generate config failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONArrayBuilder arrBuilder ;
         const OmNodes& nodes = _business->getNodes() ;
         for ( OmNodes::ConstIterator it = nodes.begin() ;
               it != nodes.end() ; it++ )
         {
            OmSdbNode* node = dynamic_cast<OmSdbNode*>( *it ) ;
            BSONObjBuilder builder ;
            builder.append( OM_BSON_FIELD_HOST_NAME, node->getHostName() ) ;
            builder.append( OM_CONF_DETAIL_EX_DG_NAME, node->getGroupName() ) ;
            builder.append( OM_CONF_DETAIL_DBPATH, node->getDBPath() ) ;
            builder.append( OM_CONF_DETAIL_SVCNAME, node->getServiceName() ) ;
            builder.append( OM_CONF_DETAIL_ROLE, node->getRole() ) ;

            for ( OmConfProperties::ConstIterator it = _properties.begin() ;
                  it != _properties.end() ; it++ )
            {
               const OmConfProperty* property = it->second ;
               if ( !_properties.isPrivateProperty( property->getName() ) )
               {
                  builder.append( property->getName(), property->getDefaultValue() ) ;
               }
            }

            arrBuilder.append( builder.obj() ) ;
         }

         nodeConfig = arrBuilder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmSdbConfigBuilder::_setLocal()
   {
      CHAR local[ OSS_MAX_HOSTNAME + 1 ] = "" ;
      ossGetHostName( local, OSS_MAX_HOSTNAME ) ;
      _localHostName = string( local ) ;

      {
         stringstream ss ;
         pmdKRCB* krcb = pmdGetKRCB() ;
         _pmdOptionsMgr* opt = krcb->getOptionCB() ;
         INT32 servicePort = opt->getServicePort() ;
         ss << servicePort ;
         _localServicePort = ss.str() ;
      }

      _defaultServicePort = _properties.getDefaultValue( OM_CONF_DETAIL_SVCNAME ) ;
   }

   INT32 OmSdbConfigBuilder::_buildStandalone()
   {
      INT32 rc = SDB_OK ;

      rc = _createNode( OM_NODE_ROLE_STANDALONE, "" ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to create node: "
                     "businessName=%s, businessType=%s, role=%s, rc=%d",
                     _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                     OM_NODE_ROLE_STANDALONE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSdbConfigBuilder::_buildCluster()
   {
      INT32 rc = SDB_OK ;

      {
         INT32 coordNum = _template.getCoordNum() ;
         if ( 0 == coordNum )
         {
            coordNum = _cluster.hostNum() ;
         }

         for ( INT32 i = 0 ; i < coordNum ; i++ )
         {
            rc = _createNode( OM_NODE_ROLE_COORD, "" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to create node: "
                           "businessName=%s, businessType=%s, role=%s, rc=%d",
                           _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                           OM_NODE_ROLE_COORD, rc ) ;
               goto error ;
            }
         }
      }

      {
         INT32 cataNum = _template.getCatalogNum() ;
         for ( INT32 i = 0 ; i < cataNum ; i++ )
         {
            rc = _createNode( OM_NODE_ROLE_CATALOG, "" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to create node: "
                           "businessName=%s, businessType=%s, role=%s, rc=%d",
                           _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                           OM_NODE_ROLE_CATALOG, rc ) ;
               goto error ;
            }
         }
      }

      {
         INT32 replicaNum = _template.getReplicaNum() ;
         INT32 groupNum = _template.getDataGroupNum() ;
         for ( INT32 i = 0 ; i < groupNum ; i++ )
         {
            string groupName = strConnect( OM_DG_NAME_PATTERN, i + 1 ) ;
            for ( INT32 j = 0 ; j < replicaNum ; j++ )
            {
               rc = _createNode( OM_NODE_ROLE_DATA, groupName ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR, "failed to create node: "
                              "businessName=%s, businessType=%s, role=%s, rc=%d",
                              _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                              OM_NODE_ROLE_DATA, rc ) ;
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

   INT32 OmSdbConfigBuilder::_createNode( const string& role, const string& groupName )
   {
      INT32 rc = SDB_OK ;
      OmSdbNode* node = NULL ;
      OmHost* host = NULL ;
      const simpleDiskInfo* disk = NULL ;
      string serviceName ;
      string dbPath ;

      host = _cluster.chooseHost( getBestHostForSdb( role ) ) ;
      if ( NULL == host || host->diskNum() == 0 )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no host or disk" ) ;
         goto error ;
      }

      disk = host->chooseDisk( getBestDiskForSdb( *host, role ) ) ;
      if ( NULL == disk )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no disk" ) ;
         goto error ;
      }

      rc = _getServiceName( *host, serviceName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get service port" ) ;
         goto error ;
      }

      rc = _getDBPath( *host, disk->mountPath, _businessInfo.businessType,
                       role, serviceName, dbPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get db path" ) ;
         goto error ;
      }

      node = SDB_OSS_NEW OmSdbNode() ;
      if ( NULL == node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create OmSdbNode, out of memory" ) ;
         goto error ;
      }

      rc = node->_setDBPath( dbPath, *host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set db path" ) ;
         goto error ;
      }

      rc = node->_setServiceName( serviceName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set service name" ) ;
         goto error ;
      }

      node->_setRole( role ) ;
      node->_setGroupName( groupName ) ;
      node->_setBusinessInfo( _businessInfo ) ;
      node->_setHostName( host->getHostName() ) ;

      rc = _cluster.addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add node to business: "
                 "businessType=%s, businessName=%s, role=%s, rc=%d",
                 _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                 role.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmSdbConfigBuilder::_getServiceName( const OmHost& host, string& serviceName )
   {
      INT32 rc = SDB_OK ;
      INT32 startPort = ossAtoi( _defaultServicePort.c_str() ) ;

      if ( host.getHostName() == _localHostName )
      {
         INT32 localPort = ossAtoi( _localServicePort.c_str() ) ;
         if ( startPort <= localPort )
         {
            startPort = localPort +  OM_SVCNAME_STEP ;
         }
         else if ( startPort < localPort + OM_SVCNAME_STEP )
         {
            startPort += OM_SVCNAME_STEP ;
         }
      }

      bool ok = false ;
      for ( ; startPort < OM_MAX_PORT ; startPort += OM_SVCNAME_STEP )
      {
         bool used = false ;
         for ( INT32 i = 0; i < OM_SDB_PORT_NUM; i++ )
         {
            if ( host.isPortUsed( startPort + i ) )
            {
               used = true ;
               break ;
            }
         }

         if ( !used )
         {
            ok = true ;
            break ;
         }
      }

      if ( !ok )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "failed to get unused port from host %s",
                 host.getHostName().c_str() ) ;
         goto error ;
      }

      {
         stringstream ss ;
         ss << startPort ;
         serviceName = ss.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSdbConfigBuilder::_getDBPath( OmHost& host, const string& diskPath,
                           const string& businessType, const string& role,
                           const string& serviceName, string& dbPath )
   {
      INT32 i = 0 ;
      INT32 rc = SDB_OK ;

      do 
      {
         stringstream ss ;
         ss << diskPath ;
         if ( OSS_FILE_SEP_CHAR != diskPath.at( diskPath.length() -1 ) )
         {
            ss << OSS_FILE_SEP ;
         }
         ss << businessType << OSS_FILE_SEP
            << OM_DBPATH_PREFIX_DATABASE << OSS_FILE_SEP
            << role << OSS_FILE_SEP
            << serviceName ;
         if ( 0 != i )
         {
            ss << "_" << i ;
         }
         i++ ;
         dbPath = ss.str() ;
      } while ( host.isPathUsed( dbPath ) ) ;

      if ( dbPath.length() > OM_PATH_LENGTH )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "the length of dbpath is too long, length = %d",
                 dbPath.length() );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #define OM_CONFIG_SDB_BUILDER_TRUE1    "true"
   #define OM_CONFIG_SDB_BUILDER_TRUE2    "TRUE"
   #define OM_CONFIG_SDB_BUILDER_FALSE1   "false"
   #define OM_CONFIG_SDB_BUILDER_FALSE2   "FALSE"

   INT32 OmSdbConfigBuilder::_check( BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      BSONElement configEle ;
      string deployMode ;

      if ( TRUE == _force )
      {
         _properties.setForce() ;
      }

      deployMode = _businessInfo.deployMode;
      if ( OM_DEPLOY_MOD_STANDALONE != deployMode &&
           OM_DEPLOY_MOD_DISTRIBUTION != deployMode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid deploy mode:%s", 
                     deployMode.c_str() ) ;
         goto error ;
      }

      configEle = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array:field=%s,"
                     "type=%d", OM_BSON_FIELD_CONFIG, configEle.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( configEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj bsonNode = ele.embeddedObject() ;
               rc = _checkAndAddNode( bsonNode ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "check node config failed: rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      {
         const OmNodes& nodes = _business->getNodes() ;

         if ( OM_DEPLOY_MOD_STANDALONE == deployMode )
         {
            if ( nodes.nodeNum() != 1 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "can't install more than one node in %s mode",
                           OM_DEPLOY_MOD_STANDALONE ) ;
               goto error ;
            }

            OmNodes::ConstIterator it = nodes.begin() ;
            OmSdbNode* node = dynamic_cast<OmSdbNode*>( *it ) ;
            string role = node->getRole() ;
            if ( role != OM_NODE_ROLE_STANDALONE )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "can't install node with role=%s in %s mode", 
                           role.c_str(), OM_DEPLOY_MOD_STANDALONE ) ;
               goto error ;
            }
         }
         else // OM_DEPLOY_MOD_DISTRIBUTION
         {
            INT32 coordNum = 0 ;
            INT32 cataNum = 0 ;
            INT32 dataNum = 0 ;
            map<string, string> groupTransMap ;
            map<string, string>::iterator groupIter ;

            for ( OmNodes::ConstIterator it = nodes.begin() ;
               it != nodes.end() ; it++ )
            {
               OmSdbNode* node = dynamic_cast<OmSdbNode*>( *it ) ;
               string role = node->getRole() ;
               string groupName = node->getGroupName() ;
               string transaction = node->getTransaction() ;

               if ( OM_CONFIG_SDB_BUILDER_TRUE1 == transaction )
               {
                  transaction = OM_CONFIG_SDB_BUILDER_TRUE2 ;
               }
               else if ( OM_CONFIG_SDB_BUILDER_FALSE1 == transaction )
               {
                  transaction = OM_CONFIG_SDB_BUILDER_FALSE2 ;
               }

               if ( OM_NODE_ROLE_STANDALONE == role )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "can't install node with role=%s in %s mode",
                              role.c_str(), OM_DEPLOY_MOD_DISTRIBUTION ) ;
                  goto error ;
               }

               if ( role == OM_NODE_ROLE_CATALOG )
               {
                  cataNum++ ;
               }
               else if ( role == OM_NODE_ROLE_COORD )
               {
                  coordNum++ ;
               }
               else if ( role == OM_NODE_ROLE_DATA )
               {
                  dataNum++ ;
                  groupIter = groupTransMap.find( groupName ) ;
                  if( groupIter == groupTransMap.end() )
                  {
                     groupTransMap.insert(
                           pair<string, string>( groupName, transaction ) ) ;
                  }
                  else
                  {
                     if( groupIter->second != transaction )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG_MSG( PDERROR, "transaction exist conflict value:"
                                    "host=%s, svcname=%s, transactionon=%s",
                                    node->getHostName().c_str(),
                                    node->getServiceName().c_str(),
                                    transaction.c_str() ) ;
                        goto error ;
                     }
                  }
               }
            }

            if ( cataNum == 0 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "catalog number can't be zero" ) ;
               goto error ;
            }

            if ( coordNum == 0 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "coord number can't be zero" ) ;
               goto error ;
            }

            if ( dataNum == 0 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "data number can't be zero" ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSdbConfigBuilder::_checkAndAddNode( const BSONObj& bsonNode )
   {
      INT32 rc = SDB_OK ;
      OmSdbNode* node = NULL ;
      OmHost* host = NULL ;
      string hostName ;
      BSONObjIterator itemIter( bsonNode ) ;

      node = SDB_OSS_NEW OmSdbNode() ;
      if ( NULL == node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create OmSdbNode, out of memory" ) ;
         goto error ;
      }

      node->_setBusinessInfo( _businessInfo ) ;

      hostName = bsonNode.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      rc = _cluster.getHost( hostName, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get host[%s]", hostName.c_str() ) ;
         goto error ;
      }

      while ( itemIter.more() )
      {
         BSONElement itemEle = itemIter.next() ;
         string fieldName    = itemEle.fieldName() ;
         string value ;

         if ( String != itemEle.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "the type of field[%s] should be \"String\"",
                        fieldName.c_str() ) ;
            goto error ;
         }

         value = itemEle.String() ;

         if ( OM_BSON_FIELD_HOST_NAME == fieldName )
         {
            node->_setHostName( value ) ;
         }
         else if ( OM_CONF_DETAIL_DATAGROUPNAME == fieldName )
         {
            node->_setGroupName( value ) ;
         }
         else
         {
            rc = _properties.checkValue( fieldName, value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check value failed:name=%s,value=%s", 
                       fieldName.c_str(), value.c_str() ) ;
               goto error ;
            }

            if ( OM_CONF_DETAIL_DBPATH == fieldName )
            {
               if ( host->isPathUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "db path has been used: host=%s, path=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setDBPath( value, *host ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set dbpath: host=%s, path=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_CONF_DETAIL_SVCNAME == fieldName )
            {
               if ( _isServiceNameUsed( *host, value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "service name has been used: host=%s, serviceName=%s", 
                       hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setServiceName( value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set service name: host=%s, serviceName=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_CONF_DETAIL_ROLE == fieldName )
            {
               node->_setRole( value ) ;
            }
            else if ( OM_CONF_DETAIL_TRANSACTION == fieldName )
            {
               node->_setTransaction( value ) ;
            }
         }
      }

      rc = _cluster.addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add node to cluster: host=%s, serviceName=%s", 
                 hostName.c_str(), node->getServiceName().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   bool OmSdbConfigBuilder::_isServiceNameUsed( const OmHost& host, const string& serviceName )
   {
      INT32 startPort = ossAtoi( serviceName.c_str() ) ;

      for ( INT32 i = 0; i < OM_SDB_PORT_NUM; i++ )
      {
         if ( host.isPortUsed( startPort + i ) )
         {
            return true ;
         }
      }

      return false ;
   }

   OmSdbConfExtendTemplate::OmSdbConfExtendTemplate()
   {
   }

   OmSdbConfExtendTemplate::~OmSdbConfExtendTemplate()
   {
   }

   INT32 OmSdbConfExtendTemplate::_init()
   {
      _dataNum = _dataGroupNum * _replicaNum ;
      return SDB_OK ;
   }

   void OmSdbConfExtendTemplate::_reset()
   {
      _replicaNum   = -1 ;
      _dataNum      = -1 ;
      _dataGroupNum = -1 ;
      _catalogNum   = -1 ;
      _coordNum     = -1 ;
   }

   INT32 OmSdbConfExtendTemplate::_setPropery( const string& name,
                                               const string& value )
   {
      if ( name.compare( OM_TEMPLATE_REPLICA_NUM ) == 0 )
      {
         _replicaNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_DATAGROUP_NUM ) == 0 )
      {
         _dataGroupNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_CATALOG_NUM ) == 0 )
      {
         _catalogNum = ossAtoi( value.c_str() ) ;
      }
      else if ( name.compare( OM_TEMPLATE_COORD_NUM ) == 0 )
      {
         _coordNum = ossAtoi( value.c_str() ) ;
      }

      return SDB_OK ;
   }

   bool OmSdbConfExtendTemplate::_isAllProperySet()
   {
      if ( _replicaNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_REPLICA_NUM ) ;
         return false ;
      }

      if( _deployMod == OM_REST_DEPLOYMOD_HORIZONTAL )
      {
         if ( _dataGroupNum == -1 )
         {
            PD_LOG_MSG( PDERROR, "%s have not been set", 
                        OM_TEMPLATE_DATAGROUP_NUM ) ;
            return false ;
         }
         
      }
      else if ( _deployMod == OM_REST_DEPLOYMOD_VERTICAL )
      {
         
         if ( _coordNum == -1 )
         {
            PD_LOG_MSG( PDERROR, "%s have not been set", OM_TEMPLATE_COORD_NUM ) ;
            return false ;
         }
         else if ( _catalogNum == -1 )
         {
            PD_LOG_MSG( PDERROR, "%s have not been set", 
                        OM_TEMPLATE_CATALOG_NUM ) ;
            return false ;
         }
      }
      else
      {
         PD_LOG_MSG( PDERROR, "unknown deploy mode:type=%s", 
                     _deployMod.c_str() ) ;
         return false ;
      }

      return true ;
   }

   OmExtendSdbConfigBuilder::OmExtendSdbConfigBuilder(
      const OmBusinessInfo& businessInfo ) : OmSdbConfigBuilder( businessInfo )
   {
   }

   OmExtendSdbConfigBuilder::~OmExtendSdbConfigBuilder()
   {
   }

   INT32 OmExtendSdbConfigBuilder::_getGroupInfo( map<string, INT32>& groupMap,
                                                  INT32& coordNum,
                                                  INT32& catalogNum )
   {
      INT32 rc = SDB_OK ;
      map<string, INT32>::iterator groupIter ;

      /*
      _bsonHostInfo:
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

      BSONObj info = _bsonHostInfo.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;
      BSONObjIterator iter( info ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         if ( ele.type() != Object )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "field's element is not Object:field=%s"
                        ",type=%d", OM_BSON_FIELD_CONFIG, ele.type() ) ;
            goto error ;
         }
         BSONObj oneHost = ele.embeddedObject() ;

         /*
         oneHost
           {
              "HostName":"host1", "ClusterName":"c1",
              "Config": [ { "datagroupname": "g1", "role": "data", ... }, ... ]
               , ...
           }
         */
         BSONObj tmpConfig = oneHost.getObjectField(
                                                   OM_CONFIGURE_FIELD_CONFIG ) ;

         {
            BSONObjIterator iter( tmpConfig ) ;
            while( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if( Object == ele.type() )
               {
                  BSONObj nodeInfo = ele.embeddedObject() ;
                  string role = nodeInfo.getStringField( OM_CONF_DETAIL_ROLE ) ;
                  string groupName = nodeInfo.getStringField(
                                                OM_CONF_DETAIL_DATAGROUPNAME ) ;
                  string businessName = nodeInfo.getStringField(
                                             OM_CONFIGURE_FIELD_BUSINESSNAME ) ;
                  if( businessName != _businessInfo.businessName )
                  {
                     continue ;
                  }
                  if( role == OM_NODE_ROLE_DATA )
                  {
                     groupIter = groupMap.find( groupName ) ;
                     if( groupIter == groupMap.end() )
                     {
                        groupMap.insert( pair<string, INT32>( groupName, 1 ) ) ;
                     }
                     else
                     {
                        ++groupIter->second ;
                     }
                  }
                  else if( role == OM_NODE_ROLE_COORD )
                  {
                     ++coordNum ;
                  }
                  else if( role == OM_NODE_ROLE_CATALOG )
                  {
                     ++catalogNum ;
                  }
               }
            }
         }
         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmExtendSdbConfigBuilder::_build( BSONArray& nodeConfig )
   {
      INT32 rc = SDB_OK ;
      INT32 nodeNum = 0 ;
      INT32 coordNum = 0 ;
      INT32 catalogNum = 0 ;
      map<string, INT32> groupMap ;
      SDB_ASSERT( NULL != _business, "_business can't be NULL" ) ;

      nodeNum = _business->nodeNum() ;

      _setLocal() ;

      rc = _getGroupInfo( groupMap, coordNum, catalogNum ) ;
      if( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get group info, rc=%d", rc ) ;
         goto error ;
      }

      if ( OM_REST_DEPLOYMOD_HORIZONTAL == _businessInfo.deployMode )
      {
         rc = _buildHorizontal( groupMap ) ;
      }
      else if ( OM_REST_DEPLOYMOD_VERTICAL == _businessInfo.deployMode )
      {
         rc = _buildVertical( coordNum, catalogNum, groupMap ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "unknown deploy mode:type=%s", 
                     _businessInfo.deployMode.c_str() ) ;
         goto error ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "generate config failed:rc=%d", rc ) ;
         goto error ;
      }

      if( _business->nodeNum() < nodeNum )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "generate config failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         INT32 i = 0 ;
         BSONArrayBuilder arrBuilder ;
         const OmNodes& nodes = _business->getNodes() ;

         for( OmNodes::ConstIterator it = nodes.begin(); it != nodes.end();
              ++it )
         {
            if( i < nodeNum )
            {
               ++i ;
               continue ;
            }
            OmSdbNode* node = dynamic_cast<OmSdbNode*>( *it ) ;
            BSONObjBuilder builder ;
            builder.append( OM_BSON_FIELD_HOST_NAME, node->getHostName() ) ;
            builder.append( OM_CONF_DETAIL_EX_DG_NAME, node->getGroupName() ) ;
            builder.append( OM_CONF_DETAIL_DBPATH, node->getDBPath() ) ;
            builder.append( OM_CONF_DETAIL_SVCNAME, node->getServiceName() ) ;
            builder.append( OM_CONF_DETAIL_ROLE, node->getRole() ) ;

            for ( OmConfProperties::ConstIterator it = _properties.begin() ;
                  it != _properties.end() ; it++ )
            {
               const OmConfProperty* property = it->second ;
               if ( !_properties.isPrivateProperty( property->getName() ) )
               {
                  builder.append( property->getName(), property->getDefaultValue() ) ;
               }
            }
            arrBuilder.append( builder.obj() ) ;

         }

         nodeConfig = arrBuilder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmExtendSdbConfigBuilder::_buildHorizontal(
                                                  map<string, INT32>& groupMap )
   {
      INT32 rc = SDB_OK ;
      INT32 replicaNum = _template.getReplicaNum() ;
      INT32 groupNum = _template.getDataGroupNum() ;
      INT32 k = 0 ;

      for( INT32 i = 0; i < groupNum; ++k )
      {
         string groupName = strConnect( OM_DG_NAME_PATTERN, k + 1 ) ;
         if( groupMap.find( groupName ) != groupMap.end() )
         {
            continue ;
         }

         for ( INT32 j = 0 ; j < replicaNum ; j++ )
         {
            rc = _createNode( OM_NODE_ROLE_DATA, groupName ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to create node: "
                           "businessName=%s, businessType=%s, role=%s, rc=%d",
                           _businessInfo.businessType.c_str(),
                           _businessInfo.businessName.c_str(),
                           OM_NODE_ROLE_DATA, rc ) ;
               goto error ;
            }
         }
         ++i ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 OmExtendSdbConfigBuilder::_buildVertical( INT32 existCoord,
                                                   INT32 existCatalog,
                                                   map<string, INT32>& dataGroup
                                                 )
   {
      INT32 rc = SDB_OK ;
      map<string, INT32>::iterator iter ;

      {
         INT32 coordNum = _template.getCoordNum() ;
         if ( 0 == coordNum )
         {
            coordNum = _cluster.hostNum() ;
         }

         coordNum = coordNum - existCoord ;
         for ( INT32 i = 0; i < coordNum; ++i )
         {
            rc = _createNode( OM_NODE_ROLE_COORD, "" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to create node: "
                           "businessName=%s, businessType=%s, role=%s, rc=%d",
                           _businessInfo.businessType.c_str(),
                           _businessInfo.businessName.c_str(),
                           OM_NODE_ROLE_COORD, rc ) ;
               goto error ;
            }
         }
      }

      {
         INT32 cataNum = _template.getCatalogNum() - existCatalog ;
         for ( INT32 i = 0; i < cataNum; ++i )
         {
            rc = _createNode( OM_NODE_ROLE_CATALOG, "" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to create node: "
                           "businessName=%s, businessType=%s, role=%s, rc=%d",
                           _businessInfo.businessType.c_str(),
                           _businessInfo.businessName.c_str(),
                           OM_NODE_ROLE_CATALOG, rc ) ;
               goto error ;
            }
         }
      }

      {
         INT32 replicaNum = _template.getReplicaNum() ;
         for( iter = dataGroup.begin(); iter != dataGroup.end(); ++iter )
         {
            INT32 dataNum = replicaNum - iter->second ;
            string groupName = iter->first ;
            for ( INT32 i = 0; i < dataNum; ++i )
            {
               rc = _createNode( OM_NODE_ROLE_DATA, groupName ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR, "failed to create node: "
                              "businessName=%s, businessType=%s, role=%s, rc=%d",
                              _businessInfo.businessType.c_str(),
                              _businessInfo.businessName.c_str(),
                              OM_NODE_ROLE_DATA, rc ) ;
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

}
