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

   Source File Name = omConfigMySQL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/13/2018  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_MYSQL_HPP_
#define OM_CONFIG_MYSQL_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmMySQLConfigBuilder ;

   class OmMySQLNode: public OmNode
   {
   public:
      OmMySQLNode() ;
      virtual ~OmMySQLNode() ;

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

      friend class OmMySQLConfigBuilder ;
   } ;

   class OmMySQLConfTemplate : public OmConfTemplate
   {
   public:
      OmMySQLConfTemplate() ;
      virtual ~OmMySQLConfTemplate() ;
   } ;

   class OmMySQLConfProperties : public OmConfProperties
   {
   public:
      OmMySQLConfProperties() ;
      virtual ~OmMySQLConfProperties() ;

   public:
      bool isPrivateProperty( const string& name ) const ;
      bool isAllPropertySet() ;
   } ;

   class OmMySQLConfigBuilder: public OmConfigBuilder
   {
   public:
      OmMySQLConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmMySQLConfigBuilder() ;

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
      string                  _localHostName ;
      string                  _defaultServicePort ;
      OmMySQLConfTemplate     _template ;
      OmMySQLConfProperties   _properties ;

   } ;
}

#endif /* OM_CONFIG_SDB_HPP_ */
