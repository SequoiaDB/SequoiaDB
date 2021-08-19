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

   Source File Name = rplTableMapping.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplTableMapping.hpp"
#include "rplConfDef.hpp"
#include "../bson/bson.hpp"
#include "ossUtil.hpp"
#include <string>

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   rplFieldMapping::rplFieldMapping()
   {
   }

   rplFieldMapping::~rplFieldMapping()
   {
      FIELD_VECTOR::iterator iter = _fieldVector.begin() ;
      while ( iter != _fieldVector.end() )
      {
         SAFE_OSS_DELETE( *iter ) ;
         iter++ ;
      }

      _fieldVector.clear() ;
   }

   INT32 rplFieldMapping::init( const CHAR * sourceFullName,
                                const CHAR *targetFullName )
   {
      INT32 rc = SDB_OK ;
      CHAR* dotPos = NULL ;

      if ( NULL == sourceFullName || NULL == targetFullName )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Source name or target name is NULL , "
                      "rc = %d" ) ;
      }

      //TODO: ubuntu compile error because of redefine strchr in cstring
      dotPos = ossStrchr( ( CHAR *)sourceFullName, '.' ) ;
      if ( NULL == dotPos || dotPos == sourceFullName || dotPos + 1 == '\0' )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Source name(%s) is invalid, rc = %d",
                      sourceFullName, rc ) ;
      }

      if ( dotPos - sourceFullName > MAX_CS_NAME_LEN
           || ossStrlen( dotPos ) > (UINT32)MAX_CS_NAME_LEN )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Source name(%s) is too long, rc = %d",
                      sourceFullName, rc ) ;
      }

      ossMemset( _sourceCSName, 0, sizeof(_sourceCSName) ) ;
      ossStrncpy( _sourceCSName, sourceFullName, dotPos - sourceFullName ) ;

      ossMemset( _sourceCLName, 0, sizeof(_sourceCLName) ) ;
      ossStrcpy( _sourceCLName, ++dotPos ) ;

      //TODO: ubuntu compile error because of redefine strchr in cstring
      dotPos = ossStrchr( (CHAR *)targetFullName, '.' ) ;
      if ( NULL == dotPos || dotPos == targetFullName || dotPos + 1 == '\0' )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Target name(%s) is invalid, rc = %d",
                      targetFullName, rc ) ;
      }

      if ( dotPos - targetFullName > MAX_CS_NAME_LEN
           || ossStrlen( dotPos ) > (UINT32)MAX_CS_NAME_LEN )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Target name(%s) is too long, rc = %d",
                      targetFullName, rc ) ;
      }

      ossMemset( _targetDBName, 0, sizeof(_targetDBName) ) ;
      ossStrncpy( _targetDBName, targetFullName, dotPos - targetFullName ) ;

      ossMemset( _targetTableName, 0, sizeof(_targetTableName) ) ;
      ossStrcpy( _targetTableName, ++dotPos ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplFieldMapping::addField( const BSONObj &field )
   {
      INT32 rc = SDB_OK ;
      rplField *fieldInfo = NULL ;
      const CHAR *tFieldType = NULL ;
      BSONElement typeEle ;
      BSONElement doubleQuoteEle ;

      typeEle = field.getField( RPL_CONF_NAME_FIELD_TYPE ) ;
      if ( typeEle.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Field(%s) must be string, rc = %d",
                      RPL_CONF_NAME_FIELD_TYPE, rc ) ;
      }

      tFieldType = typeEle.valuestr() ;

      if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_MAPPING_STRING ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplMappingStrField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_MAPPING_INT ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplIntField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_MAPPING_LONG ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplLongField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_MAPPING_DECIMAL ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplDecimalField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_MAPPING_TIMESTAMP ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplTimestampField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_CONST_STRING ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplConstStringField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_OUTPUT_TIME ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplOutputTimeField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_ORIGINAL_TIME ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplOriginalTimeField() ;
      }
      else if ( ossStrcmp( tFieldType, RPL_FIELD_TYPE_AUTO_OP ) == 0 )
      {
         fieldInfo = SDB_OSS_NEW rplAutoOPField() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Type(%s) is invalid, rc = %d", tFieldType,
                      rc ) ;
      }

      if ( NULL == fieldInfo )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create field, rc = %d", rc ) ;
      }

      rc = fieldInfo->init( field ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init fieldInfo, rc = %d", rc ) ;

      doubleQuoteEle = field.getField( RPL_CONF_NMAE_FIELD_DOUBLEQUOTE ) ;
      if ( doubleQuoteEle.isBoolean() )
      {
         fieldInfo->setIsNeedDoubleQuote( doubleQuoteEle.booleanSafe() ) ;
      }

      _fieldVector.push_back( fieldInfo ) ;
      fieldInfo = NULL ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( fieldInfo ) ;
      goto done ;
   }

   FIELD_VECTOR *rplFieldMapping::getFieldVector()
   {
      return &_fieldVector ;
   }

   const CHAR* rplFieldMapping::getTargetDBName() const
   {
      return _targetDBName ;
   }

   const CHAR* rplFieldMapping::getTargetTableName() const
   {
      return _targetTableName ;
   }

   rplTableMapping::rplTableMapping()
   {
   }

   rplTableMapping::~rplTableMapping()
   {
      clear() ;
   }

   void rplTableMapping::clear()
   {
      TABLE_MAP::iterator iter = _tableMap.begin() ;
      while ( iter != _tableMap.end() )
      {
         SAFE_OSS_DELETE( iter->second ) ;
         iter++ ;
      }

      _tableMap.clear() ;
   }

   /*{ tables:
       [
         {
           source: "cs.cl",
           target: "dbname.tablename",
           fields:
           [
             {
               source: "field1",
               target: "column1",
               targetType: 0   // see EN_FieldType
             }
           ]
         }
       ]
     }
   */
   INT32 rplTableMapping::_init( const BSONObj &conf )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = conf.getField( RPL_CONF_NAME_TABLES ) ;
      if ( Array != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse conf(%s), "
                      "field(%s) is not Array, rc = %d",
                      conf.toString().c_str(), RPL_CONF_NAME_TABLES,  rc ) ;
      }

      {
         BSONObjIterator tableIter( ele.embeddedObject() ) ;
         while ( tableIter.more() )
         {
            BSONElement tEle = tableIter.next() ;
            if ( tEle.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse conf(%s), "
                            "element(%s) is not Object, rc = %d",
                            conf.toString().c_str(), RPL_CONF_NAME_TABLES,  rc ) ;
            }

            BSONObj table = tEle.embeddedObject() ;
            rc = _parseTable( table ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse table(%s), rc = %d",
                         table.toString().c_str(),  rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplTableMapping::_parseTable( const BSONObj &table )
   {
      INT32 rc = SDB_OK ;
      const CHAR *sourceFullName = NULL ;
      const CHAR *targetFullName = NULL ;
      rplFieldMapping *fieldMapping = NULL ;
      BSONElement fieldsEle ;

      sourceFullName = table.getStringField( RPL_CONF_NAME_SOURCE ) ;
      targetFullName = table.getStringField( RPL_CONF_NAME_TARGET ) ;
      TABLE_MAP::iterator iter = _tableMap.find( sourceFullName ) ;
      if ( iter != _tableMap.end() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Source name(%s) is duplicate, rc = %d",
                      sourceFullName, rc ) ;
      }

      fieldMapping = SDB_OSS_NEW rplFieldMapping() ;
      if ( NULL == fieldMapping )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate rplFieldMapping,"
                      " rc = %d", rc ) ;
      }

      rc = fieldMapping->init( sourceFullName, targetFullName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init fieldMapping, rc = %d", rc ) ;

      fieldsEle = table.getField( RPL_CONF_NAME_FIELDS ) ;
      if ( fieldsEle.type() != Array )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Field(%s) is not Array,rc = %d",
                      RPL_CONF_NAME_FIELDS, rc ) ;
      }

      {
         BSONObj fields = fieldsEle.embeddedObject() ;
         rc = _parseFields( fields, fieldMapping ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse fields(%s), rc = %d",
                      fields.toString().c_str(), rc ) ;
      }

      _tableMap[ sourceFullName ] = fieldMapping ;
      fieldMapping = NULL ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( fieldMapping ) ;
      goto done ;
   }

   /*
   fields:
           [
             {
               source: "field1",
               target: "column1",
               targetType: 0   // see EN_FieldType
             }
           ]
   */
   INT32 rplTableMapping::_parseFields( const BSONObj fields,
                                        rplFieldMapping *fieldMapping )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator fieldIter( fields ) ;
      while ( fieldIter.more() )
      {
         BSONElement fieldEle = fieldIter.next() ;
         if ( fieldEle.type() != Object )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Field is not Object, rc = %d", rc ) ;
         }

         BSONObj field = fieldEle.embeddedObject() ;
         rc = fieldMapping->addField( field ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add field, rc = %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplTableMapping::init( const BSONObj &conf )
   {
      INT32 rc = SDB_OK ;
      try
      {
         rc = _init( conf ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init table mapping(%s), rc = %d",
                      conf.toString().c_str(), rc ) ;
      }
      catch ( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init table mapping: %s",
                      e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   rplFieldMapping *rplTableMapping::getFieldMapping( const CHAR *clFullName )
   {
      TABLE_MAP::iterator iter = _tableMap.find( clFullName ) ;
      if ( iter != _tableMap.end() )
      {
         return iter->second ;
      }

      return NULL ;
   }
}


