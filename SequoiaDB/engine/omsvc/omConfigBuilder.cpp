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

   Source File Name = omConfigBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigBuilder.hpp"
#include "omConfigSdb.hpp"
#include "omConfigZoo.hpp"
#include "omConfigPostgreSQL.hpp"
#include "omConfigMySQL.hpp"
#include "omConfigSsqlOlap.hpp"
#include "omDef.hpp"
#include "ossSocket.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include <sstream>
#include <algorithm>

namespace engine
{
   #define OM_CONF_VALUE_INT_TYPE         "int"
   #define OM_CONF_VALUE_DOUBLE_TYPE      "double"
   #define OM_INT32_MAXVALUE_STR          "2147483647"
   #define OM_GENERATOR_DOT               ","
   #define OM_GENERATOR_LINE              "-"

   string strPlus( const string &addend, INT32 augend )
   {
      INT32 total = ossAtoi( addend.c_str() ) + augend ;
      CHAR result[ OM_INT32_LENGTH + 1 ] ;
      ossItoa( total, result, OM_INT32_LENGTH ) ;

      return string( result ) ;
   }

   string strConnect( const string &left, INT32 right )
   {
      CHAR result[ OM_INT32_LENGTH + 1 ] ;
      ossItoa( right, result, OM_INT32_LENGTH ) ;

      return ( left + result ) ;
   }

   string strLower( const string& str )
   {
      string lower = str ;
      transform( lower.begin(), lower.end(), lower.begin(), ::tolower ) ;
      return lower ;
   }

   string trimLeft( string &str, const string &trimer )
   {
      str.erase( 0, str.find_first_not_of( trimer ) ) ;
      return str ;
   }

   string trimRight( string &str, const string &trimer )
   {
      string::size_type pos = str.find_last_not_of( trimer ) ;
      if ( pos == string::npos )
      {
         str.erase( 0 ) ;
      }
      else
      {
         str.erase( pos + 1 ) ;
      }

      return str ;
   }

   static string trim( string &str )
   {
      trimLeft( str, " " ) ;
      trimRight( str, " " ) ;

      return str ;
   }

