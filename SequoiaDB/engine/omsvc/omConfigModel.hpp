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

   Source File Name = omConfigModel.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/3/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_HPP_
#define OM_CONFIG_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "omCommandInterface.hpp"
#include "../bson/bson.h"
#include <set>
#include <map>
#include <list>

using namespace std ;
using namespace bson ;

namespace engine
{
   class OmNode ;
   class OmNodes ;
   class OmHost ;
   class OmBusiness ;
   class OmCluster ;

   struct OmBusinessInfo: public SDBObject
   {
      string clusterName ;
      string businessType ;
      string businessName ;
      string deployMode ;

      bool operator== ( const OmBusinessInfo& other ) const
      {
         if ( clusterName != other.clusterName )
         {
            return false ;
         }

         if ( businessType != other.businessType )
         {
            return false ;
         }

         if ( businessName != other.businessName )
         {
            return false ;
         }

         return true ;
      }
   } ;

   class OmNode: public SDBObject
   {
   public:
      OmNode() ;
      virtual ~OmNode() ;
      static INT32 createObject( const string& businessType,
                                 const string& deployMode,
                                 OmNode*& node ) ;

   public:
      INT32 init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;

   public:
      const OmBusinessInfo&   getBusinessInfo() const { return _businessInfo ; }
      const string&           getHostName() const { return _hostName ; }
      const set<string>&      getPathes() const { return _pathes ; }
      const set<string>&      getPorts() const { return _ports ; }
      const set<string>&      getDisks() const { return _disks ; }
      bool                    isDiskUsed( const string& diskName ) const ;
      bool                    isPortUsed( const string& port ) const ;

   protected:
      virtual INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) = 0 ;
      INT32 _addPath( const string& path, OmHost& host, bool ignoreDisk = false ) ;
      INT32 _addUsedDisk( const string& diskName ) ;
      INT32 _addUsedPort( const string& port ) ;
      INT32 _addUsedPath( const string& path ) ;
      void  _setHostName( const string& hostName ) ;
      void  _setBusinessInfo( const OmBusinessInfo& businessInfo ) ;

