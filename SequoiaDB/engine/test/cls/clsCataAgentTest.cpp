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
*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include <iostream>

#include "clsCatalogAgent.hpp"
#include "clsCatalogMatcher.hpp"

using namespace std;
using namespace engine;
using namespace bson ;

#define TEST_CL_NAME          "test.test"

BOOLEAN _checkVecResult( const VEC_GROUP_ID &expect,
                         const VEC_GROUP_ID &result )
{
   if ( expect.size() != result.size() )
   {
      return FALSE ;
   }
   for ( UINT32 i = 0 ; i < result.size() ; ++i )
   {
      BOOLEAN find = FALSE ;
      for ( UINT32 j = 0 ; j < expect.size() ; ++j )
      {
         if ( result[i] == expect[j] )
         {
            find = TRUE ;
            break ;
         }
      }
      if ( !find )
      {
         return FALSE ;
      }
   }
   return TRUE ;
}

void _buildCommonInfo( BSONObjBuilder &builder )
{
   builder.append( "Name", TEST_CL_NAME ) ;
   builder.append( "UniqueID", (INT64)8654359101447 ) ;
   builder.append( "Version", (INT32)5 ) ;
   builder.append( "Attribute", (INT32)1 ) ;
   builder.append( "AttributeDesc", "Compressed" ) ;
   builder.append( "CompressionType", (INT32)1 ) ;
   builder.append( "CompressionTypeDesc", "lzw" ) ;
   builder.appendBool( "EnsureShardingIndex", TRUE ) ;
}

BSONObj _buildSignalRangeInfo()
{
   /// { a : 1 }
   /// [ min, 5 ), [5, 20 ), [20, 100 ), [ 100, max ]
   ///    (1)        (2)        (3)         (4)
   BSONObjBuilder builder ;

   _buildCommonInfo( builder ) ;

   builder.append( "ShardingKey", BSON( "a" << (INT32)1 ) ) ;
   builder.append( "ShardingType", "range" ) ;

   /// CataInfo
   BSONArrayBuilder cataBd( builder.subarrayStart( "CataInfo" ) ) ;

   BSONObjBuilder bd1( cataBd.subobjStart() ) ;
   bd1.append( "ID", (INT32)0 ) ;
   bd1.append( "GroupID", (INT32)1 ) ;
   bd1.append( "GroupName", "db1" ) ;
   BSONObjBuilder lowBd( bd1.subobjStart( "LowBound" ) ) ;
   lowBd.appendMinKey( "a" ) ;
   lowBd.done() ;
   bd1.append( "UpBound", BSON( "a" << (INT32)5 ) ) ;
   bd1.done() ;

   BSONObjBuilder bd2( cataBd.subobjStart() ) ;
   bd2.append( "ID", (INT32)1 ) ;
   bd2.append( "GroupID", (INT32)2 ) ;
   bd2.append( "GroupName", "db2" ) ;
   bd2.append( "LowBound", BSON( "a" << (INT32)5 ) ) ;
   bd2.append( "UpBound", BSON( "a" << (INT32)20 ) ) ;
   bd2.done() ;

   BSONObjBuilder bd3( cataBd.subobjStart() ) ;
   bd3.append( "ID", (INT32)2 ) ;
   bd3.append( "GroupID", (INT32)3 ) ;
   bd3.append( "GroupName", "db3" ) ;
   bd3.append( "LowBound", BSON( "a" << (INT32)20 ) ) ;
   bd3.append( "UpBound", BSON( "a" << (INT32)100 ) ) ;
   bd3.done() ;

   BSONObjBuilder bd4( cataBd.subobjStart() ) ;
   bd4.append( "ID", (INT32)3 ) ;
   bd4.append( "GroupID", (INT32)4 ) ;
   bd4.append( "GroupName", "db4" ) ;
   bd4.append( "LowBound", BSON( "a" << (INT32)100 ) ) ;
   BSONObjBuilder upBd( bd1.subobjStart( "UpBound" ) ) ;
   upBd.appendMaxKey( "a" ) ;
   upBd.done() ;
   bd4.done() ;

   cataBd.done() ;

   return builder.obj() ;
}

