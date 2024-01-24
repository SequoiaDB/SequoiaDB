/******************************************************************************
@Description : seqDB-21950:主子表，主表分区键为多字段正逆序混合，带匹配符查询
@Author : YuanYue Li 2020-04-20
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   dataGroupNames.sort();
   var testcaseID = 21950;
   var csName = COMMCSNAME;
   var mclName = CHANGEDPREFIX + "_mcl_" + testcaseID;
   var sclName1 = CHANGEDPREFIX + "_scl_" + testcaseID + "_1";
   var sclName2 = CHANGEDPREFIX + "_scl_" + testcaseID + "_2";
   var sclFullName1 = csName + "." + sclName1;
   var sclFullName2 = csName + "." + sclName2;
   var recsNum = 200;

   commDropCL( db, csName, mclName );
   commDropCL( db, csName, sclName1 );
   commDropCL( db, csName, sclName2 );

   // ready main-sub cl
   var mclOptions = { "ShardingKey": { "a": 1, "b": -1 }, "IsMainCL": true };
   var mcl = commCreateCL( db, csName, mclName, mclOptions, false );
   var sclOptions = { "ShardingKey": { "a": 1, "b": -1 }, "ShardingType": "range", "Group": dataGroupNames[0] };
   var scl1 = commCreateCL( db, csName, sclName1, sclOptions, false );
   var scl2 = commCreateCL( db, csName, sclName2, sclOptions, false );
   mcl.attachCL( sclFullName1, { "LowBound": { "a": { "$minKey": 1 }, "b": { "$maxKey": 1 } }, "UpBound": { "a": 100, "b": 100 } } );
   mcl.attachCL( sclFullName2, { "LowBound": { "a": 100, "b": 100 }, "UpBound": { "a": { "$maxKey": 1 }, "b": { "$minKey": 1 } } } );

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i, "c": { "d": "test" + i } } );
   }
   mcl.insert( docs );

   // scl1 [min,50)[50,max)
   // scl2 [min,150)[150,max)
   scl1.split( dataGroupNames[0], dataGroupNames[1], 50 );
   scl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // $gt 
   var findCond = { "a": { "$gt": 100 }, "b": { "$gt": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $lt  
   var findCond = { "a": { "$lt": 100 }, "b": { "$lt": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName1], [sclFullName1]] );

   // $gte
   var findCond = { "a": { "$gte": 100 }, "b": { "$gte": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2, sclFullName1]] );

   // $lte
   var findCond = { "a": { "$lte": 100 }, "b": { "$lte": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName1]] );

   // $et
   var findCond = { "a": { "$et": 100 }, "b": { "$et": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]], true, [[sclFullName2]] );

   // $ne
   var findCond = { "a": { "$ne": 100 }, "b": { "$ne": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 101 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $in
   var findCond = { "a": { "$in": [99, 100] }, "b": { "$in": [99, 100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 99, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName1]] );

   // $nin
   var ninDocs = tmpDocs( 100, 200 );
   var findCond = { "a": { "$nin": ninDocs }, "b": { "$nin": ninDocs } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $and 子集相交
   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "b": { "$lt": 150 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 150 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2, sclFullName1]] );

   // $and 子集包含
   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "b": { "$gte": 150 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 150 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2, sclFullName1]] );

   // $and 子集相离
   var findCond = { "$and": [{ "a": { "$gt": 100 } }, { "b": { "$in": [50] } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $not 子集相交
   var findCond = { "$not": [{ "a": { "$lt": 100 } }, { "b": { "$nin": [100, 150, 120] } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $not 子集包含
   var findCond = { "$not": [{ "a": { "$lt": 100 } }, { "b": { "$lte": 150 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $not 子集相离
   var findCond = { "$not": [{ "a": { "$lt": 150 } }, { "b": { "$gte": 50 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 50 ).concat( docs.slice( 150 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集相交
   var findCond = { "$or": [{ "a": { "$lt": 100 } }, { "b": { "$gt": 50 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集包含
   var findCond = { "$or": [{ "a": { "$gte": 100 } }, { "b": { "$in": [120, 150, 170] } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集相离
   var findCond = { "$or": [{ "a": { "$lt": 100 } }, { "b": { "$gte": 150 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 150 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $all
   var findCond = { "a": { "$all": [100] }, "b": { "$all": [100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]], true, [[sclFullName2]] );

   // $isnull
   var findCond = { "a": { "$isnull": 0 }, "b": { "$isnull": 0 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $exists
   var findCond = { "a": { "$exists": 1 }, "b": { "$exists": 1 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $et+$field
   var findCond = { "a": { "$et": { "$field": "b" } } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $and+$or+$not组合查询
   var findCond = {
      "$and": [{ "a": { "$gte": 50 }, "b": { "$gte": 50 } }
         , { "$or": [{ "a": { "$gte": 100 }, "b": { "$gte": 100 } }, { "a": { "$lt": 50 }, "b": { "$lt": 50 } }] }
         , { "$not": [{ "a": { "$gte": 150 }, "b": { "$gte": 150 } }] }]
   };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 150 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2, sclFullName1]] );

   // $not+$and+$or组合查询
   var findCond = {
      "$not": [{ "a": { "$gte": 50 }, "b": { "$gte": 50 } }
         , { "$and": [{ "a": { "$gte": 100 }, "b": { "$gte": 100 } }, { "a": { "$lt": 150 }, "b": { "$lt": 150 } }] }
         , { "$or": [{ "a": { "$gt": 50 }, "b": { "$gt": 50 } }, { "a": { "$lt": 100 }, "b": { "$lt": 100 } }] }]
   };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 150 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   var objDocs = [{ "a": { "a1": 1 } }, { "a": { "a1": 2 } }, { "a": { "a1": "test" } }];
   mcl.insert( objDocs );

   // $regex + $elemMatch
   var findCond = { "a": { "$elemMatch": { "a1": { "$regex": "t.*t", "$options": "i" } } } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), objDocs.slice( 2 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   commDropCL( db, csName, mclName, false );
}
function tmpDocs ( startNum, endNum )
{
   var docs = [];
   for( var i = startNum; i < endNum; i++ )
   {
      docs.push( i );
   }
   return docs;
}