   protected:
      OmBusinessInfo _businessInfo ;
      string         _hostName ;
      set<string>    _pathes ;
      set<string>    _ports ;
      set<string>    _disks ;
   } ;

   class OmNodes: public SDBObject
   {
   public:
      OmNodes( bool manageNode = false ) ;
      virtual ~OmNodes() ;

   public:
      typedef list<OmNode*>::const_iterator ConstIterator ;
      ConstIterator begin() const ;
      ConstIterator end() const ;

   public:
      INT32    nodeNum() const ;

      template<class Predicate>
      INT32    getNodes( Predicate pred, OmNodes& nodes ) const ;

      template<class Predicate>
      INT32     count( Predicate pred ) const ;

   private:
      bool  _hasNode( OmNode* node ) const ;
      INT32 _addNode( OmNode* node ) ;
      INT32 _delNode( OmNode* node ) ;

   private:
      list<OmNode*> _nodes ;
      bool          _manageNode ;

      typedef list<OmNode*>::const_iterator  OmNodesConstIterator ;
      typedef list<OmNode*>::iterator        OmNodesIterator ;

      friend class OmHost ;
      friend class OmBusiness ;
      friend class OmCluster ;
   } ;

   template<class Predicate>
   INT32 OmNodes::getNodes( Predicate pred, OmNodes& nodes ) const
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !nodes._manageNode, "nodes can't manage node" ) ;

      for ( OmNodesConstIterator it = _nodes.begin() ; it != _nodes.end() ; it++ )
      {
         OmNode* node = *it ;
         if ( pred( node ) )
         {
            rc = nodes._addNode( node ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   template<class Predicate>
   INT32 OmNodes::count( Predicate pred ) const
   {
      INT32 num = 0 ;

      for ( OmNodesConstIterator it = _nodes.begin() ; it != _nodes.end() ; it++ )
      {
         OmNode* node = *it ;
         if ( pred( node ) )
         {
            num++ ;
         }
      }

      return num ;
   }

   class OmHost: public SDBObject
   {
   public:
      OmHost( const string& hostName ) ;
      virtual ~OmHost() ;

   public:
      const string&     getHostName() const { return _hostName ; }
      INT32             nodeNum() const ;
      INT32             diskNum() const ;
      INT32             unusedDiskNum() const ;
      bool              isPortUsed( const string& port ) const ;
      bool              isPortUsed( INT32 port ) const ;
      bool              isPathUsed( const string& path ) const ;
      INT32             addNode( OmNode* node ) ;
      INT32             addDisk( const simpleDiskInfo& disk ) ;
      const simpleDiskInfo*   getDisk( const string path ) ;

      template<class Predicate>
      INT32 count( Predicate pred ) const
      {
         return _nodes.count( pred ) ;
      }

      template<class Predicate>
      const simpleDiskInfo* chooseDisk( Predicate pred ) const ;

   private:
      INT32   _addNode( OmNode* node ) ;
      INT32   _addUsedPort( const string& port ) ;
      INT32   _addUsedDisk( const string& diskName ) ;
      INT32   _addUsedPath( const string& diskName ) ; 

   private:
      string                        _hostName ;
      map<string, simpleDiskInfo>   _disks ;
      set<string>                   _usedPorts ;
      set<string>                   _usedDisks ;
      set<string>                   _usedPathes ;
      OmNodes                       _nodes ;

      typedef map<string, simpleDiskInfo>::const_iterator OmDiskConstIterator ;
   } ;

   template<class Predicate>
   const simpleDiskInfo* OmHost::chooseDisk( Predicate pred ) const
   {
      const simpleDiskInfo* disk = NULL ;

      if ( _disks.size() == 0 )
      {
         goto done ;
      }

      for ( OmDiskConstIterator it = _disks.begin() ; it != _disks.end() ; it++ )
      {
         const simpleDiskInfo* tmp = &( it->second ) ;

         if ( it == _disks.begin() || pred( disk, tmp ) < 0 )
         {
            disk = tmp ;
         }
      }

   done:
      return disk ;
   }

   class OmBusiness: public SDBObject
   {
   public:
      OmBusiness( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmBusiness() ;

   public:
      const OmBusinessInfo&   getBusinessInfo() const { return _businessInfo ; }
      const OmNodes&          getNodes() const { return _nodes ; }
      INT32                   nodeNum() const ;
      INT32                   addNode( OmNode* node ) ;

      template<class Predicate>
      INT32 count( Predicate pred ) const
      {
         return _nodes.count( pred ) ;
      }

   private:
      OmBusinessInfo _businessInfo ;
      OmNodes        _nodes ;
   } ;

   class OmCluster: public SDBObject
   {
   public:
      OmCluster() ;
      virtual ~OmCluster() ;

   public:
      INT32 init( const BSONObj& bsonBusiness, const BSONObj& bsonHost ) ;
      INT32 nodeNum() const ;
      INT32 hostNum() const ;
      INT32 addNode( OmNode* node ) ;
      INT32 addBusiness( OmBusiness* business ) ;
      INT32 getBusiness( const string& businessName, OmBusiness*& business ) ;
      INT32 getHost( const string& hostName, OmHost*& host ) ;

      template<class Predicate>
      OmHost* chooseHost( Predicate pred ) const ;

      template<class Predicate, class Excludes>
      OmHost* chooseHost( Predicate pred, Excludes excludes ) const ;

   private:
      INT32 _initBusiness( const BSONObj& bsonBusiness ) ;
      INT32 _initHostAndNode( const BSONObj& bsonHost ) ;
      INT32 _addHostAndNode( const BSONObj& hostNodeInfo ) ;
      INT32 _buildDisk( const BSONObj& bsonDisk, OmHost& host ) ;
      INT32 _buildNode( const BSONObj& bsonNode, OmHost& host ) ;
      INT32 _addHost( OmHost* host ) ;
      INT32 _addNode( OmNode* node ) ;

   private:
      OmNodes                    _nodes ;
      map<string, OmHost*>       _hosts ;
      map<string, OmBusiness*>   _businesses ;

      typedef map<string, OmHost*>::iterator OmHostsIterator ;
      typedef map<string, OmHost*>::const_iterator OmHostsConstIterator ;
      typedef map<string, OmBusiness*>::iterator OmBusinessesIterator ;
   } ;

   template<class Predicate>
   OmHost* OmCluster::chooseHost( Predicate pred ) const
   {
      OmHost* host = NULL ;

      if ( _hosts.size() == 0 )
      {
         goto done ;
      }

      for ( OmHostsConstIterator it = _hosts.begin() ; it != _hosts.end() ; it++ )
      {
         OmHost* tmp = it->second ;

         if ( it == _hosts.begin() || pred( host, tmp ) < 0 )
         {
            host = tmp ;
         }
      }

   done:
      return host ;
   }

   template<class Predicate, class Exclude>
   OmHost* OmCluster::chooseHost( Predicate pred, Exclude exclude ) const
   {
      OmHost* host = NULL ;

      if ( _hosts.size() == 0 )
      {
         goto done ;
      }

      for ( OmHostsConstIterator it = _hosts.begin() ; it != _hosts.end() ; it++ )
      {
         OmHost* tmp = it->second ;

         if ( exclude(tmp) )
         {
            continue ;
         }

         if ( NULL == host || pred( host, tmp ) < 0 )
         {
            host = tmp ;
         }
      }

   done:
      return host ;
   }


   class byDisk
   {
   public:
      byDisk( string diskName ):
         _diskName( diskName )
      {
      }

      bool operator() ( const OmNode* node ) const
      {
         if ( node->isDiskUsed( _diskName ) )
         {
            return true ;
         }

         return false ;
      }

   private:
      string _diskName ;
   } ;
}

#endif /* OM_CONFIG_HPP_ */
