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

   Source File Name = omConfigPostgreSQL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_POSTGRESQL_HPP_
#define OM_CONFIG_POSTGRESQL_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmPostgreSQLConfigBuilder ;

   class OmPostgreSQLNode: public OmNode
   {
   public:
      OmPostgreSQLNode() ;
      virtual ~OmPostgreSQLNode() ;

   public:
      const string& getDBPath() const { return _dbPath ; }
      const string& getServiceName() const { return _serviceName ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      INT32 _setServiceName( const string& serviceName ) ;
      INT32 _setDBPath( const string& dbPath, OmHost& host ) ;

   private:
      string _dbPath ;
      string _serviceName ;

      friend class OmPostgreSQLConfigBuilder ;
   } ;

   class OmPostgreSQLConfTemplate : public OmConfTemplate
   {
   public:
      OmPostgreSQLConfTemplate() ;
      virtual ~OmPostgreSQLConfTemplate() ;
   } ;

   class OmPostgreSQLConfProperties : public OmConfProperties
   {
   public:
      OmPostgreSQLConfProperties() ;
      virtual ~OmPostgreSQLConfProperties() ;

   public:
      bool isPrivateProperty( const string& name ) const ;
      bool isAllPropertySet() ;
   } ;

   class OmPostgreSQLConfigBuilder: public OmConfigBuilder
   {
   public:
      OmPostgreSQLConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmPostgreSQLConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _build( BSONArray &nodeConfig ) ;
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkAndAddNode( const BSONObj& bsonNode ) ;
      bool  _isServiceNameUsed( const OmHost& host, const string& serviceName ) ;
      void  _setLocal() ;
      INT32 _getServiceName( const OmHost& host, string& serviceName ) ;
      INT32 _getDBPath( OmHost& host, const string& diskPath,
                        const string& businessType, const string& serviceName,
                        string& dbPath ) ;
      INT32 _createNode() ;

   private:
      string                   _localHostName ;
      string                   _defaultServicePort ;
      OmPostgreSQLConfTemplate _template ;
      OmPostgreSQLConfProperties _properties ;

   } ;
}

#endif /* OM_CONFIG_SDB_HPP_ */