BSONObj _buildSignalRangeInfo_Desc()
{
   /// { a : -1 }
   /// [ max, 100 ), [100, 20 ), [20, 5 ), [ 5, min ]
   ///    (1)           (2)        (3)        (4)
   BSONObjBuilder builder ;

   _buildCommonInfo( builder ) ;

   builder.append( "ShardingKey", BSON( "a" << (INT32)-1 ) ) ;
   builder.append( "ShardingType", "range" ) ;

   /// CataInfo
   BSONArrayBuilder cataBd( builder.subarrayStart( "CataInfo" ) ) ;

   BSONObjBuilder bd1( cataBd.subobjStart() ) ;
   bd1.append( "ID", (INT32)0 ) ;
   bd1.append( "GroupID", (INT32)1 ) ;
   bd1.append( "GroupName", "db1" ) ;
   BSONObjBuilder lowBd( bd1.subobjStart( "LowBound" ) ) ;
   lowBd.appendMaxKey( "a" ) ;
   lowBd.done() ;
   bd1.append( "UpBound", BSON( "a" << (INT32)100 ) ) ;
   bd1.done() ;

   BSONObjBuilder bd2( cataBd.subobjStart() ) ;
   bd2.append( "ID", (INT32)1 ) ;
   bd2.append( "GroupID", (INT32)2 ) ;
   bd2.append( "GroupName", "db2" ) ;
   bd2.append( "LowBound", BSON( "a" << (INT32)100 ) ) ;
   bd2.append( "UpBound", BSON( "a" << (INT32)20 ) ) ;
   bd2.done() ;

   BSONObjBuilder bd3( cataBd.subobjStart() ) ;
   bd3.append( "ID", (INT32)2 ) ;
   bd3.append( "GroupID", (INT32)3 ) ;
   bd3.append( "GroupName", "db3" ) ;
   bd3.append( "LowBound", BSON( "a" << (INT32)20 ) ) ;
   bd3.append( "UpBound", BSON( "a" << (INT32)5 ) ) ;
   bd3.done() ;

   BSONObjBuilder bd4( cataBd.subobjStart() ) ;
   bd4.append( "ID", (INT32)3 ) ;
   bd4.append( "GroupID", (INT32)4 ) ;
   bd4.append( "GroupName", "db4" ) ;
   bd4.append( "LowBound", BSON( "a" << (INT32)5 ) ) ;
   BSONObjBuilder upBd( bd1.subobjStart( "UpBound" ) ) ;
   upBd.appendMinKey( "a" ) ;
   upBd.done() ;
   bd4.done() ;

   cataBd.done() ;

   return builder.obj() ;
}

BSONObj _buildSignalHashInfo()
{
   /// { a : 1 }
   /// [ 0, 1024 ), [1024, 2048 ), [2048, 3072 ), [ 3072, 4096 ]
   ///    (1)             (2)           (3)            (4)
   BSONObjBuilder builder ;

   _buildCommonInfo( builder ) ;

   builder.append( "ShardingKey", BSON( "a" << (INT32)1 ) ) ;
   builder.append( "ShardingType", "hash" ) ;
   builder.append( "Partition", (INT32)4096 ) ;
   builder.append( "InternalV", (INT32)3 ) ;

   /// CataInfo
   BSONArrayBuilder cataBd( builder.subarrayStart( "CataInfo" ) ) ;

   BSONObjBuilder bd1( cataBd.subobjStart() ) ;
   bd1.append( "ID", (INT32)0 ) ;
   bd1.append( "GroupID", (INT32)1 ) ;
   bd1.append( "GroupName", "db1" ) ;
   bd1.append( "LowBound", BSON( "" << (INT32)0 ) ) ;
   bd1.append( "UpBound", BSON( "" << (INT32)1024 ) ) ;
   bd1.done() ;

   BSONObjBuilder bd2( cataBd.subobjStart() ) ;
   bd2.append( "ID", (INT32)1 ) ;
   bd2.append( "GroupID", (INT32)2 ) ;
   bd2.append( "GroupName", "db2" ) ;
   bd2.append( "LowBound", BSON( "" << (INT32)1024 ) ) ;
   bd2.append( "UpBound", BSON( "" << (INT32)2048 ) ) ;
   bd2.done() ;

   BSONObjBuilder bd3( cataBd.subobjStart() ) ;
   bd3.append( "ID", (INT32)2 ) ;
   bd3.append( "GroupID", (INT32)3 ) ;
   bd3.append( "GroupName", "db3" ) ;
   bd3.append( "LowBound", BSON( "" << (INT32)2048 ) ) ;
   bd3.append( "UpBound", BSON( "" << (INT32)3072 ) ) ;
   bd3.done() ;

   BSONObjBuilder bd4( cataBd.subobjStart() ) ;
   bd4.append( "ID", (INT32)3 ) ;
   bd4.append( "GroupID", (INT32)4 ) ;
   bd4.append( "GroupName", "db4" ) ;
   bd4.append( "LowBound", BSON( "" << (INT32)3072 ) ) ;
   bd4.append( "UpBound", BSON( "" << (INT32)4096 ) ) ;
   bd4.done() ;

   cataBd.done() ;

   return builder.obj() ;
}

