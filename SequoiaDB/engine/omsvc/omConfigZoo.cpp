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

   Source File Name = omConfigZoo.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigZoo.hpp"
#include "omDef.hpp"
#include <sstream>

namespace engine
{
   #define OM_ZOO_PORT_NUM (3)
   #define OM_ZOO_PORT_STEP OM_ZOO_PORT_NUM

   OmZooNode::OmZooNode()
   {
   }

   OmZooNode::~OmZooNode()
   {
   }

   INT32 OmZooNode::_init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster )
   {
      INT32 rc = SDB_OK ;

      string installPath  = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_INSTALLPATH ) ;
      string zooId = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_ZOOID ) ;
      string dataPath = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_DATAPATH ) ;
      string dataPort = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_DATAPORT ) ;
      string electPort = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_ELECTPORT) ;
      string clientPort = bsonNode.getStringField( OM_ZOO_CONF_DETAIL_CLIENTPORT ) ;

      rc = _setInstallPath( installPath, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add install path to node" );
         goto error ;
      }

      _setZooId( zooId ) ;

      rc = _setDataPath( dataPath, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add data path to node" );
         goto error ;
      }

      rc = _setDataPort( dataPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add data port to node" );
         goto error ;
      }

      rc = _setElectPort( electPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add elect port to node" );
         goto error ;
      }

      rc = _setClientPort( clientPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add client port to node" );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooNode::_setInstallPath( const string& path, OmHost& host )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( path, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add install path to node" );
         goto error ;
      }

      _installPath = path ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void  OmZooNode::_setZooId( const string& zooId )
   {
      _zooId = zooId ;
   }

   INT32 OmZooNode::_setDataPath( const string& path, OmHost& host )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( path, host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add data path to node" );
         goto error ;
      }

      _dataPath = path ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooNode::_setDataPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add data port to node" );
         goto error ;
      }

      _dataPort = port ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooNode::_setElectPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add elect port to node" );
         goto error ;
      }

      _electPort = port ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooNode::_setClientPort( const string& port )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( port ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add client port to node" );
         goto error ;
      }

      _clientPort = port ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OmZooConfTemplate::OmZooConfTemplate()
                     : _zooNum( -1 )
   {
   }

   OmZooConfTemplate::~OmZooConfTemplate()
   {
      reset() ;
   }

   void OmZooConfTemplate::_reset()
   {
      _zooNum = -1 ;
   }

   bool OmZooConfTemplate::_isAllProperySet()
   {
      if ( _zooNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_ZOO_NUM ) ;
         return false ;
      }

      return true ;
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"zookeeper", "BusinessName":"myzookeeper",
      "DeployMod": "zookeeper", 
      "Property":[{"Name":"zoonum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 OmZooConfTemplate::_setPropery( const string& name, const string& value )
   {
      if ( name.compare( OM_TEMPLATE_ZOO_NUM ) == 0 )
      {
         _zooNum = ossAtoi( value.c_str() ) ;
      }

      return SDB_OK ;
   }

   OmZooConfProperties::OmZooConfProperties()                  
   {
   }

   OmZooConfProperties::~OmZooConfProperties()
   {
   }

   bool OmZooConfProperties::isPrivateProperty( const string& name ) const
   {
      if ( OM_ZOO_CONF_DETAIL_ZOOID == name ||
           OM_ZOO_CONF_DETAIL_INSTALLPATH == name ||
           OM_ZOO_CONF_DETAIL_DATAPATH == name ||
           OM_ZOO_CONF_DETAIL_DATAPORT == name ||
           OM_ZOO_CONF_DETAIL_ELECTPORT == name ||
           OM_ZOO_CONF_DETAIL_CLIENTPORT == name )
      {
         return true ;
      }

      return false ;
   }

   bool OmZooConfProperties::isAllPropertySet()
   {
      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_INSTALLPATH ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_INSTALLPATH ) ;
         return false ;
      }

      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_DATAPATH ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_DATAPATH ) ;
         return false ;
      }

      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_ZOOID ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_ZOOID ) ;
         return false ;
      }

      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_DATAPORT ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_DATAPORT ) ;
         return false ;
      }

      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_ELECTPORT ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_ELECTPORT ) ;
         return false ;
      }

      if ( !isPropertySet( OM_ZOO_CONF_DETAIL_CLIENTPORT ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_CLIENTPORT ) ;
         return false ;
      }

      return true ;
   }

   /*
      get best host rule:
          rule1: the more the better which host contains unused disk's count
          rule2: the less the better which host contains node's count
   */
   class getBestHostForZoo
   {
   public:
      int operator() ( const OmHost* host1, const OmHost* host2 )
      {
         SDB_ASSERT( NULL != host1, "host1 can't be NULL" ) ;
         SDB_ASSERT( NULL != host2, "host2 can't be NULL" ) ;

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

   class getBestDiskForZoo
   {
   public:
      getBestDiskForZoo( const OmHost& host ):
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

   class byZooId
   {
   public:
      byZooId( const string& zooId ):
         _zooId( zooId )
      {
      }

      bool operator() ( const OmNode* node ) const
      {
         if ( OM_BUSINESS_ZOOKEEPER != node->getBusinessInfo().businessType )
         {
            return false ;
         }

         const OmZooNode* zooNode = dynamic_cast<const OmZooNode*>( node ) ;
         if ( _zooId == zooNode->getZooId() )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _zooId ;
   } ;

   OmZooConfigBuilder::OmZooConfigBuilder( const OmBusinessInfo& businessInfo ):
      OmConfigBuilder( businessInfo ), _zooId( 1 )
   {
   }

   OmZooConfigBuilder::~OmZooConfigBuilder()
   {
   }

   INT32 OmZooConfigBuilder::_build( BSONArray& nodeConfig )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _business, "_business can't be NULL" ) ;
      SDB_ASSERT( 0 == _business->nodeNum(), "_business already has nodes") ;

      for ( INT32 i = 0 ; i < _template.getZooNum() ; i++ )
      {
         rc = _createNode() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create node: rc=%d", rc ) ;
            goto error ;
         }
      }

      {
         BSONArrayBuilder arrBuilder ;
         const OmNodes& nodes = _business->getNodes() ;
         for ( OmNodes::ConstIterator it = nodes.begin() ;
               it != nodes.end() ; it++ )
         {
            OmZooNode* node = dynamic_cast<OmZooNode*>( *it ) ;
            BSONObjBuilder builder ;
            builder.append( OM_BSON_FIELD_HOST_NAME, node->getHostName() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_ZOOID, node->getZooId() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_INSTALLPATH, node->getInstallPath() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_DATAPATH, node->getDataPath() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_DATAPORT, node->getDataPort() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_ELECTPORT, node->getElectPort() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_CLIENTPORT, node->getClientPort() ) ;

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
         }

         nodeConfig = arrBuilder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooConfigBuilder::_createNode()
   {
      INT32 rc = SDB_OK ;
      OmZooNode* node = NULL ;
      OmHost* host = NULL ;
      const simpleDiskInfo* disk = NULL ;
      string dataPort ;
      string installPath ;
      string dataPath ;

      host = _cluster.chooseHost( getBestHostForZoo() ) ;
      if ( NULL == host || host->diskNum() == 0 )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no host or disk" ) ;
         goto error ;
      }

      disk = host->chooseDisk( getBestDiskForZoo( *host ) ) ;
      if ( NULL == disk )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no disk" ) ;
         goto error ;
      }

      rc = _getDataPort( *host, dataPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get data port" ) ;
         goto error ;
      }

      rc = _getInstallPath( *host, _businessInfo.businessType,
                            _businessInfo.businessName, dataPort, installPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get install path" ) ;
         goto error ;
      }

      rc = _getDataPath( *host, disk->mountPath, _businessInfo.businessType,
                         _businessInfo.businessName, dataPort, dataPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get data path" ) ;
         goto error ;
      }

      node = SDB_OSS_NEW OmZooNode() ;
      if ( NULL == node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create zookeeper node, out of memory" ) ;
         goto error ;
      }

      rc = node->_setInstallPath( installPath, *host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set install path" ) ;
         goto error ;
      }

      rc = node->_setDataPath( dataPath, *host ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set data path" ) ;
         goto error ;
      }

      rc = node->_setDataPort( dataPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set data port" ) ;
         goto error ;
      }

      rc = node->_setElectPort( strPlus( dataPort, 1) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set elect port" ) ;
         goto error ;
      }

      rc = node->_setClientPort( strPlus( dataPort, 2) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set client port" ) ;
         goto error ;
      }

      {
         stringstream ss ;
         ss << _zooId++ ;

         node->_setZooId( ss.str() ) ;
      }

      node->_setHostName( host->getHostName() ) ;
      node->_setBusinessInfo( _businessInfo ) ;

      rc = _cluster.addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add node to business: "
                 "businessType=%s, businessName=%s, rc=%d",
                 _businessInfo.businessType.c_str(), _businessInfo.businessName.c_str(),
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmZooConfigBuilder::_getDataPort( const OmHost& host, string& dataPort )
   {
      INT32 rc = SDB_OK ;
      string defaultDataPort ;

      defaultDataPort = _properties.getDefaultValue( OM_ZOO_CONF_DETAIL_DATAPORT ) ;

      INT32 startPort = ossAtoi( defaultDataPort.c_str() ) ;

      bool ok = false ;
      for ( ; startPort < OM_MAX_PORT ; startPort += OM_ZOO_PORT_STEP )
      {
         bool used = false ;
         for ( INT32 i = 0; i < OM_ZOO_PORT_NUM; i++ )
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
         dataPort = ss.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   bool OmZooConfigBuilder::_isDataPortUsed( const OmHost& host, string& dataPort )
   {
      INT32 startPort = ossAtoi( dataPort.c_str() ) ;

      for ( INT32 i = 0; i < OM_ZOO_PORT_NUM; i++ )
      {
         if ( host.isPortUsed( startPort + i ) )
         {
            return true ;
         }
      }

      return false ;
   }

   INT32 OmZooConfigBuilder::_getInstallPath( OmHost& host,
                           const string& businessType, const string& businessName,
                           const string& dataPort, string& installPath )
   {
      INT32 i = 0 ;
      INT32 rc = SDB_OK ;

      do 
      {
         stringstream ss ;
         ss << OM_DEFAULT_INSTALL_ROOT_PATH
            << businessType << OSS_FILE_SEP
            << businessName << OSS_FILE_SEP
            << dataPort ;
         if ( 0 != i )
         {
            ss << "_" << i ;
         }
         i++ ;
         installPath = ss.str() ;
      } while ( host.isPathUsed( installPath ) ) ;

      if ( installPath.length() > OM_PATH_LENGTH )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "the length of dbpath is too long, length = %d",
                     installPath.length() );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooConfigBuilder::_getDataPath( OmHost& host, const string& diskPath,
                           const string& businessType, const string& businessName,
                           const string& dataPort, string& dataPath )
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
            << businessName << OSS_FILE_SEP
            << OM_DBPATH_PREFIX_DATABASE << OSS_FILE_SEP
            << dataPort ;
         if ( 0 != i )
         {
            ss << "_" << i ;
         }
         i++ ;
         dataPath = ss.str() ;
      } while ( host.isPathUsed( dataPath ) ) ;

      if ( dataPath.length() > OM_PATH_LENGTH )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "the length of dbpath is too long, length = %d",
                     dataPath.length() );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooConfigBuilder::_check( BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      BSONElement configEle ;

      configEle = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array:field=%s, type=%d",
                     OM_BSON_FIELD_CONFIG, configEle.type() ) ;
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

      if ( 0 == _business->nodeNum() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "the node number is zero" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmZooConfigBuilder::_checkAndAddNode( const BSONObj& bsonNode )
   {
      INT32 rc = SDB_OK ;
      OmZooNode* node = NULL ;
      OmHost* host = NULL ;
      string hostName ;
      BSONObjIterator itemIter( bsonNode ) ;

      node = SDB_OSS_NEW OmZooNode() ;
      if ( NULL == node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create zoo node, out of memory" ) ;
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
         else 
         {
            rc = _properties.checkValue( fieldName, value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check value failed:name=%s,value=%s", 
                       fieldName.c_str(), value.c_str() ) ;
               goto error ;
            }

            if ( OM_ZOO_CONF_DETAIL_CLIENTPORT == fieldName )
            {
               if ( host->isPortUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "client port has been used: host=%s, client port=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setClientPort( value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set client port: host=%s, client port=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_ZOO_CONF_DETAIL_DATAPATH == fieldName )
            {
               if ( NULL == host->getDisk( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "dataPath's disk does not exist: hostName=%s, "
                              "dataPath=%s", hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               if ( host->isPathUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "data path has been used: host=%s, data path=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setDataPath( value, *host ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set data path: host=%s, data path=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_ZOO_CONF_DETAIL_DATAPORT == fieldName )
            {
               if ( host->isPortUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "data port has been used: host=%s, data port=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setDataPort( value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set data port: host=%s, data port=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_ZOO_CONF_DETAIL_ELECTPORT == fieldName )
            {
               if ( host->isPortUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "elect port has been used: host=%s, elect port=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setElectPort( value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set elect port: host=%s, elect port=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_ZOO_CONF_DETAIL_INSTALLPATH == fieldName )
            {
               if ( host->isPathUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "install path has been used: host=%s, install path=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setInstallPath( value, *host ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set install path: host=%s, install path=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_ZOO_CONF_DETAIL_ZOOID == fieldName )
            {
               if ( 0 != _business->count( byZooId( value ) ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "zooid is exist: businessName=%s, zooid=%s",
                              _businessInfo.businessName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               node->_setZooId( value ) ;
            }
         }
      }

      rc = _cluster.addNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add node to cluster: host=%s, zooId=%s", 
                 hostName.c_str(), node->getZooId().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }
}

