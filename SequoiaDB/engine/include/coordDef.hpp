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

   Source File Name = coordDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORDDEF_HPP__
#define COORDDEF_HPP__

#include "clsCatalogAgent.hpp"
#include "utilMap.hpp"
#include "../bson/bson.h"
#include <vector>
#include <queue>
#include <string>
#include <set>

using namespace bson ;

namespace engine
{
   #define COORD_GROUPS_AVG_NUM              ( 20 )

   struct coordErrorInfo
   {
      INT32       _rc ;
      BSONObj     _obj ;

      coordErrorInfo( INT32 rc = SDB_OK )
      {
         _rc = rc ;
      }
      coordErrorInfo( INT32 rc, const BSONObj &obj )
      {
         _rc = rc ;
         _obj = obj.getOwned() ;
      }
      coordErrorInfo( const MsgOpReply *reply )
      {
         INT32 length = reply->header.messageLength -
                        (INT32)sizeof( MsgOpReply ) ;
         _rc = reply->flags ;
         if ( reply->flags && length > 0 )
         {
            try
            {
               _obj = BSONObj( (const CHAR*)reply + sizeof( MsgOpReply ) ).getOwned() ;
            }
            catch( std::exception & )
            {
            }
         }
      }
   } ;
   typedef std::queue<CHAR *>                                        REPLY_QUE ;
   typedef _utilMap< UINT64, coordErrorInfo, COORD_GROUPS_AVG_NUM >  ROUTE_RC_MAP ;
   typedef _utilMap< UINT64, MsgHeader*, COORD_GROUPS_AVG_NUM >      ROUTE_REPLY_MAP ;
   typedef _utilMap< UINT32, netIOVec, COORD_GROUPS_AVG_NUM >        GROUP_2_IOVEC ;
   typedef std::set< INT32 >                                         SET_RC ;
   typedef std::set< UINT64 >                                        SET_ROUTEID ;

   typedef _utilMap< UINT32, UINT32, COORD_GROUPS_AVG_NUM >          CoordGroupList ;
   typedef clsNodeItem                                               CoordNodeInfo ;
   typedef VEC_NODE_INFO                                             CoordVecNodeInfo ;

   typedef clsGroupItem                                              CoordGroupInfo ;

   typedef boost::shared_ptr<CoordGroupInfo>                         CoordGroupInfoPtr;
   typedef _utilMap< UINT32, CoordGroupInfoPtr, COORD_GROUPS_AVG_NUM >  CoordGroupMap;
   typedef std::vector< CoordGroupInfoPtr >                          GROUP_VEC ;
   typedef std::vector<std::string>                                  CoordSubCLlist;
   typedef _utilMap< UINT32, CoordSubCLlist, COORD_GROUPS_AVG_NUM >  CoordGroupSubCLMap;

   /*
      _CoordCataInfo define
   */
   class _CoordCataInfo : public SDBObject
   {
   public:
      _CoordCataInfo( INT32 version, const char *pCollectionName )
      :_catlogSet ( pCollectionName, FALSE )
      {
         _catlogSet.setSKSite( clsGetShardingKeySite() ) ;
      }

      ~_CoordCataInfo()
      {}

      void getGroupLst( CoordGroupList &groupLst ) const
      {
         groupLst = _groupLst ;
      }

      const CoordGroupList& getGroupLst() const
      {
         return _groupLst ;
      }

      INT32 getGroupNum() const
      {
         return _groupLst.size() ;
      }

      BOOLEAN isMainCL() const
      {
         return _catlogSet.isMainCL() ;
      }

      INT32 getSubCLList( CoordSubCLlist &subCLLst )
      {
         return _catlogSet.getSubCLList( subCLLst ) ;
      }

      BOOLEAN isContainSubCL( const string &subCLName ) const
      {
         return _catlogSet.isContainSubCL( subCLName ) ;
      }