BSONObj _buildMultiRangeInfo()
{
   /// { a : 1, b : 1 }
   /// [ min:min, 100:0 ), [ 100:0, 100:100 ), [ 100:100, 100:500 ), [ 100:500, 500:500 ), [ 500:500, max:max ]
   ///          (1)                (2)                  (3)                   (4)                  (5)
   BSONObjBuilder builder ;

   _buildCommonInfo( builder ) ;

   builder.append( "ShardingKey", BSON( "a" << (INT32)1 <<
                                        "b" << (INT32)1 ) ) ;
   builder.append( "ShardingType", "range" ) ;

   /// CataInfo
   BSONArrayBuilder cataBd( builder.subarrayStart( "CataInfo" ) ) ;

   BSONObjBuilder bd1( cataBd.subobjStart() ) ;
   bd1.append( "ID", (INT32)0 ) ;
   bd1.append( "GroupID", (INT32)1 ) ;
   bd1.append( "GroupName", "db1" ) ;
   BSONObjBuilder lowBd( bd1.subobjStart( "LowBound" ) ) ;
   lowBd.appendMinKey( "a" ) ;
   lowBd.appendMinKey( "b" ) ;
   lowBd.done() ;
   bd1.append( "UpBound", BSON( "a" << (INT32)100 <<
                                "b" << (INT32)0 ) ) ;
   bd1.done() ;

   BSONObjBuilder bd2( cataBd.subobjStart() ) ;
   bd2.append( "ID", (INT32)1 ) ;
   bd2.append( "GroupID", (INT32)2 ) ;
   bd2.append( "GroupName", "db2" ) ;
   bd2.append( "LowBound", BSON( "a" << (INT32)100 <<
                                 "b" << (INT32)0 ) ) ;
   bd2.append( "UpBound", BSON( "a" << (INT32)100 <<
                                "b" << (INT32)100 ) ) ;
   bd2.done() ;

   BSONObjBuilder bd3( cataBd.subobjStart() ) ;
   bd3.append( "ID", (INT32)2 ) ;
   bd3.append( "GroupID", (INT32)3 ) ;
   bd3.append( "GroupName", "db3" ) ;
   bd3.append( "LowBound", BSON( "a" << (INT32)100 <<
                                 "b" << (INT32)100 ) ) ;
   bd3.append( "UpBound", BSON( "a" << (INT32)100 <<
                                "b" << (INT32)500 ) ) ;
   bd3.done() ;

   BSONObjBuilder bd4( cataBd.subobjStart() ) ;
   bd4.append( "ID", (INT32)3 ) ;
   bd4.append( "GroupID", (INT32)4 ) ;
   bd4.append( "GroupName", "db4" ) ;
   bd4.append( "LowBound", BSON( "a" << (INT32)100 <<
                                 "b" << (INT32)500 ) ) ;
   bd4.append( "UpBound", BSON( "a" << (INT32)500 <<
                                "b" << (INT32)500 ) ) ;
   bd4.done() ;

   BSONObjBuilder bd5( cataBd.subobjStart() ) ;
   bd5.append( "ID", (INT32)4 ) ;
   bd5.append( "GroupID", (INT32)5 ) ;
   bd5.append( "GroupName", "db5" ) ;
   bd5.append( "LowBound", BSON( "a" << (INT32)500 <<
                                 "b" << (INT32)500 ) ) ;
   BSONObjBuilder upBd( bd1.subobjStart( "UpBound" ) ) ;
   upBd.appendMaxKey( "a" ) ;
   upBd.appendMaxKey( "b" ) ;
   upBd.done() ;
   bd5.done() ;

   cataBd.done() ;

   return builder.obj() ;
}

