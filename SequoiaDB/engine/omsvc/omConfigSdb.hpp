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

   Source File Name = omConfigSdb.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_SDB_HPP_
#define OM_CONFIG_SDB_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmSdbConfigBuilder ;

   class OmSdbNode: public OmNode
   {
   public:
      OmSdbNode() ;
      virtual ~OmSdbNode() ;

   public:
      const string& getDBPath() const { return _dbPath ; }
      const string& getServiceName() const { return _serviceName ; }
      const string& getRole() const { return _role ; }
      const string& getGroupName() const { return _groupName ; }
      const string& getTransaction() const { return _transaction ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      INT32 _setServiceName( const string& serviceName ) ;
      INT32 _setDBPath( const string& dbPath, OmHost& host ) ;
      void  _setRole( const string& role ) ;
      void  _setGroupName( const string& groupName ) ;
      void  _setTransaction( const string& transaction ) ;

   private:
      string _dbPath ;
      string _serviceName ;
      string _role ;
      string _groupName ;
      string _transaction ;

      friend class OmSdbConfigBuilder ;
   } ;

   class OmSdbConfTemplate : public OmConfTemplate
   {
      public:
         OmSdbConfTemplate() ;
         virtual ~OmSdbConfTemplate() ;

      public:
         INT32 getReplicaNum() const { return _replicaNum ; }
         INT32 getDataNum() const { return _dataNum ; }
         INT32 getDataGroupNum() const { return _dataGroupNum ; }
         INT32 getCatalogNum() const { return _catalogNum ; }
         INT32 getCoordNum() const { return _coordNum ; }
         void  setCoordNum( INT32 coordNum ) ;

      private:
         INT32 _init() ;
         void  _reset() ;
         bool  _isAllProperySet() ;
         INT32 _setPropery( const string& name, const string& value ) ;

      private:
         INT32 _replicaNum ;
         INT32 _dataNum ;
         INT32 _catalogNum ;
         INT32 _dataGroupNum ;
         INT32 _coordNum ;
   } ;

   class OmSdbConfProperties : public OmConfProperties
   {
      public:
         OmSdbConfProperties() ;
         virtual ~OmSdbConfProperties() ;

      public:
         bool           isPrivateProperty( const string& name ) const ;
         bool           isAllPropertySet() ;
   } ;

   class OmSdbConfigBuilder: public OmConfigBuilder
   {
   public:
      OmSdbConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmSdbConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      INT32 _build( BSONArray &nodeConfig ) ;
      INT32 _buildStandalone() ;
      INT32 _buildCluster() ;         
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkAndAddNode( const BSONObj& bsonNode ) ;
      bool  _isServiceNameUsed( const OmHost& host, const string& serviceName ) ;

   private:
      OmSdbConfTemplate    _template ;

   protected:
      string               _localHostName ;
      string               _localServicePort ;
      string               _defaultServicePort ;
      OmSdbConfProperties  _properties ;

   protected:
      void  _setLocal() ;
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _getServiceName( const OmHost& host, const string& role,
                             string& serviceName ) ;
      INT32 _getDBPath( OmHost& host, const string& diskPath,
                        const string& businessType, const string& role,
                        const string& serviceName, string& dbPath ) ;
      INT32 _createNode( const string& role, const string& groupName ) ;
   } ;

   class OmSdbConfExtendTemplate : public OmConfTemplate
   {
      public:
         OmSdbConfExtendTemplate() ;
         virtual ~OmSdbConfExtendTemplate() ;

      public:
         INT32 getReplicaNum() const { return _replicaNum ; }
         INT32 getDataNum() const { return _dataNum ; }
         INT32 getDataGroupNum() const { return _dataGroupNum ; }
         INT32 getCatalogNum() const { return _catalogNum ; }
         INT32 getCoordNum() const { return _coordNum ; }

      private:
         INT32 _init() ;
         void  _reset() ;
         bool  _isAllProperySet() ;
         INT32 _setPropery( const string& name, const string& value ) ;

      private:
         INT32 _replicaNum ;
         INT32 _dataNum ;
         INT32 _catalogNum ;
         INT32 _dataGroupNum ;
         INT32 _coordNum ;
   } ;

   class OmExtendSdbConfigBuilder : public OmSdbConfigBuilder
   {
   public:
      OmExtendSdbConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmExtendSdbConfigBuilder() ;

   private:
         OmConfTemplate& _getConfTemplate() { return _template ; }
         INT32 _build( BSONArray& nodeConfig ) ;
         INT32 _buildHorizontal( map<string, INT32>& groupMap ) ;
         INT32 _buildVertical( INT32 existCoord, INT32 existCatalog,
                               map<string, INT32>& dataGroup ) ;
         INT32 _getGroupInfo( map<string, INT32>& groupMap,
                              INT32& coordNum,
                              INT32& catalogNum ) ;

   private:
      OmSdbConfExtendTemplate _template ;
   } ;

}

#endif /* OM_CONFIG_SDB_HPP_ */
