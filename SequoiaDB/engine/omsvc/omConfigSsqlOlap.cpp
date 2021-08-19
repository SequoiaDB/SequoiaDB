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

   Source File Name = omConfigSsqlOlap.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          28/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigSsqlOlap.hpp"
#include "omDef.hpp"
#include <set>
#include <sstream>
#include <boost/filesystem/fstream.hpp>

namespace engine
{
   #define OM_SSQL_OLAP_STEP (10)

   static const CHAR* ssqlOlapProperties[] =
   {
      OM_SSQL_OLAP_CONF_MASTER_HOST,
      OM_SSQL_OLAP_CONF_MASTER_PORT,
      OM_SSQL_OLAP_CONF_MASTER_DIR,
      OM_SSQL_OLAP_CONF_MASTER_TEMP_DIR,
      OM_SSQL_OLAP_CONF_STANDBY_HOST,
      OM_SSQL_OLAP_CONF_SEGMENT_HOSTS,
      OM_SSQL_OLAP_CONF_SEGMENT_PORT,
      OM_SSQL_OLAP_CONF_SEGMENT_DIR,
      OM_SSQL_OLAP_CONF_SEGMENT_TEMP_DIR,
      OM_SSQL_OLAP_CONF_HDFS_URL,
   } ;

   bool _isValidDirectory( const string& dir )
   {
      boost::filesystem::path p(dir) ;
      if ( !p.is_complete() )
      {
         return false ;
      }

      return true ;
   }
   
   OmSsqlOlapNode::OmSsqlOlapNode()
   {
   }

   OmSsqlOlapNode::~OmSsqlOlapNode()
   {
   }