BSONObj _buildMultiRangeInfo_Desc()
{
   /// { a : -1, b : 1 }
   /// [ max:min, 500:0 ), [ 500:0, 500:100 ), [ 500:100, 500:500 ), [ 500:500, 100:500 ), [ 100:500, min:max ]
   ///          (1)                (2)                  (3)                   (4)                  (5)
   BSONObjBuilder builder ;

   _buildCommonInfo( builder ) ;

   builder.append( "ShardingKey", BSON( "a" << (INT32)-1 <<
                                        "b" << (INT32)1 ) ) ;
   builder.append( "ShardingType", "range" ) ;

   /// CataInfo
   BSONArrayBuilder cataBd( builder.subarrayStart( "CataInfo" ) ) ;

   BSONObjBuilder bd1( cataBd.subobjStart() ) ;
   bd1.append( "ID", (INT32)0 ) ;
   bd1.append( "GroupID", (INT32)1 ) ;
   bd1.append( "GroupName", "db1" ) ;
   BSONObjBuilder lowBd( bd1.subobjStart( "LowBound" ) ) ;
   lowBd.appendMaxKey( "a" ) ;
   lowBd.appendMinKey( "b" ) ;
   lowBd.done() ;
   bd1.append( "UpBound", BSON( "a" << (INT32)500 <<
                                "b" << (INT32)0 ) ) ;
   bd1.done() ;

   BSONObjBuilder bd2( cataBd.subobjStart() ) ;
   bd2.append( "ID", (INT32)1 ) ;
   bd2.append( "GroupID", (INT32)2 ) ;
   bd2.append( "GroupName", "db2" ) ;
   bd2.append( "LowBound", BSON( "a" << (INT32)500 <<
                                 "b" << (INT32)0 ) ) ;
   bd2.append( "UpBound", BSON( "a" << (INT32)500 <<
                                "b" << (INT32)100 ) ) ;
   bd2.done() ;

   BSONObjBuilder bd3( cataBd.subobjStart() ) ;
   bd3.append( "ID", (INT32)2 ) ;
   bd3.append( "GroupID", (INT32)3 ) ;
   bd3.append( "GroupName", "db3" ) ;
   bd3.append( "LowBound", BSON( "a" << (INT32)500 <<
                                 "b" << (INT32)100 ) ) ;
   bd3.append( "UpBound", BSON( "a" << (INT32)500 <<
                                "b" << (INT32)500 ) ) ;
   bd3.done() ;

   BSONObjBuilder bd4( cataBd.subobjStart() ) ;
   bd4.append( "ID", (INT32)3 ) ;
   bd4.append( "GroupID", (INT32)4 ) ;
   bd4.append( "GroupName", "db4" ) ;
   bd4.append( "LowBound", BSON( "a" << (INT32)500 <<
                                 "b" << (INT32)500 ) ) ;
   bd4.append( "UpBound", BSON( "a" << (INT32)100 <<
                                "b" << (INT32)500 ) ) ;
   bd4.done() ;

   BSONObjBuilder bd5( cataBd.subobjStart() ) ;
   bd5.append( "ID", (INT32)4 ) ;
   bd5.append( "GroupID", (INT32)5 ) ;
   bd5.append( "GroupName", "db5" ) ;
   bd5.append( "LowBound", BSON( "a" << (INT32)100 <<
                                 "b" << (INT32)500 ) ) ;
   BSONObjBuilder upBd( bd1.subobjStart( "UpBound" ) ) ;
   upBd.appendMinKey( "a" ) ;
   upBd.appendMaxKey( "b" ) ;
   upBd.done() ;
   bd5.done() ;

   cataBd.done() ;

   return builder.obj() ;
}