      INT32 getSubCLCount () const
      {
         return _catlogSet.getSubCLCount() ;
      }

      INT32 getGroupByMatcher( const BSONObj &matcher,
                               CoordGroupList &groupLst )
      {
         INT32 rc = SDB_OK;

         if ( matcher.isEmpty() )
         {
            getGroupLst( groupLst ) ;
         }
         else
         {
            UINT32 i = 0 ;
            VEC_GROUP_ID vecGroup ;
            rc = _catlogSet.findGroupIDS( matcher, vecGroup ) ;
            if ( rc )
            {
               goto error ;
            }
            for ( ; i < vecGroup.size(); i++ )
            {
               groupLst[vecGroup[i]] = vecGroup[i];
            }
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 getGroupByRecord( const BSONObj &recordObj,
                              UINT32 &groupID )
      {
         return _catlogSet.findGroupID ( recordObj, groupID ) ;
      }

      INT32 getSubCLNameByRecord( const BSONObj &recordObj,
                                  string &subCLName )
      {
         return _catlogSet.findSubCLName( recordObj, subCLName ) ;
      }

      INT32 getMatchSubCLs( const BSONObj &matcher,
                            CoordSubCLlist &subCLList )
      {
         if ( matcher.isEmpty() )
         {
            return _catlogSet.getSubCLList( subCLList ) ;
         }
         else
         {
            return _catlogSet.findSubCLNames( matcher, subCLList ) ;
         }
      }

      INT32 getVersion() const
      {
         return _catlogSet.getVersion() ;
      }

      INT32 fromBSONObj ( const BSONObj &boRecord )
      {
         INT32 rc = _catlogSet.updateCatSet ( boRecord, 0 ) ;
         if ( SDB_OK == rc )
         {
            UINT32 groupID = 0 ;
            VEC_GROUP_ID *vecGroup = _catlogSet.getAllGroupID() ;
            for ( UINT32 index = 0 ; index < vecGroup->size() ; ++index )
            {
               groupID = (*vecGroup)[index] ;
               _groupLst[groupID] = groupID ;
            }
         }
         return rc ;
      }

      void getShardingKey ( BSONObj &shardingKey ) const
      {
         shardingKey = _catlogSet.getShardingKey() ;
      }

      BOOLEAN isIncludeShardingKey( const BSONObj &record ) const
      {
         return _catlogSet.isIncludeShardingKey( record ) ;
      }

      BOOLEAN isSharded () const
      {
         return _catlogSet.isSharding() ;
      }

      BOOLEAN isRangeSharded() const
      {
         return _catlogSet.isRangeSharding() ;
      }

      INT32 getGroupLowBound( UINT32 groupID, BSONObj &lowBound ) const
      {
         return _catlogSet.getGroupLowBound( groupID, lowBound ) ;
      }

      clsCatalogSet* getCatalogSet()
      {
         return &_catlogSet ;
      }

      BSONObj toBSON()
      {
         return _catlogSet.toCataInfoBson() ;
      }

      const CHAR* getName() const
      {
         return _catlogSet.name() ;
      }

      UINT32 getShardingKeySiteID() const
      {
         return _catlogSet.getShardingKeySiteID() ;
      }

      INT32 getLobGroupID( const OID &oid,
                           UINT32 sequence,
                           UINT32 &groupID )
      {
         return _catlogSet.findGroupID( oid, sequence, groupID ) ;
      }

   private:
      _CoordCataInfo()
      :_catlogSet( NULL, FALSE ) 
      {}  

   private:
      clsCatalogSet        _catlogSet ;
      CoordGroupList       _groupLst ;

   };
   typedef _CoordCataInfo CoordCataInfo ;

   typedef boost::shared_ptr< CoordCataInfo >         CoordCataInfoPtr ;
   typedef std::map< std::string, CoordCataInfoPtr >  CoordCataMap ;

}

#endif // COORDDEF_HPP__