   INT32 OmSsqlOlapNode::_init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster )
   {
      INT32 rc = SDB_OK ;
      string role ;
      string port ;
      string dataDir ;
      string tempDir ;
      string installDir ;
      string isSingle ;

      role = bsonNode.getStringField( OM_SSQL_OLAP_CONF_ROLE ) ;
      installDir = bsonNode.getStringField( OM_SSQL_OLAP_CONF_INSTALL_DIR ) ;

      if ( OM_SSQL_OLAP_MASTER == role || OM_SSQL_OLAP_STANDBY == role )
      {
         port = bsonNode.getStringField( OM_SSQL_OLAP_CONF_MASTER_PORT ) ;
         dataDir = bsonNode.getStringField( OM_SSQL_OLAP_CONF_MASTER_DIR ) ;
         tempDir = bsonNode.getStringField( OM_SSQL_OLAP_CONF_MASTER_TEMP_DIR ) ;
         isSingle = "true" ;
      }
      else if ( OM_SSQL_OLAP_SEGMENT == role )
      {
         port = bsonNode.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_PORT ) ;
         dataDir = bsonNode.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_DIR ) ;
         tempDir = bsonNode.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_TEMP_DIR ) ;
         isSingle = bsonNode.getStringField( OM_SSQL_OLAP_CONF_IS_SINGLE ) ;
         SDB_ASSERT( "true" == isSingle || "false" == isSingle, "invalid is_single") ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid role [%s]", role.c_str() ) ;
         goto error ;
      }

      _setRole( role ) ;

      rc = _setPort( port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to set port of %s node", role.c_str() ) ;
         goto error ;
      }

      rc = _setDataDir( dataDir, host, true ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to set data dir of %s node", role.c_str() ) ;
         goto error ;
      }

      rc = _setTempDir( tempDir, host, true ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to set temp dir of %d node", role.c_str() ) ;
         goto error ;
      }

      if ( "true" == isSingle )
      {
         rc = _setInstallDir( installDir, host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set install dir of %s node", role.c_str() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmSsqlOlapNode::_setRole( const string& role )
   {
      _role = role ;
   }
   
   INT32 OmSsqlOlapNode::_setPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add used port to node" );
         goto error ;
      }
      
      _port = port ;

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 OmSsqlOlapNode::_setDataDir( const string& dataDir, OmHost& host, bool ignoreDisk )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( dataDir, host, ignoreDisk ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add data dir to node" );
         goto error ;
      }

      _dataDir = dataDir ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapNode::_setTempDir( const string& tempDir, OmHost& host, bool ignoreDisk )
   {
      INT32 rc = SDB_OK ;

      if ( !ignoreDisk && NULL == host.getDisk( tempDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "failed to get disk of path[%s]", tempDir.c_str() ) ;
         goto error ;
      }

      _tempDir = tempDir ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapNode::_setInstallDir( const string& installDir, OmHost& host, bool ignoreDisk )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( installDir, host, ignoreDisk ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add install dir to node" );
         goto error ;
      }

      _installDir = installDir ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OmSsqlOlapConfTemplate::OmSsqlOlapConfTemplate():
      _deployStandby( false ), _segmentNum( -1 )
   {
   }

   OmSsqlOlapConfTemplate::~OmSsqlOlapConfTemplate()
   {
      reset() ;
   }

   void OmSsqlOlapConfTemplate::_reset()
   {
      _deployStandby = false ;
      _segmentNum = -1 ;
   }

   bool OmSsqlOlapConfTemplate::_isAllProperySet()
   {
      if ( _segmentNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_SSQL_OLAP_SEGMENT_NUM ) ;
         return false ;
      }

      return true ;
   }

   INT32 OmSsqlOlapConfTemplate::_setPropery( const string& name, const string& value )
   {
      INT32 rc = SDB_OK ;

      if ( name.compare( OM_SSQL_OLAP_DEPLOY_STANDBY ) == 0 )
      {         
         if ( "true" == strLower( value ) )
         {
            _deployStandby = true ;
         }
         else if ( "false" == strLower( value ) )
         {
            _deployStandby = false ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "invalid value \"%s\" of field %s", 
                        value.c_str(), OM_SSQL_OLAP_DEPLOY_STANDBY ) ;
            goto error ;
         }
      }
      else if ( name.compare( OM_SSQL_OLAP_SEGMENT_NUM ) == 0 )
      {
         _segmentNum = ossAtoi( value.c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OmSsqlOlapConfProperties::OmSsqlOlapConfProperties()
   {
   }

   OmSsqlOlapConfProperties::~OmSsqlOlapConfProperties()
   {
   }

   bool OmSsqlOlapConfProperties::isPrivateProperty( const string& name ) const
   {
      if ( OM_SSQL_OLAP_CONF_MASTER_HOST == name ||
           OM_SSQL_OLAP_CONF_MASTER_PORT == name ||
           OM_SSQL_OLAP_CONF_STANDBY_HOST == name ||
           OM_SSQL_OLAP_CONF_MASTER_DIR == name ||
           OM_SSQL_OLAP_CONF_SEGMENT_HOSTS == name ||
           OM_SSQL_OLAP_CONF_SEGMENT_PORT == name ||
           OM_SSQL_OLAP_CONF_SEGMENT_DIR == name )
      {
         return true ;
      }

      return false ;
   }

   bool OmSsqlOlapConfProperties::isAllPropertySet()
   {
      const static CHAR* properties[] = {
         OM_SSQL_OLAP_CONF_MASTER_HOST,
         OM_SSQL_OLAP_CONF_MASTER_PORT,
         OM_SSQL_OLAP_CONF_STANDBY_HOST,
         OM_SSQL_OLAP_CONF_MASTER_DIR,
         OM_SSQL_OLAP_CONF_MASTER_TEMP_DIR,
         OM_SSQL_OLAP_CONF_HDFS_URL,
         OM_SSQL_OLAP_CONF_SEGMENT_HOSTS,
         OM_SSQL_OLAP_CONF_SEGMENT_PORT,
         OM_SSQL_OLAP_CONF_SEGMENT_DIR,
         OM_SSQL_OLAP_CONF_SEGMENT_TEMP_DIR,
         OM_SSQL_OLAP_CONF_INSTALL_DIR,
         OM_SSQL_OLAP_CONF_LOG_DIR,
         OM_SSQL_OLAP_CONF_MAX_CONNECTIONS,
         OM_SSQL_OLAP_CONF_SHARED_BUFFERS,
         OM_SSQL_OLAP_CONF_REMOVE_IF_FAILED
      } ;
      const INT32 num = sizeof( properties ) / sizeof ( properties[0] ) ;

      for ( INT32 i = 0 ; i < num ; i++ )
      {
         if ( !isPropertySet( string( properties[i] ) ) )
         {
            PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                        properties[i] ) ;
            return false ;
         }
      }

      return true ;
   }

   class byOtherSsqlOlap
   {
   public:
      byOtherSsqlOlap( const string& currentOlapName ):
         _currentOlapName( currentOlapName )
      {
      }

      bool operator()( const OmNode* node ) const
      {
         if ( OM_BUSINESS_SEQUOIASQL == node->getBusinessInfo().businessType &&
              OM_SEQUOIASQL_DEPLOY_OLAP == node->getBusinessInfo().deployMode &&
              _currentOlapName != node->getBusinessInfo().businessName )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _currentOlapName ;
   } ;

   class bySsqlOlap
   {
   public:
      bySsqlOlap( const string& businessName = "" )
         : _businessName( businessName )
      {
      }
      
      bool operator() ( const OmNode* node ) const
      {
         if ( OM_BUSINESS_SEQUOIASQL == node->getBusinessInfo().businessType &&
              OM_SEQUOIASQL_DEPLOY_OLAP == node->getBusinessInfo().deployMode )
         {
            if ( _businessName == "" ||
                 _businessName == node->getBusinessInfo().businessName )
            {
               return true ;
            }
         }

         return false ;
      }

   private:
      string _businessName ;
   } ;

   class bySsqlOlapRole
   {
   public:
      bySsqlOlapRole( const string& role, const string& businessName = "" ):
         _role( role ), _businessName( businessName )
      {
      }

      bool operator() ( const OmNode* node ) const
      {
         if ( OM_BUSINESS_SEQUOIASQL != node->getBusinessInfo().businessType ||
              OM_SEQUOIASQL_DEPLOY_OLAP != node->getBusinessInfo().deployMode )
         {
            return false ;
         }

         if ( _businessName != "" &&
              _businessName != node->getBusinessInfo().businessName )
         {
            return false ;
         }

         const OmSsqlOlapNode* sqlOlapNode = dynamic_cast<const OmSsqlOlapNode*>( node ) ;
         if ( _role == sqlOlapNode->getRole() )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _role ;
      string _businessName ;
   } ;

   /*
      get best host rule:
          rule1: the less the better which host contains OLAP's count
          rule2: the more the better which host contains unused disk's count
          rule3: the less the better which host contains node's count
   */
   class getBestHostForSsqlOlap
   {
   public:

      int operator() ( const OmHost* host1, const OmHost* host2 )
      {
         SDB_ASSERT( NULL != host1, "host1 can't be NULL" ) ;
         SDB_ASSERT( NULL != host2, "host2 can't be NULL" ) ;

         INT32 olap1 = host1->count( bySsqlOlap() ) ;
         INT32 olap2 = host2->count( bySsqlOlap() ) ;
         if ( olap1 != olap2 )
         {
            if ( olap1 < olap2 )
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
   } ;

   class excludeHost
   {
   public:
      excludeHost( const string& businessName, const string& role, const set<string>* hosts = NULL ):
         _businessName( businessName ), _role( role ), _hosts( hosts )
      {
      }
      
      bool operator()( const OmHost* host )
      {
         if ( NULL != _hosts )
         {
            if ( _hosts->end() != _hosts->find( host->getHostName() ) )
            {
               return true ;
            }
         }
         
         if ( host->diskNum() == 0 )
         {
            return true ;
         }

         // a host can only deploy one OLAP business
         if ( host->count( byOtherSsqlOlap( _businessName ) ) > 0 )
         {
            return true ;
         }

         // a host can only deploy one node of the same role
         if ( host->count( bySsqlOlapRole( _role ) ) > 0 )
         {
            return true ;
         }

         if ( OM_SSQL_OLAP_MASTER == _role )
         {
            if ( host->count( bySsqlOlapRole( OM_SSQL_OLAP_STANDBY ) ) > 0 )
            {
               return true ;
            }
         }
         else if ( OM_SSQL_OLAP_STANDBY == _role )
         {
            if ( host->count( bySsqlOlapRole( OM_SSQL_OLAP_MASTER ) ) > 0 )
            {
               return true ;
            }
         }

         return false ;
      }

   private:
      string _businessName ;
      string _role ;
      const set<string>* _hosts ;
   } ;

   class getBestDiskForSsqlOlap
   {
   public:
      getBestDiskForSsqlOlap( const OmHost& host ):
         _host ( host )
      {
      }

      int operator() ( const simpleDiskInfo* disk1, const simpleDiskInfo* disk2 )
      {
         SDB_ASSERT( NULL != disk1, "disk1 can't be NULL" ) ;
         SDB_ASSERT( NULL != disk2, "disk2 can't be NULL" ) ;

         INT32 diskNum1 = _host.count( byDisk( disk1->diskName ) ) ;
         INT32 diskNum2 = _host.count( byDisk( disk2->diskName ) ) ;
         if ( diskNum1 != diskNum2 )
         {
            //total count less, the better
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
   } ;
   
   OmSsqlOlapConfigBuilder::OmSsqlOlapConfigBuilder( const OmBusinessInfo& businessInfo ):
      OmConfigBuilder( businessInfo )
   {
   }

   OmSsqlOlapConfigBuilder::~OmSsqlOlapConfigBuilder()
   {
   }

   INT32 OmSsqlOlapConfigBuilder::_build( BSONArray &nodeConfig )
   {
      INT32 rc = SDB_OK ;
      bool deployStandby = _template.getDeployStandby() ;
      INT32 segmentNum = _template.getSegmentNum() ;

      SDB_ASSERT( NULL != _business, "_business can't be NULL" ) ;
      SDB_ASSERT( 0 == _business->nodeNum(), "_business already has nodes") ;

      if ( segmentNum < 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "segment number can't be less than 1" ) ;
         goto error ;
      }

      if ( _cluster.hostNum() < 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host number is less than 1" ) ;
         goto error ;
      }

      if ( deployStandby && _cluster.hostNum() == 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "only one host, can't deploy standby" ) ;
         goto error ;
      }

      if ( segmentNum > _cluster.hostNum() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host number (%d) is less than segment number (%d)",
                     _cluster.hostNum(), segmentNum ) ;
         goto error ;
      }

      // build master & standby nodes
      rc = _createMasterNode( deployStandby ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create master node" ) ;
         goto error ;
      }

      // build segment nodes
      rc = _createSegmentNodes( segmentNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create segment node" ) ;
         goto error ;
      }

      {
         BSONArrayBuilder arrBuilder ;
         BSONArrayBuilder segHostsBuilder ;
         BSONObjBuilder builder ;
         OmNodes masterNodes ;
         OmNodes standbyNodes ;
         OmNodes segmentNodes ;
         OmSsqlOlapNode* masterNode = NULL ;
         OmSsqlOlapNode* standbyNode = NULL ;
         OmSsqlOlapNode* segmentNode = NULL ;
         const OmNodes& nodes = _business->getNodes() ;

         rc = nodes.getNodes( bySsqlOlapRole( OM_SSQL_OLAP_MASTER ), masterNodes ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get master node from business [%s]",
                        _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         if ( masterNodes.nodeNum() != 1 )
         {
            PD_LOG_MSG( PDERROR, "invalid master node number: %d",
                        masterNodes.nodeNum() ) ;
            goto error ;
         }

         masterNode = dynamic_cast<OmSsqlOlapNode*>( *masterNodes.begin() ) ;
         builder.append( OM_SSQL_OLAP_CONF_MASTER_HOST, masterNode->getHostName() ) ;
         builder.append( OM_SSQL_OLAP_CONF_MASTER_PORT, masterNode->getPort() ) ;
         builder.append( OM_SSQL_OLAP_CONF_MASTER_DIR, masterNode->getDataDir() ) ;

         rc = nodes.getNodes( bySsqlOlapRole( OM_SSQL_OLAP_STANDBY ), standbyNodes ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get standby node from business [%s]",
                        _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         if ( standbyNodes.nodeNum() == 0 )
         {
            builder.append( OM_SSQL_OLAP_CONF_STANDBY_HOST, "" ) ;
         }
         else if ( standbyNodes.nodeNum() == 1 )
         {
            standbyNode = dynamic_cast<OmSsqlOlapNode*>( *standbyNodes.begin() ) ;
            builder.append( OM_SSQL_OLAP_CONF_STANDBY_HOST, standbyNode->getHostName() ) ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "invalid standby node number: %d",
                        standbyNodes.nodeNum() ) ;
            goto error ;
         }

         rc = nodes.getNodes( bySsqlOlapRole( OM_SSQL_OLAP_SEGMENT ), segmentNodes ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get segment nodes from business [%s]",
                        _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         if ( segmentNodes.nodeNum() != segmentNum )
         {
            PD_LOG_MSG( PDERROR, "create %d segment node(s), but expected %d",
                        segmentNodes.nodeNum(), segmentNum ) ;
            goto error ;
         }

         for ( OmNodes::ConstIterator it = segmentNodes.begin() ;
               it != segmentNodes.end() ; it++ )
         {
            segmentNode = dynamic_cast<OmSsqlOlapNode*>( *it ) ;

            if ( it == segmentNodes.begin() )
            {
               builder.append( OM_SSQL_OLAP_CONF_SEGMENT_PORT, segmentNode->getPort() ) ;
               builder.append( OM_SSQL_OLAP_CONF_SEGMENT_DIR, segmentNode->getDataDir() ) ;
            }

            segHostsBuilder.append( segmentNode->getHostName() ) ;
         }

         builder.append( OM_SSQL_OLAP_CONF_SEGMENT_HOSTS, segHostsBuilder.arr() ) ;

         // append public properties
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

         nodeConfig = arrBuilder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // master can't be deployed in the same host as standby
   // standby must use the same port and data dir as master
   INT32 OmSsqlOlapConfigBuilder::_createMasterNode( bool deployStandby )
   {
      INT32 rc = SDB_OK ;
      OmHost* masterHost = NULL ;
      OmHost* standbyHost = NULL ;
      string defaultPort ;
      string port ;
      string dataDir ;
      set<OmHost*> hosts ;
      const simpleDiskInfo* disk = NULL ;
      OmSsqlOlapNode* masterNode = NULL ;
      OmSsqlOlapNode* standbyNode = NULL ;

      // choose host for master
      masterHost = _cluster.chooseHost( 
                     getBestHostForSsqlOlap(),
                     excludeHost( _businessInfo.businessName,
                                  OM_SSQL_OLAP_MASTER) ) ;
      if ( NULL == masterHost )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "no suitable host for master node of business [%s]",
                     _businessInfo.businessName.c_str() ) ;
         goto error ;
      }

      hosts.insert( masterHost ) ;

      if ( deployStandby )
      {
         set<string> hostsName ;
         hostsName.insert( masterHost->getHostName() ) ;

         // choose host for standby
         standbyHost = _cluster.chooseHost(
                        getBestHostForSsqlOlap(),
                        excludeHost( _businessInfo.businessName,
                                     OM_SSQL_OLAP_STANDBY,
                                     &hostsName ) ) ;
         if ( NULL == standbyHost )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "no suitable host for standby node of business [%s]",
                        _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         hosts.insert( standbyHost ) ;
      }

      defaultPort = _properties.getDefaultValue( OM_SSQL_OLAP_CONF_MASTER_PORT ) ;
      if ( "" == defaultPort )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid default value of property [%s]",
                     OM_SSQL_OLAP_CONF_MASTER_PORT ) ;
         goto error ;
      }

      rc = _getPort( hosts, defaultPort, port) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get port of master node" ) ;
         goto error ;
      }

      disk = masterHost->chooseDisk( getBestDiskForSsqlOlap( *masterHost ) ) ;
      if ( NULL == disk )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "failed to get disk for master node" ) ;
         goto error ;
      }

      rc = _getDataDir( hosts, disk->mountPath, OM_SSQL_OLAP_MASTER, port, dataDir ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get data dir of master node" ) ;
         goto error ;
      }

      {
         masterNode = SDB_OSS_NEW OmSsqlOlapNode() ;
         if ( NULL == masterNode )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to create OmSsqlOlapNode, out of memory" ) ;
            goto error ;
         }

         masterNode->_setBusinessInfo( _businessInfo ) ;
         masterNode->_setHostName( masterHost->getHostName() ) ;
         masterNode->_setRole( OM_SSQL_OLAP_MASTER ) ;

         rc = masterNode->_setPort( port ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set port for master node" ) ;
            goto error ;
         }

         rc = masterNode->_setDataDir( dataDir, *masterHost ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set data dir for master node" ) ;
            goto error ;
         }

         rc = _cluster.addNode( masterNode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to business: "
                    "businessType=%s, businessName=%s, role=%s, rc=%d",
                    _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                    OM_SSQL_OLAP_MASTER, rc ) ;
            goto error ;
         }

         masterNode = NULL ;
      }

      if ( NULL != standbyHost )
      {
         standbyNode = SDB_OSS_NEW OmSsqlOlapNode() ;
         if ( NULL == standbyNode )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to create OmSsqlOlapNode, out of memory" ) ;
            goto error ;
         }

         standbyNode->_setBusinessInfo( _businessInfo ) ;
         standbyNode->_setHostName( standbyHost->getHostName() ) ;
         standbyNode->_setRole( OM_SSQL_OLAP_STANDBY ) ;

         rc = standbyNode->_setPort( port ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set port for standby node" ) ;
            goto error ;
         }

         rc = standbyNode->_setDataDir( dataDir, *standbyHost, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set data dir for standby node" ) ;
            goto error ;
         }

         rc = _cluster.addNode( standbyNode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to business: "
                    "businessType=%s, businessName=%s, role=%s, rc=%d",
                    _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                    OM_SSQL_OLAP_STANDBY, rc ) ;
            goto error ;
         }

         standbyNode = NULL ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( masterNode ) ;
      SAFE_OSS_DELETE( standbyNode ) ;
      goto done ;
   }

   // segment nodes can't be deployed in the same host
   INT32 OmSsqlOlapConfigBuilder::_createSegmentNodes( INT32 segmentNum )
   {
      INT32 rc = SDB_OK ;
      set<string> hostsName ;
      set<OmHost*> hosts ;
      string port ;
      string dataDir ;
      OmSsqlOlapNode* node = NULL ;

      for ( INT32 i = 0 ; i < segmentNum ; i++ )
      {
         OmHost* host = _cluster.chooseHost( getBestHostForSsqlOlap(),
                                              excludeHost( _businessInfo.businessName,
                                                           OM_SSQL_OLAP_SEGMENT,
                                                           &hostsName ) ) ;
         if ( NULL == host )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "no suitable host for segment node of business [%s]",
                        _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         hostsName.insert( host->getHostName() ) ;
         hosts.insert( host ) ;
      }

      {
         string defaultPort = _properties.getDefaultValue( OM_SSQL_OLAP_CONF_SEGMENT_PORT ) ;
         if ( "" == defaultPort )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "invalid default value of property [%s]",
                        OM_SSQL_OLAP_CONF_SEGMENT_PORT ) ;
            goto error ;
         }

         rc = _getPort( hosts, defaultPort, port) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get port of segment node" ) ;
            goto error ;
         }
      }

      {
         OmHost* host = *( hosts.begin() ) ;
         const simpleDiskInfo* disk = host->chooseDisk( getBestDiskForSsqlOlap( *host ) ) ;
         if ( NULL == disk )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "failed to get disk for segment node" ) ;
            goto error ;
         }

         rc = _getDataDir( hosts, disk->mountPath, OM_SSQL_OLAP_SEGMENT, port, dataDir ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get data dir of segment node" ) ;
            goto error ;
         }
      }

      for ( set<OmHost*>::const_iterator it = hosts.begin() ; it != hosts.end() ; it++ ) 
      {
         OmHost* host = *it ;
         
         node = SDB_OSS_NEW OmSsqlOlapNode() ;
         if ( NULL == node )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to create OmSsqlOlapNode, out of memory" ) ;
            goto error ;
         }

         node->_setBusinessInfo( _businessInfo ) ;
         node->_setHostName( host->getHostName() ) ;
         node->_setRole( OM_SSQL_OLAP_SEGMENT ) ;

         rc = node->_setPort( port ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set port for segment node" ) ;
            goto error ;
         }

         rc = node->_setDataDir( dataDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set data dir for segment node" ) ;
            goto error ;
         }

         rc = _cluster.addNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to business: "
                    "businessType=%s, businessName=%s, role=%s, rc=%d",
                    _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                    OM_SSQL_OLAP_SEGMENT, rc ) ;
            goto error ;
         }

         node = NULL ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_getPort( const set<OmHost*>& hosts, const string& defaultPort, string& port )
   {
      INT32 rc = SDB_OK ;
      INT32 startPort = ossAtoi( defaultPort.c_str() ) ;

      for ( ; startPort < OM_MAX_PORT ; startPort += OM_SSQL_OLAP_STEP )
      {
         if ( !_isPortUsed( hosts, startPort ))
         {
            break ;
         }
      }

      if ( startPort >= OM_MAX_PORT )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "failed to get unused port" ) ;
         goto error ;
      }

      {
         stringstream ss ;
         ss << startPort ;
         port = ss.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   bool OmSsqlOlapConfigBuilder::_isPortUsed( const set<OmHost*>& hosts, INT32 port ) const
   {
      for ( set<OmHost*>::const_iterator it = hosts.begin() ; it != hosts.end() ; it++ )
      {
         OmHost* host = *it ;
         if ( host->isPortUsed( port ) )
         {
            return true ;
         }
      }

      return false ;
   }

   INT32 OmSsqlOlapConfigBuilder::_getDataDir( const set<OmHost*>& hosts, 
                                                 const string& diskPath,
                                                 const string& role,
                                                 const string& port,
                                                 string& dataDir )
   {
      INT32 i = 0 ;
      INT32 rc = SDB_OK ;

      do 
      {
         stringstream ss ;
         ss << diskPath ;
         if ( OSS_FILE_SEP_CHAR != diskPath.at( diskPath.length() - 1 ) )
         {
            ss << OSS_FILE_SEP ;
         }
         ss << OM_BUSINESS_SEQUOIASQL << OSS_FILE_SEP
            << OM_SEQUOIASQL_DEPLOY_OLAP << OSS_FILE_SEP
            << OM_DBPATH_PREFIX_DATABASE << OSS_FILE_SEP
            << role << OSS_FILE_SEP
            << port ;
         if ( 0 != i )
         {
            ss << "_" << i ;
         }
         i++ ;
         dataDir = ss.str() ;
      } while ( _isDataDirUsed( hosts, dataDir) ) ;

      if ( dataDir.length() > OM_PATH_LENGTH )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "the length of data dir is too long, length = %d",
                     dataDir.length() );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_isDataDirUsed( const set<OmHost*>& hosts, const string& dataDir ) const
   {
      for ( set<OmHost*>::const_iterator it = hosts.begin() ; it != hosts.end() ; it++ )
      {
         OmHost* host = *it ;
         if ( host->isPathUsed( dataDir ) )
         {
            return true ;
         }
      }

      return false ;
   }
   
   INT32 OmSsqlOlapConfigBuilder::_check( BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      BSONElement configEle ;
      string deployMode ;
      BSONObj config ;
      INT32 num ;

      deployMode = _businessInfo.deployMode;
      if ( OM_SEQUOIASQL_DEPLOY_OLAP != deployMode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid deploy mode: %s", 
                     deployMode.c_str() ) ;
         goto error ;
      }

      configEle = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array: field=%s, "
                     "type=%d", OM_BSON_FIELD_CONFIG, configEle.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator it( configEle.embeddedObject() ) ;
         BSONElement ele ;
         if ( !it.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "no value in config field" ) ;
            goto error ;
         }

         ele = it.next() ;
         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "the element of config is not Object: type=%d",
                        ele.type() ) ;
            goto error ;
         }

         config = ele.embeddedObject() ;

         if ( it.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "too many values in config" ) ;
            goto error ;
         }
      }

      num = sizeof( ssqlOlapProperties ) / sizeof( ssqlOlapProperties[0] ) ;
      for ( INT32 i = 0 ; i < num ; i++ )
      {
         if ( !config.hasField( ssqlOlapProperties[i] ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "missing config property [%s]", ssqlOlapProperties[i] ) ;
            goto error ;
         }
      }

      {
         BSONObjIterator it( config ) ;
         BSONElement ele ;

         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            string fieldName = ele.fieldName() ;

            if ( OM_SSQL_OLAP_CONF_SEGMENT_HOSTS == fieldName )
            {
               if ( Array != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "the type of property [%s] must be Array",
                              OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
                  goto error ;
               }
            }
            else
            {
               string value ;

               if ( String != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "the type of property [%s] must be String",
                              fieldName.c_str() ) ;
                  goto error ;
               }

               value = ele.String() ;

               if ( OM_SSQL_OLAP_CONF_STANDBY_HOST != fieldName && "" == value )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "the value of field [%s] can't be empty",
                              fieldName.c_str() ) ;
                  goto error ;
               }
               else if ( OM_SSQL_OLAP_CONF_INSTALL_DIR == fieldName || 
                         OM_SSQL_OLAP_CONF_LOG_DIR == fieldName )
               {
                  if ( !_isValidDirectory( value ) )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "the value of field [%s] "
                                 "should be a valid absolute path",
                                 fieldName.c_str() ) ;
                     goto error ;
                  }
               }

               rc = _properties.checkValue( fieldName, value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "invalid value: name=%s,value=%s", 
                          fieldName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
         }
      }

      rc = _checkMaster( config ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check master, rc=%d", rc ) ;
         goto error ;
      }

      rc = _checkSegment( config ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check segment, rc=%d", rc ) ;
         goto error ;
      }

      rc = _rebuild( bsonConfig ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rebuild config, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_checkMaster( const BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      string masterHostName ;
      string masterPort ;
      string masterDataDir ;
      string masterTempDir ;
      string standbyHostName ;
      string installDir ;
      string hostNames[2] ;
      OmSsqlOlapNode* node = NULL ;
      INT32 num ;

      masterHostName = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_MASTER_HOST ) ;
      masterPort = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_MASTER_PORT ) ;
      masterDataDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_MASTER_DIR ) ;
      masterTempDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_MASTER_TEMP_DIR ) ;
      standbyHostName = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_STANDBY_HOST ) ;
      installDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_INSTALL_DIR ) ;

      if ( masterHostName == standbyHostName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "master and standby can't be deployed in the same host [%s]",
                     masterHostName.c_str() ) ;
         goto error ;
      }

      if ( !_isValidDirectory( masterDataDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "master_data_dir [%s] is invalid",
                     masterDataDir.c_str() ) ;
         goto error ;
      }

      if ( !_isValidDirectory( masterTempDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "master_temp_dir [%s] is invalid",
                     masterTempDir.c_str() ) ;
         goto error ;
      }

      if ( masterDataDir == masterTempDir )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "master_temp_dir can't be same as master_data_dir" ) ;
         goto error ;
      }

      if ( !_isValidDirectory( installDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "install_dir [%s] is invalid", installDir.c_str() ) ;
         goto error ;
      }

      hostNames[0] = masterHostName ;
      hostNames[1] = standbyHostName ;
      num = ( "" == standbyHostName ) ? 1 : 2 ;

      for ( INT32 i = 0; i < num ; i++ )
      {
         OmHost* host = NULL ;

         rc = _cluster.getHost( hostNames[i], host ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get host[%s]", hostNames[i].c_str() ) ;
            goto error ;
         }

         if ( host->count( bySsqlOlapRole( OM_SSQL_OLAP_MASTER ) ) != 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host [%s] already has master node",
                        hostNames[i].c_str() ) ;
            goto error ;
         }

         if ( host->count( bySsqlOlapRole( OM_SSQL_OLAP_STANDBY ) ) != 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host [%s] already has standby node",
                        hostNames[i].c_str() ) ;
            goto error ;
         }

         if ( host->isPortUsed( masterPort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "port [%s] is used in host [%s]",
                        masterPort.c_str(), hostNames[i].c_str() ) ;
            goto error ;
         }

         if ( host->isPathUsed( masterDataDir ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "master_data_dir [%s] is used in host [%s]",
                        masterDataDir.c_str(), hostNames[i].c_str() ) ;
            goto error ;
         }

         if ( host->isPathUsed( installDir ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "install_dir [%s] is used in host [%s]",
                        installDir.c_str(), hostNames[i].c_str() ) ;
            goto error ;
         }

         node = SDB_OSS_NEW OmSsqlOlapNode() ;
         if ( NULL == node )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to create OmSsqlOlapNode, out of memory" ) ;
            goto error ;
         }

         node->_setBusinessInfo( _businessInfo ) ;
         node->_setHostName( hostNames[i] ) ;
         node->_setRole( ( i == 0 ) ? OM_SSQL_OLAP_MASTER : OM_SSQL_OLAP_STANDBY ) ;

         rc = node->_setPort( masterPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set port of %s node",
                        node->getRole().c_str() ) ;
            goto error ;
         }

         rc = node->_setDataDir( masterDataDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set data dir of %s node",
                        node->getRole().c_str() ) ;
            goto error ;
         }

         rc = node->_setTempDir( masterTempDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set temp dir of %s node",
                        node->getRole().c_str() ) ;
            goto error ;
         }

         rc = node->_setInstallDir( installDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set install dir of %s node",
                        node->getRole().c_str() ) ;
            goto error ;
         }

         rc = _cluster.addNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to cluster: host=%s, role=%s", 
                    hostNames[i].c_str(), node->getRole().c_str() ) ;
            goto error ;
         }

         node = NULL ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_checkSegment( const BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      string masterHostName ;
      string standbyHostName ;
      string segmentPort ;
      string segmentDataDir ;
      string segmentTempDir ;
      string installDir ;
      set<string> hostNames ;
      OmSsqlOlapNode* node = NULL ;

      segmentPort = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_PORT ) ;
      segmentDataDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_DIR ) ;
      segmentTempDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_SEGMENT_TEMP_DIR ) ;
      installDir = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_INSTALL_DIR ) ;
      masterHostName = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_MASTER_HOST ) ;
      standbyHostName = bsonConfig.getStringField( OM_SSQL_OLAP_CONF_STANDBY_HOST ) ;

      rc = _getSegmentHostNames( bsonConfig, hostNames ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get segment host names from config" ) ;
         goto error ;
      }

      if ( hostNames.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "there is no host for segment" ) ;
         goto error ;
      }

      if ( !_isValidDirectory( segmentDataDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "segment_data_dir [%s] should be valid absolute path",
                     segmentDataDir.c_str() ) ;
         goto error ;
      }

      if ( !_isValidDirectory( segmentTempDir ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "segment_temp_dir [%s] should be valid absolute path",
                     segmentTempDir.c_str() ) ;
         goto error ;
      }

      if ( segmentDataDir == segmentTempDir )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "segment_temp_dir can't be same as segment_data_dir" ) ;
         goto error ;
      }

      for ( set<string>::iterator it = hostNames.begin() ; it != hostNames.end() ; it++ )
      {
         OmHost* host = NULL ;
         string hostName = *it ;

         rc = _cluster.getHost( hostName, host ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to get host[%s]", hostName.c_str() ) ;
            goto error ;
         }

         if ( host->count( bySsqlOlapRole( OM_SSQL_OLAP_SEGMENT ) ) != 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "host [%s] already has segment node",
                        hostName.c_str() ) ;
            goto error ;
         }

         if ( host->isPortUsed( segmentPort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "port [%s] is used in host [%s]",
                        segmentPort.c_str(), hostName.c_str() ) ;
            goto error ;
         }

         if ( host->isPathUsed( segmentDataDir ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "segment_data_dir [%s] is used in host [%s]",
                        segmentDataDir.c_str(), hostName.c_str() ) ;
            goto error ;
         }

         node = SDB_OSS_NEW OmSsqlOlapNode() ;
         if ( NULL == node )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to create OmSsqlOlapNode, out of memory" ) ;
            goto error ;
         }

         node->_setBusinessInfo( _businessInfo ) ;
         node->_setHostName( hostName ) ;
         node->_setRole( OM_SSQL_OLAP_SEGMENT ) ;

         rc = node->_setPort( segmentPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set port of segment node" ) ;
            goto error ;
         }

         rc = node->_setDataDir( segmentDataDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set data dir of segment node" ) ;
            goto error ;
         }

         rc = node->_setTempDir( segmentTempDir, *host, true ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to set temp dir of segment node" ) ;
            goto error ;
         }

         if ( hostName != masterHostName && hostName != standbyHostName )
         {
            // only one segment in this host, no master or standby

            if ( host->isPathUsed( installDir ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "install_dir [%s] is used in host [%s]",
                           installDir.c_str(), hostName.c_str() ) ;
               goto error ;
            }

            rc = node->_setInstallDir( installDir, *host, true ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "failed to set install dir of segment node" ) ;
               goto error ;
            }
         }

         rc = _cluster.addNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add node to cluster: host=%s, role=%s", 
                    hostName.c_str(), node->getRole().c_str() ) ;
            goto error ;
         }

         node = NULL ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_getSegmentHostNames( const BSONObj& bsonConfig, set<string>& hostNames )
   {
      INT32 rc = SDB_OK ;

      BSONElement ele = bsonConfig.getField( OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
      if ( ele.eoo() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "the config property [%s] is missed",
                     OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
         goto error ;
      }

      if ( Array != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "the config property [%s] must be Array of String",
                     OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
         goto error ;
      }

      {
         BSONObjIterator it( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            string hostName ;
            BSONElement e = it.next();
            if ( String != e.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "the element of config property [%s] must be String",
                           OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
               goto error ;
            }

            hostName = e.String() ;
            if ( "" == hostName )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "the element of config property [%s] can't be empty",
                           OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
               goto error ;
            }

            if ( hostNames.end() != hostNames.find( hostName ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "duplicate host name [%s] of config property [%s]",
                           hostName.c_str(), OM_SSQL_OLAP_CONF_SEGMENT_HOSTS ) ;
               goto error ;
            }

            hostNames.insert( hostName ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::_rebuild( BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      BSONObj config ;
      BSONArrayBuilder newConfigArray ;
      string masterHostName ;
      string standbyHostName ;
      set<string> hostNames ;

      {
         BSONElement ele = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
         SDB_ASSERT( Array == ele.type(), "config is not array" ) ;

         BSONObjIterator it( ele.embeddedObject() ) ;
         SDB_ASSERT( it.more(), "no element in config" ) ;

         config = it.next().embeddedObject() ;
      }

      masterHostName = config.getStringField( OM_SSQL_OLAP_CONF_MASTER_HOST ) ;
      standbyHostName = config.getStringField( OM_SSQL_OLAP_CONF_STANDBY_HOST ) ;

      rc = _getSegmentHostNames( config, hostNames ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get segment host names from config" ) ;
         goto error ;
      }

      // master
      {
         BSONObjBuilder builder ;
         builder.append( OM_BSON_FIELD_HOST_NAME, masterHostName ) ;
         builder.append( OM_SSQL_OLAP_CONF_ROLE, OM_SSQL_OLAP_MASTER ) ;
         builder.appendElements( config ) ;

         newConfigArray.append( builder.obj() ) ;
      }

      // standby
      if ( "" != standbyHostName )
      {
         BSONObjBuilder builder ;
         builder.append( OM_BSON_FIELD_HOST_NAME, standbyHostName ) ;
         builder.append( OM_SSQL_OLAP_CONF_ROLE, OM_SSQL_OLAP_STANDBY ) ;
         builder.appendElements( config ) ;

         newConfigArray.append( builder.obj() ) ;
      }

      // segment
      for ( set<string>::iterator it = hostNames.begin() ; it != hostNames.end() ; it++ )
      {
         string segmentHostName = *it ;
         BSONObjBuilder builder ;
         builder.append( OM_BSON_FIELD_HOST_NAME, segmentHostName ) ;
         builder.append( OM_SSQL_OLAP_CONF_ROLE, OM_SSQL_OLAP_SEGMENT ) ;
         if ( segmentHostName != masterHostName && segmentHostName != standbyHostName )
         {
            builder.append( OM_SSQL_OLAP_CONF_IS_SINGLE, "true" ) ;
         }
         else
         {
            builder.append( OM_SSQL_OLAP_CONF_IS_SINGLE, "false" ) ;
         }
         builder.appendElements( config ) ;

         newConfigArray.append( builder.obj() ) ;
      }

      // new config
      {
         BSONObjBuilder builder ;
         BSONObjIterator it( bsonConfig ) ;
         
         while ( it.more() )
         {
            BSONElement ele = it.next() ;

            if ( string( ele.fieldName() ) == OM_BSON_FIELD_CONFIG )
            {
               BSONArray array = newConfigArray.arr() ;
               builder.append( OM_BSON_FIELD_CONFIG, array ) ;
            }
            else
            {
               builder.append( ele ) ;
            }
         }

         bsonConfig = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmSsqlOlapConfigBuilder::getHostNames( const BSONObj& bsonConfig, set<string>& hostNames )
   {
      INT32 rc = SDB_OK ;
      BSONObj config ;
      BSONElement configEle ;
      string masterHostName ;
      string standbyHostName ;

      configEle = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array: field=%s, "
                     "type=%d", OM_BSON_FIELD_CONFIG, configEle.type() ) ;
         goto error ;
      }

      {
         BSONObjIterator it( configEle.embeddedObject() ) ;
         BSONElement ele ;
         if ( !it.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "no value in config field" ) ;
            goto error ;
         }

         ele = it.next() ;
         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "the element of config is not Object: type=%d",
                        ele.type() ) ;
            goto error ;
         }

         config = ele.embeddedObject() ;

         if ( it.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "too many values in config" ) ;
            goto error ;
         }
      }

      rc = _getSegmentHostNames( config, hostNames ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get segment host names from config" ) ;
         goto error ;
      }

      masterHostName = config.getStringField( OM_SSQL_OLAP_CONF_MASTER_HOST ) ;
      if ( "" == masterHostName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "the host name of master node is empty" ) ;
         goto error ;
      }
      hostNames.insert( masterHostName ) ;

      standbyHostName = config.getStringField( OM_SSQL_OLAP_CONF_STANDBY_HOST ) ;
      if ( "" != standbyHostName )
      {
         hostNames.insert( standbyHostName ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
