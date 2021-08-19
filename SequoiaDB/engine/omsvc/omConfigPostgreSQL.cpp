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

   Source File Name = omConfigPostgreSQL.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigPostgreSQL.hpp"
#include "omDef.hpp"
#include <sstream>

namespace engine
{
   #define OM_SVCNAME_STEP (10)
   #define OM_SDB_PORT_NUM (1)

   OmPostgreSQLNode::OmPostgreSQLNode()
   {
   }

   OmPostgreSQLNode::~OmPostgreSQLNode()
   {
   }

   INT32 OmPostgreSQLNode::_init( const BSONObj& bsonNode,
                                OmHost& host, OmCluster& cluster )
   {
      INT32 rc = SDB_OK ;

      string dbPath = bsonNode.getStringField( OM_BSON_DBPATH ) ;
      string serviceName = bsonNode.getStringField( OM_BSON_PORT ) ;

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

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmPostgreSQLNode::_setServiceName( const string& serviceName )
   {
      INT32 rc = SDB_OK ;

      rc = _addUsedPort( serviceName ) ;
      if ( rc )
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

   INT32 OmPostgreSQLNode::_setDBPath( const string& dbPath, OmHost& host )
   {
      INT32 rc = SDB_OK ;

      rc = _addPath( dbPath, host, TRUE ) ;
      if ( rc )
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

   OmPostgreSQLConfTemplate::OmPostgreSQLConfTemplate()
   {
   }

   OmPostgreSQLConfTemplate::~OmPostgreSQLConfTemplate()
   {
      reset() ;
   }

   OmPostgreSQLConfProperties::OmPostgreSQLConfProperties()
   {
   }

   OmPostgreSQLConfProperties::~OmPostgreSQLConfProperties()
   {
   }

   bool OmPostgreSQLConfProperties::isPrivateProperty( const string& name ) const
   {
      if ( OM_BSON_DBPATH == name ||
           OM_BSON_PORT == name )
      {
         return true ;
      }

      return false ;
   }

   bool OmPostgreSQLConfProperties::isAllPropertySet()
   {
      if ( !isPropertySet( OM_BSON_DBPATH ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_BSON_DBPATH ) ;
         return false ;
      }

      if ( !isPropertySet( OM_BSON_PORT ) )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_BSON_PORT ) ;
         return false ;
      }

      return true ;
   }

   class bySsqlOltpBuz
   {
   public:
      bool operator() ( const OmNode *node ) const
      {
         if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL !=
                    node->getBusinessInfo().businessType )
         {
            return false ;
         }

         return true ;
      }
   } ;

   /*
      get best host rule:
          rule1: the less the better which host contains specify role's count
          rule2: the more the better which host contains unused disk's count
          rule3: the less the better which host contains node's count
                 ( all the roles )
   */
   class getBestHostForSsqlOltp
   {
   public:
      int operator() ( const OmHost *host1, const OmHost *host2 )
      {
         SDB_ASSERT( NULL != host1, "host1 can't be NULL" ) ;
         SDB_ASSERT( NULL != host2, "host2 can't be NULL" ) ;

         INT32 roleNum1 = host1->count( _bySsqlOltpBuz ) ;
         INT32 roleNum2 = host2->count( _bySsqlOltpBuz ) ;

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
      bySsqlOltpBuz _bySsqlOltpBuz ;
   } ;

   class getBestDiskForSsqlOltp
   {
   public:
      getBestDiskForSsqlOltp( const OmHost &host ) : _host ( host )
      {
      }

      int operator() ( const simpleDiskInfo *disk1,
                       const simpleDiskInfo *disk2 )
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

   OmPostgreSQLConfigBuilder::OmPostgreSQLConfigBuilder(
                                          const OmBusinessInfo& businessInfo )
                              : OmConfigBuilder( businessInfo )
   {
   }

   OmPostgreSQLConfigBuilder::~OmPostgreSQLConfigBuilder()
   {
   }

   INT32 OmPostgreSQLConfigBuilder::_build( BSONArray& nodeConfig )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _business, "_business can't be NULL" ) ;
      SDB_ASSERT( 0 == _business->nodeNum(), "_business already has nodes") ;

      _setLocal() ;

