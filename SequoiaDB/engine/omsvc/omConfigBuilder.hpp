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

   Source File Name = omConfigBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_BUILDER_
#define OM_CONFIG_BUILDER_

#include "omConfigModel.hpp"

namespace engine
{
   #define OM_MAX_PORT                    (65535)
   #define OM_PATH_LENGTH                 (256)

   string strPlus( const string &addend, INT32 augend ) ;
   string strConnect( const string &left, INT32 right ) ;
   string strLower( const string& str ) ;
   INT32 getValueAsString( const BSONObj &bsonTemplate, 
                           const string &fieldName, string &value ) ;

   class OmConfTemplate: public SDBObject
   {
   public:
      OmConfTemplate() {}
      virtual ~OmConfTemplate() {}

   public:
      INT32             init( const BSONObj &bsonTemplate ) ;
      void              reset() ;

      string            getBusinessType() { return _businessType ; }
      string            getBusinessName() { return _businessName ;}
      string            getClusterName() { return _clusterName ; }
      string            getDeployMod() { return _deployMod ; }

   protected:
      virtual INT32     _init() ;
      virtual void      _reset() ;
      virtual bool      _isAllProperySet() ;
      virtual INT32     _setPropery( const string& name, const string& value ) ;

   protected:
      string            _businessType ;
      string            _businessName ;
      string            _clusterName ;
      string            _deployMod ;
   } ;

   class OmRangeValidator : public SDBObject
   {
   public:
      OmRangeValidator( const string &type, const CHAR *value ) ;
      OmRangeValidator( const string &type, const CHAR *begin, const CHAR *end, 
                        bool isClosed = true ) ;
      ~OmRangeValidator() ;

   public:
      bool           isValid( const string &value ) const ;
      const string&  getType() const { return _type ; }

   private:
      INT32          _compare( string left, string right ) const ;
      bool           _isPureNumber( const char *value ) const ;
      bool           _isNumber( const char *value ) const ;
   private:
      string         _type ;
      bool           _isClosed ;
      bool           _isValidAll ;
      string         _begin ;
      string         _end ;
   } ;

   class OmConfValidator : public SDBObject
   {
   public:
      OmConfValidator() ;
      ~OmConfValidator() ;

   public:
      INT32          init( const string &type, const string &validateStr ) ;
      bool           isValid( const string &value ) const ;
      const string&  getType() const { return _type ; }

   private:
      void           _clear() ;
      OmRangeValidator* _createRangeValidator( const string& value ) ;

   private:
      string _type ;
      list<OmRangeValidator *> _validatorList ;
      typedef list<OmRangeValidator *>::const_iterator ConstIterator ;
      typedef list<OmRangeValidator *>::iterator Iterator ;
   } ;

   class OmConfProperty : public SDBObject
   {
   public:
      OmConfProperty() ;
      ~OmConfProperty() ;

   public:
      INT32          init( const BSONObj &property ) ;
      bool           isValid( const string &value ) const ;
      const string&  getDefaultValue() const { return _defaultValue ; }
      const string&  getName() const { return _name ; }
      const string&  getType() const { return _type ; }
      const string&  getValidString() const { return _validateStr ; }

   private:
      string            _name ;
      string            _type ;
      string            _defaultValue ;
      string            _validateStr ;
      OmConfValidator   _confValidator ;
   } ;

   class OmConfProperties: public SDBObject
   {
   public:
      OmConfProperties() ;
      virtual ~OmConfProperties() ;

   public:
      typedef map<string, OmConfProperty*>::const_iterator ConstIterator ;
      typedef map<string, OmConfProperty*>::iterator Iterator ;
      Iterator begin() ;
      Iterator end() ;

   public:
      void              reset() ;
      INT32             addProperty( const BSONObj& bsonProperty ) ;
      bool              isPropertySet( const string &name ) const ;
      OmConfProperty*   getConfProperty( const string &name ) const ;
      string            getDefaultValue( const string &name ) ;
      INT32             checkValue( const string &name, const string &value ) ;
      void              setForce() ;

   public:
      virtual bool      isPrivateProperty( const string& name ) const = 0 ;      
      virtual bool      isAllPropertySet() = 0 ;

   protected:
      virtual void      _reset() ;

   protected:
      //allow set configure not exist
      BOOLEAN _force ;
      map<string, OmConfProperty*> _properties ;
   } ;
   
   class OmConfigBuilder: public SDBObject
   {
   protected:
      OmConfigBuilder( const OmBusinessInfo& businessInfo ):
         _force( FALSE ), _businessInfo( businessInfo ), _business( NULL ) {}

   public:
      virtual ~OmConfigBuilder() {}
      static INT32 createInstance( const OmBusinessInfo& businessInfo,
                                   string &operationType,
                                   OmConfigBuilder*& builder ) ;

   public:
      void setOperationType( string& operationType )
      {
         _operationType = operationType ;
      }
      INT32 generateConfig( const BSONObj& bsonTemplate, 
                            const BSONObj& confProperties, 
                            const BSONObj& bsonHostInfo,
                            const BSONObj& bsonBusinessInfo,
                            const set<string>& hostNames,
                            BSONObj& bsonConfig ) ;

      INT32 checkConfig( const BSONObj& confProperties, 
                         const BSONObj& bsonHostInfo,
                         const BSONObj& bsonBusinessInfo,
                         BSONObj& newBusinessConfig,
                         BOOLEAN force = FALSE ) ;

      virtual INT32 getHostNames( const BSONObj& bsonConfig,
                                  const CHAR *pFieldName,
                                  set<string>& hostNames ) ;

      const string& getErrorDetail() const { return _errorDetail ; }

   private:
      INT32 _parseProperties( const BSONObj& confProperties ) ;
      INT32 _filterGenerateHost( const BSONObj& bsonHostInfo,
                                 const set<string>& hostNames,
                                 BSONObj& bsonBuildHost ) ;

   protected:
      virtual OmConfTemplate&    _getConfTemplate() = 0 ;
      virtual OmConfProperties&  _getConfProperties() = 0 ;
      virtual INT32 _build( BSONArray& bsonConfig ) = 0 ;
      virtual INT32 _check( BSONObj& bsonConfig ) = 0 ;

   protected:
      //allow set configure not exist
      BOOLEAN           _force ;
      //The config and node info for all hosts of the cluster
      BSONObj           _bsonHostInfo ;
      OmBusinessInfo    _businessInfo ;
      OmCluster         _cluster ;
      OmBusiness*       _business ;
      string            _operationType ;
      string            _errorDetail ;
   } ;
}

#endif /* OM_CONFIG_BUILDER_ */
