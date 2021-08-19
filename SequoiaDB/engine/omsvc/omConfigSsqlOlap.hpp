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

   Source File Name = omConfigZoo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          28/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_SSQL_OLAP_H_
#define OM_CONFIG_SSQL_OLAP_H_

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmSsqlOlapConfigBuilder ;

   class OmSsqlOlapNode: public OmNode
   {
   public:
      OmSsqlOlapNode() ;
      virtual ~OmSsqlOlapNode() ;

   public:
      const string& getRole() const { return _role ; }
      const string& getPort() const { return _port ; }
      const string& getDataDir() const { return _dataDir ; }
      const string& getTempDir() const { return _tempDir ; }
      const string& getInstallDir() const { return _installDir ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      void  _setRole( const string& role ) ;
      INT32 _setPort( const string& port ) ;
      INT32 _setDataDir( const string& dataDir, OmHost& host, bool ignoreDisk = false ) ;
      INT32 _setTempDir( const string& tempDir, OmHost& host, bool ignoreDisk = false ) ;
      INT32 _setInstallDir( const string& installDir, OmHost& host, bool ignoreDisk = false ) ;

   private:
      string _role ;
      string _port ;
      string _dataDir ;
      string _tempDir ;
      string _installDir ;

      friend class OmSsqlOlapConfigBuilder ;
   } ;

   class OmSsqlOlapConfTemplate: public OmConfTemplate
   {
   public:
      OmSsqlOlapConfTemplate() ;
      virtual ~OmSsqlOlapConfTemplate() ;

   public:
      bool  getDeployStandby() const { return _deployStandby ; }
      INT32 getSegmentNum() const { return _segmentNum ; }

   private:
      void  _reset() ;
      bool  _isAllProperySet() ;
      INT32 _setPropery( const string& name, const string& value ) ;

   private:
      bool  _deployStandby ;
      INT32 _segmentNum ;
   } ;

   class OmSsqlOlapConfProperties: public OmConfProperties
   {
   public:
      OmSsqlOlapConfProperties() ;
      virtual ~OmSsqlOlapConfProperties() ;

   public:
      bool isPrivateProperty( const string& name ) const ;
      bool isAllPropertySet() ;
   } ;

   class OmSsqlOlapConfigBuilder: public OmConfigBuilder
   {
   public:
      OmSsqlOlapConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmSsqlOlapConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _build( BSONArray &nodeConfig ) ;
      INT32 _createMasterNode( bool deployStandby ) ;
      INT32 _createSegmentNodes( INT32 segmentNum ) ;
      INT32 _getPort( const set<OmHost*>& hosts, const string& defaultPort , string& port ) ;
      bool  _isPortUsed( const set<OmHost*>& hosts, INT32 port ) const ;
      INT32 _getDataDir( const set<OmHost*>& hosts, const string& diskPath,
                           const string& role, const string& port, string& dataDir ) ;
      INT32 _isDataDirUsed( const set<OmHost*>& hosts, const string& dataDir ) const ;
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkMaster( const BSONObj& bsonConfig ) ;
      INT32 _checkSegment( const BSONObj& bsonConfig ) ;
      INT32 _getSegmentHostNames( const BSONObj& bsonConfig, set<string>& hostNames ) ;
      INT32 _rebuild( BSONObj& bsonConfig ) ;
      INT32 getHostNames( const BSONObj& bsonConfig, set<string>& hostNames ) ;

   private:
      OmSsqlOlapConfTemplate _template ;
      OmSsqlOlapConfProperties _properties ;
   } ;
}

#endif /* OM_CONFIG_SSQL_OLAP_H_ */
