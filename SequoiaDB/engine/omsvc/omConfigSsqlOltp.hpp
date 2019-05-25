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

   Source File Name = omConfigSsqlOltp.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_SSQL_OLTP_HPP_
#define OM_CONFIG_SSQL_OLTP_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmSsqlOltpConfigBuilder ;

   class OmSsqlOltpNode: public OmNode
   {
   public:
      OmSsqlOltpNode() ;
      virtual ~OmSsqlOltpNode() ;

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

      friend class OmSsqlOltpConfigBuilder ;
   } ;

   class OmSsqlOltpConfTemplate : public OmConfTemplate
   {
   public:
      OmSsqlOltpConfTemplate() ;
      virtual ~OmSsqlOltpConfTemplate() ;

   private:
   } ;

   class OmSsqlOltpConfProperties : public OmConfProperties
   {
   public:
      OmSsqlOltpConfProperties() ;
      virtual ~OmSsqlOltpConfProperties() ;

   public:
      bool isPrivateProperty( const string& name ) const ;
      bool isAllPropertySet() ;
   } ;

   class OmSsqlOltpConfigBuilder: public OmConfigBuilder
   {
   public:
      OmSsqlOltpConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmSsqlOltpConfigBuilder() ;

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
      OmSsqlOltpConfTemplate   _template ;
      OmSsqlOltpConfProperties _properties ;

   } ;
}

#endif /* OM_CONFIG_SDB_HPP_ */