   INT32 getValueAsString( const BSONObj &bsonTemplate, 
                           const string &fieldName, string &value )
   {
      INT32 rc = SDB_OK ;
      BSONElement element = bsonTemplate.getField( fieldName ) ;
      if ( element.eoo() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( String == element.type() )
      {
         value = element.String() ;
      }
      else if ( NumberInt == element.type() )
      {
         CHAR tmp[20] ;
         ossSnprintf( tmp, sizeof(tmp), "%d", element.Int() ) ;
         value = string( tmp ) ;
      }
      else if ( NumberLong == element.type() )
      {
         CHAR tmp[40] ;
         ossSnprintf( tmp, sizeof(tmp), OSS_LL_PRINT_FORMAT, element.Long() ) ;
         value = string( tmp ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string omGetCurrentErrorInfo()
   {
      string errorInfo ;
      _pmdEDUCB* cb = pmdGetThreadEDUCB() ;
      const CHAR* info = cb->getInfo( EDU_INFO_ERROR ) ;
      if ( NULL != info )
      {
         errorInfo = info ;
      }

      return errorInfo ;
   }

   INT32 OmConfTemplate::init( const BSONObj &confTemplate )
   {
      INT32 rc = SDB_OK ;
      rc = getValueAsString( confTemplate, OM_BSON_BUSINESS_TYPE, 
                             _businessType ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_BUSINESS_TYPE ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_BUSINESS_NAME, 
                             _businessName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_BUSINESS_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_CLUSTER_NAME, 
                             _clusterName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_CLUSTER_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_DEPLOY_MOD, _deployMod ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_DEPLOY_MOD ) ;
         goto error ;
      }

      {
         BSONElement propertyElement ;
         propertyElement = confTemplate.getField( OM_BSON_PROPERTY_ARRAY ) ;
         if ( propertyElement.eoo() || Array != propertyElement.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "template's field is not Array:field=%s,"
                        "type=%d", OM_BSON_PROPERTY_ARRAY, 
                        propertyElement.type() ) ;
            goto error ;
         }

         BSONObjIterator i( propertyElement.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( ele.type() == Object )
            {
               BSONObj oneProperty = ele.embeddedObject() ;
               string itemName ;
               string itemValue ;

               rc = getValueAsString( oneProperty, OM_BSON_PROPERTY_NAME, itemName ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                              OM_BSON_PROPERTY_NAME ) ;
                  goto error ;
               }

               rc = getValueAsString( oneProperty, OM_BSON_PROPERTY_VALUE, 
                                      itemValue ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                              OM_BSON_PROPERTY_VALUE ) ;
                  goto error ;
               }

               {
                  OmConfProperty confProperty ;
                  rc = confProperty.init( oneProperty ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "init property failed:rc=%d", rc ) ;
                     goto error ;
                  }

                  if ( !confProperty.isValid( itemValue ) )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Template value is invalid:item=%s,value=%s,"
                                 "valid=%s", itemName.c_str(), itemValue.c_str(), 
                                 confProperty.getValidString().c_str() ) ;
                     goto error ;
                  }
               }

               rc = _setPropery( itemName, itemValue ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "_setPropery failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      if ( !_isAllProperySet() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "miss template configur item" ) ;
         goto error ;
      }

      rc = _init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   void OmConfTemplate::reset()
   {
      _businessType = "" ;
      _businessName = "" ;
      _clusterName  = "" ;
      _deployMod    = "" ;
      _reset() ;
   }

   INT32 OmConfTemplate::_init()
   {
      return SDB_OK ;
   }

   void OmConfTemplate::_reset()
   {
      return ;
   }

   bool OmConfTemplate::_isAllProperySet()
   {
      return true ;
   }

   INT32 OmConfTemplate::_setPropery( const string& name, const string& value )
   {
      return SDB_OK ;
   }

   OmRangeValidator::OmRangeValidator( const string &type, const CHAR *value )
   {
      _isClosed   = true ;
      _begin      = value ;
      _end        = value ;
      _isValidAll = false ;
      _type       = type ;

      trim( _begin ) ;
      trim( _end ) ;

      if ( ( _begin.length() == 0 ) && ( _end.length() == 0 ) )
      {
         /* if range is empty, all the value is valid */
         _isValidAll = true ;
      }
   }

   OmRangeValidator::OmRangeValidator( const string &type, const CHAR *begin, 
                                       const CHAR *end, bool isClosed )
   {
      _isClosed   = isClosed ;
      _begin      = begin ;
      _end        = end ;
      _isValidAll = false ;
      _type       = type ;

      trim( _begin ) ;
      trim( _end ) ;

      if ( ( _begin.length() == 0 ) && ( _end.length() == 0 ) )
      {
         /* if range is empty, all the value is valid */
         _isValidAll = true ;
      }

      if ( _type.compare( OM_CONF_VALUE_INT_TYPE ) == 0 )
      {
         if ( _end.length() == 0 )
         {
            _end = OM_INT32_MAXVALUE_STR ;
         }
      }
   }

   OmRangeValidator::~OmRangeValidator()
   {
   }

   bool OmRangeValidator::_isPureNumber( const char *value ) const
   {
      INT32 dotCount = 0 ;
      while ( NULL != value && *value != '\0' )
      {
         if ( *value >= '0' && *value <= '9' )
         {
            value++ ;
            continue ;
         }
         else if ( *value == '.' )
         {
            dotCount++ ;
            if ( dotCount <= 1 )
            {
               value++ ;
               continue ;
            }
         }

         return false ;
      }

      return true ;
   }

   bool OmRangeValidator::_isNumber( const char *value ) const
   {
      if ( *value == '+' || *value == '-' ) 
      {
         value++ ;
      }

      return _isPureNumber( value ) ;
   }

   bool OmRangeValidator::isValid( const string &value ) const
   {
      if ( OM_CONF_VALUE_INT_TYPE == _type ||
           OM_CONF_VALUE_DOUBLE_TYPE == _type )
      {
         if ( !_isNumber( value.c_str() ) )
         {
            return false ;
         }
      }

      if ( _isValidAll )
      {
         return true ;
      }

      INT32 compareEnd = _compare( value, _end ) ;
      if ( _isClosed )
      {
         if ( compareEnd == 0 )
         {
            return true ;
         }
      }

      INT32 compareBegin = _compare( value, _begin ) ;
      if ( compareBegin >= 0 && compareEnd < 0 )
      {
         return true ;
      }

      return false ;
   }

   INT32 OmRangeValidator::_compare( string left, string right ) const
   {
      if ( _type == OM_CONF_VALUE_INT_TYPE )
      {
         INT32 leftInt  = ossAtoi( left.c_str() ) ;
         INT32 rightInt = ossAtoi( right.c_str() ) ;

         return ( leftInt - rightInt ) ;
      }
      else if ( OM_CONF_VALUE_DOUBLE_TYPE == _type )
      {
         INT32 leftDouble  = ossAtof( left.c_str() ) ;
         INT32 rightDouble = ossAtof( right.c_str() ) ;

         return ( leftDouble - rightDouble ) ;
      }

      return left.compare( right ) ;
   }

   OmConfValidator::OmConfValidator()
   {
   }

   OmConfValidator::~OmConfValidator()
   {
      _clear() ;
   }

   OmRangeValidator *OmConfValidator::_createRangeValidator( const string &value )
   {
      OmRangeValidator *rv       = NULL ;
      string::size_type posTmp = value.find( OM_GENERATOR_LINE ) ;
      if( string::npos != posTmp )
      {
         string::size_type posTmp2 = value.find( OM_GENERATOR_LINE, posTmp+1 ) ;

         if( string::npos != posTmp2 )
         {
            rv = SDB_OSS_NEW OmRangeValidator( _type,
                                               value.substr(0,posTmp2).c_str(), 
                                               value.substr(posTmp2+1).c_str() ) ;
         }
         else
         {
            rv = SDB_OSS_NEW OmRangeValidator( _type,
                                               value.substr(0,posTmp).c_str(), 
                                               value.substr(posTmp+1).c_str() ) ;
         }
      }
      else
      {
         rv = SDB_OSS_NEW OmRangeValidator( _type, value.c_str() ) ;
      }

      return rv ;
   }

    INT32 OmConfValidator::init( const string &type, const string &validateStr )
   {
      _clear() ;

      string tmp ;
      INT32 rc = SDB_OK ;
      _type    = type ;

      OmRangeValidator *rv     = NULL ;
      string::size_type pos1 = 0 ;
      string::size_type pos2 = validateStr.find( OM_GENERATOR_DOT ) ;
      while( string::npos != pos2 )
      {
         rv = _createRangeValidator( validateStr.substr( pos1, pos2 - pos1 ) ) ;
         if ( NULL == rv )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _validatorList.push_back( rv ) ;

         pos1 = pos2 + 1 ;
         pos2 = validateStr.find( OM_GENERATOR_DOT, pos1 ) ;
      }

      tmp = validateStr.substr( pos1 ) ; 
      rv  = _createRangeValidator( tmp ) ;
      if ( NULL == rv )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      _validatorList.push_back( rv ) ;

   done:
      return rc ;
   error:
      _clear() ;
      goto done ;
   }

   bool OmConfValidator::isValid( const string &value ) const
   {
      ConstIterator iter = _validatorList.begin() ;
      while ( iter != _validatorList.end() )
      {
         if ( ( *iter )->isValid( value ) )
         {
            return true ;
         }

         iter++ ;
      }

      return false ;
   }

   void OmConfValidator::_clear()
   {
      Iterator iter = _validatorList.begin() ;
      while ( iter != _validatorList.end() )
      {
         OmRangeValidator *p = *iter ;
         SDB_OSS_DEL p ;
         iter++ ;
      }

      _validatorList.clear() ;
   }

   OmConfProperty::OmConfProperty()
   {
   }

   OmConfProperty::~OmConfProperty()
   {
   }

   /*
   propery:
      {
         "Name":"replicanum", "Type":"int", "Default":"1", "Valid":"1", 
         "Display":"edit box", "Edit":"false", "Desc":"", "WebName":"" 
      }
   */
   INT32 OmConfProperty::init( const BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      rc = getValueAsString( property, OM_BSON_PROPERTY_TYPE, _type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_TYPE, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_NAME, _name ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_NAME, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_DEFAULT, 
                             _defaultValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_DEFAULT, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_VALID, _validateStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_VALID, rc ) ;
         goto error ;
      }

      rc = _confValidator.init( _type, _validateStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "init _confValidator failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   bool OmConfProperty::isValid( const string &value ) const
   {
      return _confValidator.isValid( value ) ;
   }

   OmConfProperties::OmConfProperties() : _force( FALSE )
   {
   }

   OmConfProperties::~OmConfProperties()
   {
      reset() ;
   }

   OmConfProperties::Iterator OmConfProperties::begin()
   {
      return _properties.begin() ;
   }

   OmConfProperties::Iterator OmConfProperties::end()
   {
      return _properties.end() ;
   }

   void OmConfProperties::reset()
   {
      Iterator it ;
      it = _properties.begin() ;
      while ( it != _properties.end() )
      {
         OmConfProperty *property = it->second ;
         SDB_OSS_DEL property ;
         _properties.erase( it++ ) ;
      }

      _reset() ;
   }

   void OmConfProperties::_reset()
   {
   }

   INT32 OmConfProperties::addProperty( const BSONObj &bsonProperty )
   {
      INT32 rc = SDB_OK ;
      OmConfProperty *property = NULL ;

      property = SDB_OSS_NEW OmConfProperty() ;
      if ( NULL == property )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to new confProperty, out of memory" ) ;
         goto error ;
      }

      rc = property->init( bsonProperty ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init confProperty failed:property=%s, rc=%d", 
                 bsonProperty.toString(false, true ).c_str(), rc ) ;
         goto error ;
      }

      {
         Iterator it ;
         it = _properties.find( property->getName() ) ;
         if ( it != _properties.end() )
         {
            OmConfProperty *tmp = it->second ;
            SDB_OSS_DEL tmp ;
            _properties.erase( it ) ;
         }
      }

      try
      {
         _properties[ property->getName() ] = property ;
      }
      catch ( exception& e )
      {
         PD_LOG_MSG(PDERROR, "unexpected error happened when add property: %s",
                e.what());
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( property ) ;
      goto done ;
   }

   OmConfProperty* OmConfProperties::getConfProperty( const string &name ) const
   {
      OmConfProperty* property = NULL ;
      ConstIterator it ;

      it = _properties.find( name ) ;
      if ( it != _properties.end() )
      {
         property = it->second ;
      }

      return property ;
   }

   bool OmConfProperties::isPropertySet( const string &name ) const
   {
      return NULL != getConfProperty( name ) ;
   }

   INT32 OmConfProperties::checkValue( const string &name, const string &value )
   {
      INT32 rc = SDB_OK ;
      OmConfProperty *property ;

      property = getConfProperty( name ) ;
      if ( NULL == property )
      {
         if( FALSE == _force )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            PD_LOG_MSG( PDERROR, "can't find the property:name=%s", 
                        name.c_str() ) ;
            goto error ;
         }
         else
         {
            PD_LOG_MSG( PDWARNING, "can't find the property:name=%s", 
                        name.c_str() ) ;
            goto done ;
         }
      }

      if ( !property->isValid( value ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "property's value is invalid:name=%s,value=%s,"
                     "valid=%s", name.c_str(), value.c_str(), 
                     property->getValidString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void OmConfProperties::setForce()
   {
      _force = TRUE ;
   }

   string OmConfProperties::getDefaultValue( const string &name )
   {
      OmConfProperty* property = getConfProperty( name ) ;
      if ( NULL != property )
      {
         return property->getDefaultValue() ;
      }

      return "" ;
   }
   
   INT32 OmConfigBuilder::createInstance( const OmBusinessInfo& businessInfo,
                                          string &operationType,
                                          OmConfigBuilder*& builder)
   {
      INT32 rc = SDB_OK ;
      OmConfigBuilder* _builder = NULL ;

      if ( OM_BUSINESS_SEQUOIADB == businessInfo.businessType )
      {
         if( OM_FIELD_OPERATION_DEPLOY == operationType )
         {
            _builder = SDB_OSS_NEW OmSdbConfigBuilder( businessInfo ) ;
         }
         else if( OM_FIELD_OPERATION_EXTEND == operationType )
         {
            _builder = SDB_OSS_NEW OmExtendSdbConfigBuilder( businessInfo ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "invalid deploy mode: %s",
                        businessInfo.deployMode.c_str() ) ;
            goto error ;
         }
      }
      else if ( OM_BUSINESS_SEQUOIASQL_POSTGRESQL == businessInfo.businessType )
      {
         _builder = SDB_OSS_NEW OmPostgreSQLConfigBuilder( businessInfo ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_MYSQL == businessInfo.businessType )
      {
         _builder = SDB_OSS_NEW OmMySQLConfigBuilder( businessInfo ) ;
      }
      else if ( OM_BUSINESS_SEQUOIASQL_OLAP == businessInfo.businessType )
      {
         _builder = SDB_OSS_NEW OmSsqlOlapConfigBuilder( businessInfo ) ;
      }
      else if ( OM_BUSINESS_ZOOKEEPER == businessInfo.businessType )
      {
         _builder = SDB_OSS_NEW OmZooConfigBuilder( businessInfo ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid business type: %s",
                     businessInfo.businessType.c_str() ) ;
         goto error ;
      }

      if ( NULL == _builder )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "failed to create config builder, out of memory: "
                              "businessType=%s",
                     businessInfo.businessType.c_str() ) ;
         goto error ;
      }

      builder = _builder ;
      _builder->setOperationType( operationType ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"zookeeper", "BusinessName":"z1",
      "DeployMod": "distribution", 
      "Property":[{"Name":"zoonum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                 ] 
   }
   confProperties:
   {
      "Property":[{"Name":"installpath", "Type":"path", "Default":"/opt/zookeeper", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
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
   bsonBusinessInfo:
   {
      "BusinessInfo":[
                        {
                           "ClusterName": "c1",
                           "BusinessType": "sequoiadb",
                           "BusinessName": "b1",
                           "DeployMod":"distribution"
                        }
                        ,...
                     ]
   }
   */
   INT32 OmConfigBuilder::generateConfig( const BSONObj &bsonTemplate, 
                                          const BSONObj &confProperties, 
                                          const BSONObj &bsonHostInfo,
                                          const BSONObj &bsonBusinessInfo,
                                          const set<string>& hostNames,
                                          BSONObj &bsonConfig )
   {
      INT32 rc = SDB_OK ;
      OmBusiness* business = NULL ;
      BSONObj bsonBuildHost ;

      _bsonHostInfo = bsonHostInfo.copy() ;
      rc = _filterGenerateHost( bsonHostInfo, hostNames, bsonBuildHost ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "init cluster failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _cluster.init( bsonBusinessInfo, bsonBuildHost ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init cluster failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         OmConfTemplate& confTemplate = _getConfTemplate() ;
         rc = confTemplate.init( bsonTemplate ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "init template failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      if( _operationType == OM_FIELD_OPERATION_DEPLOY )
      {
         //delete in ~Omcluster
         business = SDB_OSS_NEW OmBusiness( _businessInfo ) ;
         if ( NULL == business )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR,
                        "failed to alloc new OmBusiness: %s, out of memory",
                        _businessInfo.businessName.c_str(), rc ) ;
            goto error ;
         }

         rc = _cluster.addBusiness( business ) ;
         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( business ) ;
            PD_LOG( PDERROR, "failed to add business [%s] to cluster: rc=%d",
                    _businessInfo.businessName.c_str(), rc ) ;
            goto error ;
         }
      }
      else if( _operationType == OM_FIELD_OPERATION_EXTEND )
      {
         rc = _cluster.getBusiness( _businessInfo.businessName, business ) ;
         if( rc )
         {
            PD_LOG_MSG( PDERROR, "business does not exists: %s",
                        _businessInfo.businessName.c_str() );
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid operation type: %s",
                     _operationType.c_str() ) ;
         goto error ;
      }

      _business = business ;

      {
         BSONArray nodeConfig ;
         BSONObjBuilder confBuilder ;

         rc = _build( nodeConfig ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build config: rc=%d", rc ) ;
            goto error ;
         }

         confBuilder.append( OM_BSON_FIELD_CONFIG, nodeConfig ) ;
         confBuilder.append( OM_BSON_BUSINESS_NAME, _businessInfo.businessName ) ;
         confBuilder.append( OM_BSON_BUSINESS_TYPE, _businessInfo.businessType ) ;
         confBuilder.append( OM_BSON_DEPLOY_MOD, _businessInfo.deployMode ) ;
         confBuilder.append( OM_BSON_CLUSTER_NAME, _businessInfo.clusterName ) ;
         bsonConfig = confBuilder.obj() ;
      }
      
   done:
      return rc ;
   error:
      _errorDetail = omGetCurrentErrorInfo() ;
      goto done ;
   }

   /*
   confProperties:
   {
      "Property":[{"Name":"dbpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 OmConfigBuilder::_parseProperties( const BSONObj &confProperties )
   {
      INT32 rc = SDB_OK ;
      BSONElement propertyEle ;
      OmConfProperties& properties = _getConfProperties() ;

      propertyEle = confProperties.getField( OM_BSON_PROPERTY_ARRAY ) ;
      if ( propertyEle.eoo() || Array != propertyEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "confProperties's field is not Array:field=%s,"
                     "type=%d", OM_BSON_PROPERTY_ARRAY, propertyEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator i( propertyEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BOOLEAN isHidden = false ;
               BSONObj oneProperty = ele.embeddedObject() ;
               string hidden = oneProperty.getStringField(
                                                   OM_BSON_PROPERTY_HIDDEN ) ;

               ossStrToBoolean( hidden.c_str(), &isHidden ) ;

               //filtering hidden config item
               if ( isHidden )
               {
                  continue ;
               }

               rc = properties.addProperty( oneProperty ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "addProperty failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }

         if ( !properties.isAllPropertySet() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "miss property configure" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      bsonHostInfo(in):
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

      bsonBuildHost(out)
   */
   INT32 OmConfigBuilder::_filterGenerateHost( const BSONObj& bsonHostInfo,
                                               const set<string>& hostNames,
                                               BSONObj& bsonBuildHost )
   {
      INT32 rc = SDB_OK ;

      if( hostNames.empty() )
      {
         bsonBuildHost = bsonHostInfo.copy() ;
      }
      else
      {
         BSONObj info ;
         BSONObjBuilder bsonBuild ;
         BSONArrayBuilder buildArray ;
         set<string>::iterator hostNameIter ;

         info = bsonHostInfo.getObjectField( OM_BSON_FIELD_HOST_INFO ) ;

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
            hostNameIter = hostNames.find( ele.embeddedObject()
                                   .getStringField( OM_BSON_FIELD_HOST_NAME ) ) ;
            if( hostNameIter != hostNames.end() )
            {
               buildArray.append( ele ) ;
            }
         }
         bsonBuild.append( OM_BSON_FIELD_HOST_INFO, buildArray.arr() ) ;
         bsonBuildHost = bsonBuild.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   /*
   newBusinessConfig:
   {
      "BusinessType":"sequoiadb", "BusinessName":"b1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "datagroupname": "", 
          "dbpath": "/home/db2/standalone/11830", "svcname": "11830", ...}
         ,...
      ]
   }
   confProperties:
   {
      "Property":[{"Name":"dbpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
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
   bsonBusinessInfo:
   {
      "BusinessInfo":[
                        {
                           "ClusterName": "c1",
                           "BusinessType": "sequoiadb",
                           "BusinessName": "b1",
                           "DeployMod":"distribution"
                        }
                        ,...
                     ]
   }
   */
   INT32 OmConfigBuilder::checkConfig( const BSONObj &confProperties, 
                                       const BSONObj &bsonHostInfo,
                                       const BSONObj &bsonBusinessInfo,
                                       BSONObj &newBusinessConfig,
                                       BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      OmBusiness* business = NULL ;

      _force = force ;

      _bsonHostInfo = bsonHostInfo.copy() ;

      rc = _cluster.init( bsonBusinessInfo, bsonHostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init cluster failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      if( _operationType == OM_FIELD_OPERATION_DEPLOY )
      {
         rc = _cluster.getBusiness( _businessInfo.businessName, business) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "business[%s] already exists",
                    _businessInfo.businessName.c_str() ) ;
            goto error ;
         }

         business = SDB_OSS_NEW OmBusiness( _businessInfo ) ;
         if ( NULL == business )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "failed to alloc new OmBusiness: %s, "
                        "out of memory", _businessInfo.businessName.c_str() ) ;
            goto error ;
         }
         
         rc = _cluster.addBusiness( business ) ;
         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( business ) ;
            PD_LOG( PDERROR, "failed to add business [%s] to cluster: rc=%d",
                    _businessInfo.businessName.c_str(), rc ) ;
            goto error ;
         }

      }
      else if( _operationType == OM_FIELD_OPERATION_EXTEND )
      {
         rc = _cluster.getBusiness( _businessInfo.businessName, business ) ;
         if( rc )
         {
            PD_LOG_MSG( PDERROR, "business does not exists: %s",
                        _businessInfo.businessName.c_str() );
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "invalid operation type: %s",
                     _operationType.c_str() ) ;
         goto error ;
      }

      _business = business ;

      rc = _check( newBusinessConfig ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check config: rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _errorDetail = omGetCurrentErrorInfo() ;
      goto done ;
   }

   INT32 OmConfigBuilder::getHostNames( const BSONObj& bsonConfig,
                                        const CHAR *pFieldName,
                                        set<string>& hostNames )
   {
      INT32 rc = SDB_OK ;
      BSONObj config ;

      config = bsonConfig.getObjectField( pFieldName ) ;

      {
         BSONObjIterator iter( config ) ;
         while ( iter.more() )
         {
            BSONObj oneNode ;
            BSONElement ele = iter.next() ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "field's element is not Object:field=%s"
                           ",type=%d", OM_BSON_FIELD_CONFIG, ele.type() ) ;
               goto error ;
            }
            oneNode = ele.embeddedObject() ;
            hostNames.insert( oneNode.getStringField(
                                                   OM_BSON_FIELD_HOST_NAME ) ) ;
         }
      }

   done:
      return rc ;
   error:
      _errorDetail = omGetCurrentErrorInfo() ;
      goto done ;
   }
}