TEST(clsTest, clsCataMatcher_Range_Signal_01)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildSignalRangeInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { a : 100 }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" << 100 ), vecGroup ) ) ;
   ASSERT_TRUE( 1 == vecGroup.size() && 4 == vecGroup[0] ) ;

   /// { a : 25 }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" << 25 ), vecGroup ) ) ;
   ASSERT_TRUE( 1 == vecGroup.size() && 3 == vecGroup[0] ) ;

   /// { a : { $et : 10 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$et" << 10 ) ),
                                               vecGroup ) ) ;
   ASSERT_TRUE( 1 == vecGroup.size() && 2 == vecGroup[0] ) ;

   /// { a : { $in : [ 0, 20, 110 ] } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$in" <<
                                                     BSON_ARRAY( 0 << 20 << 110 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lt : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$lt" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$lte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 10, $lt : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 10 <<
                                                         "$lt" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 10, $lte : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 10 <<
                                                         "$lte" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 100, $lte : 10 }  }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "a" <<
                                                         BSON( "$gt" << 100 <<
                                                               "$lte" << 10 ) )) ) ;
      ASSERT_TRUE( tmpMatcher.isNull() ) ;
   }

   /// { b : 10 }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" << 10 ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $et : { $field : "b" } } }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" <<
                                                      BSON( "$field" << "b" ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { b : { $lt : 10 }, a : { $gt : 105 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "b" <<
                                                   BSON( "$lt" << 10 ) <<
                                                     "a" <<
                                                   BSON( "$gt" << 105 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Range_Signal_02)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildSignalRangeInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { $and : [ { a : { $lte : 20 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 20 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 20 } }, { a : { $gte : 5 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 20 ) ) <<
                                                               BSON( "a" <<
                                                                  BSON( "$gte" << 5 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 2 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 20 } }, { b : { $gte : 5 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 20 ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gte" << 5 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : { $lte : 20 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 20 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 5 }, { a : 25 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" << 5 ) <<
                                                               BSON( "a" << 25 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 5 }, { b : 25 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$or" <<
                                                        BSON_ARRAY( BSON( "a" << 5 ) <<
                                                                    BSON( "b" << 25 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                  BSON_ARRAY( BSON( "a" << 5 ) <<
                                                              BSON( "b" << 25 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $not : [ { a : 1 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$not" <<
                                                      BSON_ARRAY( BSON( "a" << 1 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { $and : [ { $or : [ { a : 1 }, { a : 20 } ] }, { a : { $gt : 15 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "a" << 1 ) <<
                                                                              BSON( "a" << 20 ) ) ) <<
                                                               BSON( "a" <<
                                                                  BSON( "$gt" << 15 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { $and : [ { a : 20 } ] }, { a : 100 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "$and" <<
                                                                  BSON_ARRAY( BSON( "a" << 20 ) ) ) <<
                                                               BSON( "a" << 100 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Range_Signal_Desc_01)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildSignalRangeInfo_Desc() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { a : { $in : [ 0, 20, 110 ] } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$in" <<
                                                     BSON_ARRAY( 0 << 20 << 110 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$lte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Hash_Signal_01)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildSignalHashInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { a : 10 } == { a : { $et : 10 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" << 10 ), vecGroup ) ) ;
   ASSERT_TRUE( 1 == vecGroup.size() ) ;
   expect = vecGroup ;

   vecGroup.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$et" << 10 ) ),
                                               vecGroup ) ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $in : [ 0, 20, 110 ] } } ==
   /// { $or : [ { a : 0 }, { a : 20 }, { a : 110 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$in" <<
                                                     BSON_ARRAY( 0 << 20 << 110 ) ) ),
                                               vecGroup ) ) ;
   expect = vecGroup ;

   vecGroup.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" << 0 ) <<
                                                               BSON( "a" << 20 ) <<
                                                               BSON( "a" << 110 ) ) ),
                                               vecGroup ) ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lt : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$lt" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lte : 20 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$lte" << 20 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 10, $lt : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 10 <<
                                                         "$lt" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 10, $lte : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$gt" << 10 <<
                                                         "$lte" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 100, $lte : 10 }  }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "a" <<
                                                         BSON( "$gt" << 100 <<
                                                               "$lte" << 10 ) )) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $isnull : 0 }  }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "a" <<
                                                         BSON( "$isnull" << 0 ) )) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $exists : 1 }  }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "a" <<
                                                         BSON( "$exists" << 1 ) )) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { b : 10 }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" << 10 ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $et : { $field : "b" } } }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" <<
                                                      BSON( "$field" << "b" ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { b : { $lt : 10 }, a : { $gt : 105 } }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" <<
                                                        BSON( "$lt" << 10 ) <<
                                                           "a" <<
                                                        BSON( "$gt" << 105 ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $isnull : 1 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$isnull" << 1 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $exists : 0 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$exists" << 0 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Hash_Signal_02)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildSignalHashInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { $and : [ { a : { $lte : 20 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 20 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 20 } }, { a : { $gte : 5 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 20 ) ) <<
                                                               BSON( "a" <<
                                                                  BSON( "$gte" << 5 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 20 } }, { b : { $gte : 5 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 20 ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gte" << 5 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : { $lte : 20 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 20 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 5 }, { b : 25 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$or" <<
                                                        BSON_ARRAY( BSON( "a" << 5 ) <<
                                                                    BSON( "b" << 25 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                  BSON_ARRAY( BSON( "a" << 5 ) <<
                                                              BSON( "b" << 25 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $not : [ { a : 1 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 ), TRUE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$not" <<
                                                      BSON_ARRAY( BSON( "a" << 1 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { $and : [ { $or : [ { a : 1 }, { a : 20 } ] }, { a : { $gt : 15 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "a" << 1 ) <<
                                                                              BSON( "a" << 20 ) ) ) <<
                                                               BSON( "a" <<
                                                                  BSON( "$gt" << 15 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or: [ { a: { $lt: 5 } }, { a: { $et: 2 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 5 ) ) <<
                                                               BSON( "a" <<
                                                                  BSON( "$et" << 2 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Range_Multi_01)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildMultiRangeInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { a : 100 }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" << 100 ), vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 100 }, b : { $gt : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$gt" << 100 ) <<
                                                     "b" <<
                                                    BSON( "$gt" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gte : 100 }, b : { $gte : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$gte" << 100 ) <<
                                                     "b" <<
                                                    BSON( "$gte" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $in : [ 0, 100 ] }, b : { $in : [ 0, 501 ] } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                   BSON( "$in" <<
                                                     BSON_ARRAY( 0 << 100 ) ) <<
                                                     "b" <<
                                                   BSON( "$in" <<
                                                     BSON_ARRAY( 0 << 501 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lt : 100 }, b : { $gt : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$lt" << 100 ) <<
                                                     "b" <<
                                                    BSON( "$gt" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $lte : 100 }, b : { $lte : 100 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "a" <<
                                                    BSON( "$lte" << 100 ) <<
                                                     "b" <<
                                                    BSON( "$lte" << 100 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { a : { $gt : 100, $lte : 10 }, b : 10  }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 << "b" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "a" <<
                                                         BSON( "$gt" << 100 <<
                                                               "$lte" << 10 ) <<
                                                           "b" << 10 ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isNull() ) ;
   }

   /// { b : 10 }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 << "b" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" << 10 ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { a : { $et : { $field : "b" } } }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 << "b" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "b" <<
                                                      BSON( "$field" << "b" ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }

   /// { b : { $lt : 10 }, a : { $gt : 105 } }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "b" <<
                                                   BSON( "$lt" << 10 ) <<
                                                     "a" <<
                                                   BSON( "$gt" << 105 ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Range_Multi_02)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildMultiRangeInfo() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { $and : [ { a : { $lte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 100 } }, { b : { $gte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 100 ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : 100 }, { $or : [ { b : 10 }, { b : 500 } ] } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" << 100 ) <<
                                                               BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "b" << 10 ) <<
                                                                              BSON( "b" << 500 ) )
                                                                  ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : { $lte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 10, b : 10 }, { a : 100, b : 500 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" << 10 <<
                                                                     "b" << 10 ) <<
                                                               BSON( "a" << 100 <<
                                                                     "b" << 500 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 5 }, { b : 25 }, { c : 1 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << 1 << "b" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$or" <<
                                                        BSON_ARRAY( BSON( "a" << 5 ) <<
                                                                    BSON( "b" << 25 ) <<
                                                                    BSON( "c" << 1 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                  BSON_ARRAY( BSON( "a" << 5 ) <<
                                                              BSON( "b" << 25 ) <<
                                                              BSON( "c" << 1 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { $or : [ { a : 1 }, { a : 100 } ] }, { b : { $gt : 400 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "a" << 1 ) <<
                                                                              BSON( "a" << 100 ) ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gt" << 400 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { $and : [ { a : { $gt : 100 } }, { a : { $lte : 500 } } ] }, { a : 1 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "$and" <<
                                                                  BSON_ARRAY( BSON( "a" <<
                                                                                 BSON( "$gt" << 100 ) ) <<
                                                                              BSON( "a" <<
                                                                                 BSON( "$lte" << 500 ) ) ) ) <<
                                                               BSON( "a" << 1 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}

TEST(clsTest, clsCataMatcher_Range_Multi_Desc_01)
{
   clsCatalogSet catSet( TEST_CL_NAME ) ;
   BSONObj obj = _buildMultiRangeInfo_Desc() ;
   VEC_GROUP_ID vecGroup ;
   VEC_GROUP_ID expect ;

   ASSERT_TRUE( SDB_OK == catSet.updateCatSet( obj ) ) ;

   /// { $and : [ { a : { $lte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : { $lt : 100 } }, { b : { $gte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lt" << 100 ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { a : 100 }, { $or : [ { b : 10 }, { b : 500 } ] } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "a" << 100 ) <<
                                                               BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "b" << 10 ) <<
                                                                              BSON( "b" << 500 ) )
                                                                  ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : { $lte : 100 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" <<
                                                                  BSON( "$lte" << 100 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 10, b : 10 }, { a : 500, b : 500 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "a" << 10 <<
                                                                     "b" << 10 ) <<
                                                               BSON( "a" << 500 <<
                                                                     "b" << 500 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { a : 5 }, { b : 25 }, { c : 1 } ] }
   {
      clsCatalogMatcher tmpMatcher( BSON( "a" << -1 << "b" << 1 ), FALSE ) ;
      ASSERT_TRUE( SDB_OK == tmpMatcher.loadPattern( BSON( "$or" <<
                                                        BSON_ARRAY( BSON( "a" << 5 ) <<
                                                                    BSON( "b" << 25 ) <<
                                                                    BSON( "c" << 1 ) ) ) ) ) ;
      ASSERT_TRUE( tmpMatcher.isUniverse() ) ;
   }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                  BSON_ARRAY( BSON( "a" << 5 ) <<
                                                              BSON( "b" << 25 ) <<
                                                              BSON( "c" << 1 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $and : [ { $or : [ { a : 1 }, { a : 500 } ] }, { b : { $gt : 400 } } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$and" <<
                                                   BSON_ARRAY( BSON( "$or" <<
                                                                  BSON_ARRAY( BSON( "a" << 1 ) <<
                                                                              BSON( "a" << 500 ) ) ) <<
                                                               BSON( "b" <<
                                                                  BSON( "$gt" << 400 ) ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

   /// { $or : [ { $and : [ { a : { $gt : 100 } }, { a : { $lte : 500 } } ] }, { a : 1 } ] }
   vecGroup.clear() ;
   expect.clear() ;
   ASSERT_TRUE( SDB_OK == catSet.findGroupIDS( BSON( "$or" <<
                                                   BSON_ARRAY( BSON( "$and" <<
                                                                  BSON_ARRAY( BSON( "a" <<
                                                                                 BSON( "$gt" << 100 ) ) <<
                                                                              BSON( "a" <<
                                                                                 BSON( "$lte" << 500 ) ) ) ) <<
                                                               BSON( "a" << 1 ) ) ),
                                               vecGroup ) ) ;
   expect.push_back( 1 ) ;
   expect.push_back( 2 ) ;
   expect.push_back( 3 ) ;
   expect.push_back( 4 ) ;
   expect.push_back( 5 ) ;
   ASSERT_TRUE( _checkVecResult( expect, vecGroup ) ) ;

}