      rc = _createNode() ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to create node: "
                     "businessName=%s, businessType=%s, rc=%d",
                     _businessInfo.businessType.c_str(),
                     _businessInfo.businessName.c_str(), rc ) ;
         goto error ;
      }

      {
         BSONArrayBuilder arrBuilder ;
         const OmNodes& nodes = _business->getNodes() ;
         OmNodes::ConstIterator iter ;

         for ( iter = nodes.begin(); iter != nodes.end(); ++iter )
         {
            OmPostgreSQLNode* node = dynamic_cast<OmPostgreSQLNode*>( *iter ) ;
            BSONObjBuilder builder ;

            builder.append( OM_BSON_HOSTNAME, node->getHostName() ) ;
            builder.append( OM_BSON_DBPATH, node->getDBPath() ) ;
            builder.append( OM_BSON_PORT, node->getServiceName() ) ;

            // append public properties
            for ( OmConfProperties::ConstIterator it = _properties.begin() ;
                  it != _properties.end() ; ++it )
            {
               const OmConfProperty* property = it->second ;

               if ( !_properties.isPrivateProperty( property->getName() ) )
               {
                  builder.append( property->getName(),
                                  property->getDefaultValue() ) ;
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

   void OmPostgreSQLConfigBuilder::_setLocal()
   {
      CHAR local[ OSS_MAX_HOSTNAME + 1 ] = "" ;

      ossGetHostName( local, OSS_MAX_HOSTNAME ) ;

      _localHostName = string( local ) ;

      _defaultServicePort = _properties.getDefaultValue( OM_BSON_PORT ) ;
   }

   INT32 OmPostgreSQLConfigBuilder::_createNode()
   {
      INT32 rc = SDB_OK ;
      OmPostgreSQLNode* node = NULL ;
      OmHost* host = NULL ;
      const simpleDiskInfo* disk = NULL ;
      string serviceName ;
      string dbPath ;

      host = _cluster.chooseHost( getBestHostForSsqlOltp() ) ;
      if ( NULL == host || 0 == host->diskNum() )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no host or disk" ) ;
         goto error ;
      }

      disk = host->chooseDisk( getBestDiskForSsqlOltp( *host ) ) ;
      if ( NULL == disk )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "create node failed: no disk" ) ;
         goto error ;
      }

      rc = _getServiceName( *host, serviceName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get service port" ) ;
         goto error ;
      }

      rc = _getDBPath( *host, disk->mountPath, _businessInfo.businessType,
                       serviceName, dbPath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to get db path" ) ;
         goto error ;
      }

      node = SDB_OSS_NEW OmPostgreSQLNode() ;
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

      node->_setBusinessInfo( _businessInfo ) ;
      node->_setHostName( host->getHostName() ) ;

      rc = _cluster.addNode( node ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add node to business: "
                 "businessType=%s, businessName=%s, rc=%d",
                 _businessInfo.businessType.c_str(),
                 _businessInfo.businessName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   INT32 OmPostgreSQLConfigBuilder::_getServiceName( const OmHost& host,
                                                     string& serviceName )
   {
      INT32 rc = SDB_OK ;
      INT32 startPort = ossAtoi( _defaultServicePort.c_str() ) ;
      bool ok = false ;

      for ( ; startPort < OM_MAX_PORT ; startPort += OM_SVCNAME_STEP )
      {
         if ( !host.isPortUsed( startPort ) )
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

   #define OM_DBPATH_POSTGRESQL "postgresql"

   INT32 OmPostgreSQLConfigBuilder::_getDBPath( OmHost& host,
                                                const string& diskPath,
                                                const string& businessType,
                                                const string& serviceName,
                                                string& dbPath )
   {
      INT32 rc = SDB_OK ;
      INT32 i  = 0 ;
      BOOLEAN isRootPath = FALSE ;
      string deployPath ;

      if ( OSS_FILE_SEP == diskPath )
      {
         isRootPath = TRUE ;
         deployPath = host.getDeployPath( businessType ) ;
      }

      do 
      {
         stringstream ss ;

         if ( isRootPath && deployPath.length() > 0 )
         {
            ss << deployPath ;
            if ( OSS_FILE_SEP_CHAR != deployPath.at( deployPath.length() -1 ) )
            {
               ss << OSS_FILE_SEP ;
            }
         }
         else
         {
            ss << diskPath ;
            if ( OSS_FILE_SEP_CHAR != diskPath.at( diskPath.length() -1 ) )
            {
               ss << OSS_FILE_SEP ;
            }
         }

         if ( FALSE == isRootPath )
         {
            ss << OM_BUSINESS_SEQUOIASQL << OSS_FILE_SEP <<
                  OM_DBPATH_POSTGRESQL   << OSS_FILE_SEP ;
         }

         ss << OM_DBPATH_PREFIX_DATABASE << OSS_FILE_SEP <<
               serviceName ;
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

   INT32 OmPostgreSQLConfigBuilder::_check( BSONObj& bsonConfig )
   {
      INT32 rc = SDB_OK ;
      BSONElement configEle ;

      configEle = bsonConfig.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array:field=%s,"
                              "type=%d",
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
               if ( rc )
               {
                  PD_LOG( PDERROR, "check node config failed: rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      {
         const OmNodes& nodes = _business->getNodes() ;

         if ( nodes.nodeNum() != 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "can't install more than one node" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 OmPostgreSQLConfigBuilder::_checkAndAddNode( const BSONObj& bsonNode )
   {
      INT32 rc = SDB_OK ;
      OmPostgreSQLNode* node = NULL ;
      OmHost* host = NULL ;
      string hostName ;
      BSONObjIterator itemIter( bsonNode ) ;

      node = SDB_OSS_NEW OmPostgreSQLNode() ;
      if ( NULL == node )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create OmSdbNode, out of memory" ) ;
         goto error ;
      }

      node->_setBusinessInfo( _businessInfo ) ;

      hostName = bsonNode.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      rc = _cluster.getHost( hostName, host ) ;
      if ( rc )
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

         if ( OM_BSON_HOSTNAME == fieldName )
         {
            node->_setHostName( value ) ;
         }
         else
         {
            rc = _properties.checkValue( fieldName, value ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "check value failed:name=%s,value=%s", 
                       fieldName.c_str(), value.c_str() ) ;
               goto error ;
            }

            if ( OM_BSON_DBPATH == fieldName )
            {
               if ( host->isPathUsed( value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "db path has been used: host=%s, path=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setDBPath( value, *host ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "failed to set dbpath: host=%s, path=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
            else if ( OM_BSON_PORT == fieldName )
            {
               if ( _isServiceNameUsed( *host, value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "port has been used: host=%s, port=%s", 
                              hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }

               rc = node->_setServiceName( value ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to set port: host=%s, port=%s", 
                          hostName.c_str(), value.c_str() ) ;
                  goto error ;
               }
            }
         }
      }

      rc = _cluster.addNode( node ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to add node to cluster: host=%s, port=%s", 
                 hostName.c_str(), node->getServiceName().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( node ) ;
      goto done ;
   }

   bool OmPostgreSQLConfigBuilder::_isServiceNameUsed(
                                                    const OmHost& host,
                                                    const string& serviceName )
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

}
