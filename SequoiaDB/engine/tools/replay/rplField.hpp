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

   Source File Name = rplFieldInfo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_FIELD_INFO_HPP_
#define REPLAY_FIELD_INFO_HPP_

#include "oss.hpp"
#include "rplOutputter.hpp"
#include <string>
#include <map>

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   enum EN_FieldType
   {
      MAPPING_STRING    = 0,       // mapping string
      MAPPING_INT       = 1,
      MAPPING_DECIMAL   = 2,
      MAPPING_LONG      = 3,
      MAPPING_TIMESTAMP = 4,

      CONST_STRING      = 100, // const string value
      OUTPUT_TIME       = 101,    // output time
      AUTO_OP           = 102,    //auto generate op str
      ORIGINAL_TIME     = 200  // replica log's original time
   } ;

   class rplField
   {
   private:
      BOOLEAN _needDoubleQuote ;

   public:
      rplField() : _needDoubleQuote( TRUE )
      {

      }

      virtual ~rplField() {} ;

   public:
      virtual INT32 init( const BSONObj &fieldConf ) = 0 ;
      virtual EN_FieldType getFieldType() const = 0 ;
      virtual INT32 getValue( const BSONObj &sRecord, string &value ) = 0 ;
      BOOLEAN isNeedDoubleQuote()
      {
         return _needDoubleQuote ;
      }

      void setIsNeedDoubleQuote( BOOLEAN needDoubleQuote )
      {
         _needDoubleQuote = needDoubleQuote ;
      }
   } ;

   const INT32 MAX_FIELDNAME_LEN = 1024 ;

   class rplConstStringField : public rplField, public SDBObject
   {
   public:
      rplConstStringField() ;
      ~rplConstStringField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 getValue( const BSONObj &sRecord, string &value ) ;

   private:
      string _value ;
   } ;

   class rplOutputTimeField : public rplField, public SDBObject
   {
   public:
      rplOutputTimeField() ;
      ~rplOutputTimeField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 getValue( const BSONObj &sRecord, string &value ) ;

   private:
      void getCurrentTimeStr( string &timeStr ) ;
   } ;

   class rplOriginalTimeField : public rplField, public SDBObject
   {
   public:
      rplOriginalTimeField() ;
      ~rplOriginalTimeField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 getValue( const BSONObj &sRecord, string &value ) ;

   public:
      static void getTimeStr( const UINT64 &microSeconds, CHAR *szTimeStr,
                              INT32 timeStrSize ) ;
   } ;

   class rplAutoOPField : public rplField, public SDBObject
   {
   public:
      rplAutoOPField() ;
      ~rplAutoOPField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 getValue( const BSONObj &sRecord, string &value ) ;
   } ;

   class rplMappingField : public rplField, public SDBObject
   {
   public:
      rplMappingField() ;
      virtual ~rplMappingField() ;

   public:
      virtual INT32 init( const BSONObj &fieldConf ) ;
      INT32 getValue( const BSONObj &sRecord, string &value ) ;

   protected:
      virtual INT32 _getValue( const BSONElement &ele, string &value ) = 0 ;

   protected:
      CHAR _sFieldName[ MAX_FIELDNAME_LEN + 1 ] ;
      CHAR _tFieldName[ MAX_FIELDNAME_LEN + 1 ] ;
      BOOLEAN _hasDefaultValue ;
      string _defaultValue ;
   } ;

   class rplMappingStrField : public rplMappingField
   {
   public:
      rplMappingStrField() ;
      virtual ~rplMappingStrField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 _getValue( const BSONElement &ele, string &value ) ;
   } ;

   class rplIntField : public rplMappingField
   {
   public:
      rplIntField() ;
      ~rplIntField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 _getValue( const BSONElement &ele, string &value ) ;
   } ;

   class rplLongField : public rplMappingField
   {
   public:
      rplLongField() ;
      ~rplLongField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 _getValue( const BSONElement &ele, string &value ) ;
   } ;

   class rplDecimalField : public rplMappingField
   {
   public:
      rplDecimalField() ;
      ~rplDecimalField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 _getValue( const BSONElement &ele, string &value ) ;
   } ;

   class rplTimestampField : public rplMappingField
   {
   public:
      rplTimestampField() ;
      ~rplTimestampField() ;

   public:
      INT32 init( const BSONObj &fieldConf ) ;
      EN_FieldType getFieldType() const ;
      INT32 _getValue( const BSONElement &ele, string &value ) ;
   } ;
}

#endif // REPLAY_FIELD_INFO_HPP_

