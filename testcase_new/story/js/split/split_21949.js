/******************************************************************************
@Description : seqDB-21949:主子表，主表分区键为单字段正序，带匹配符查询 
@Author : YuanYue Li 2020-04-14
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   dataGroupNames.sort();
   var testcaseID = 21949;
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
   var mclOptions = { "ShardingKey": { "a": 1 }, "IsMainCL": true };
   var mcl = commCreateCL( db, csName, mclName, mclOptions, false );
   var sclOptions = { "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": dataGroupNames[0] };
   var scl1 = commCreateCL( db, csName, sclName1, sclOptions, false );
   var scl2 = commCreateCL( db, csName, sclName2, sclOptions, false );
   mcl.attachCL( sclFullName1, { "LowBound": { "a": { "$minKey": 1 } }, "UpBound": { "a": 100 } } );
   mcl.attachCL( sclFullName2, { "LowBound": { "a": 100 }, "UpBound": { "a": { "$maxKey": 1 } } } );

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i } );
   }
   mcl.insert( docs );

   // scl1 [min,50) [50,max)
   // scl2 [min,150) [150,max)
   scl1.split( dataGroupNames[0], dataGroupNames[1], 50 );
   scl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // $gt
   var findCond = { "a": { "$gt": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $lt
   var findCond = { "a": { "$lt": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName1], [sclFullName1]] );

   // $gte
   var findCond = { "a": { "$gte": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $lte
   var findCond = { "a": { "$lte": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName1]] );

   // $et
   var findCond = { "a": { "$et": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]], true, [[sclFullName2]] );

   // $ne
   var findCond = { "a": { "$ne": 100 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 101 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $in
   var findCond = { "a": { "$in": [100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]], true, [[sclFullName2]] );

   // $nin
   var ninDocs = tmpDocs( 100, 200 );
   var findCond = { "a": { "$nin": ninDocs } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $and 子集相交
   var findCond = { "$and": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName1]] );

   // $and 子集相离 
   var findCond = { "$and": [{ "a": { "$lte": 50 } }, { "a": { "$gt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), ['SYSCoord'] );

   // $and 子集包含
   var findCond = { "$and": [{ "a": { "$in": [150] } }, { "a": { "$gt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 150, 151 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName2]] );

   // $not 子集相交
   var findCond = { "$not": [{ "a": { "$lt": 50 } }, { "a": { "$gt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $not 子集相离
   var findCond = { "$not": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 150 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 50 ).concat( docs.slice( 150 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $not 子集包含
   var findCond = { "$not": [{ "a": { "$lt": 50 } }, { "a": { "$lte": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ),
      [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集相交
   var findCond = { "$or": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集相离
   var findCond = { "$or": [{ "a": { "$lt": 50 } }, { "a": { "$gte": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 50 ).concat( docs.slice( 100 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $or 子集包含
   var findCond = { "$or": [{ "a": { "$lte": 50 } }, { "a": { "$lt": 100 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName1], [sclFullName1]] );

   // 多个$and组合查询
   var findCond = { "$and": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 100 } }, { "a": { $isnull: 0 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName1]] );

   // 多个$not组合查询
   var findCond = { "$not": [{ "a": { "$nin": [50, 100, 150] } }, { "a": { "$exists": 1 } }, { "a": { "$isnull": 0 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50, 51 ).concat( docs.slice( 100, 101 ) ).concat( docs.slice( 150, 151 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // 多个$or组合查询
   var findCond = { "$or": [{ "a": { "$gte": 100 } }, { "a": { "$gt": 150 } }, { "a": { "$in": [100, 150] } }] };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $and + $or + $not组合查询
   var findCond = {
      "$and": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 150 } }
         , { "$or": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 100 } }] }, { "$not": [{ "a": { "$gte": 100 } }] }]
   };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50, 100 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName1]] );

   // $not+$and+$or组合查询
   var findCond = {
      "$not": [{ "a": { "$gte": 50 } }, { "$and": [{ "a": { "$gte": 100 } }, { "a": { "$lt": 150 } }] }
         , { "$or": [{ "a": { "$gt": 100 } }, { "a": { "$lt": 150 } }] }]
   };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 150 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // 多个$and+$多个$or+多个$not
   var findCond = {
      "$and": [{ "$or": [{ "a": { "$gte": 50 } }, { "a": { "$lt": 150 } }] }
         , { "$and": [{ "a": { "$gt": 50 } }, { "a": { "$lte": 200 } }] }, { "$not": [{ "a": { "$gt": 150 } }] }
         , { "$or": [{ "a": { "$gt": 100 } }, { "a": { "$lte": 50 } }] }, { "$not": [{ "a": { "$lt": 50 } }] }]
   };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 101, 151 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2], [sclFullName2]] );

   // $all
   var findCond = { "a": { "$all": [100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]], true, [[sclFullName2]] );

   // $isnull 
   var findCond = { "a": { "$isnull": 0 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $exists
   var findCond = { "a": { "$exists": 1 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $mod
   var findCond = { "a": { "$mod": [101, 100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   var arrDocs = [{ "a": [99] }, { "a": [100] }, { "a": [200] }];
   mcl.insert( arrDocs );

   // $+标识符 + $returnMatch
   var findCond = { "a": { "$returnMatch": 0, "$in": [100] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( arrDocs.slice( 1, 2 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $size
   var findCond = { "a": { "$size": 1, "$et": 1 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), arrDocs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $type
   var findCond = { "a": { "$type": 1, "$et": 4 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), arrDocs );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // $expand
   var findCond = { "$and": [{ "a.$1": 100 }, { "a": { "$expand": 1 } }] };
   commCompareResults( mcl.find( findCond ).sort( { "e": 1 } ), [{ "a": 100 }] );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   var objDocs = [{ "a": { "a1": 1 } }, { "a": { "a1": 2 } }, { "a": { "a1": "test" } }];
   mcl.insert( objDocs );

   // $regex + $elemMatch
   var findCond = { "a": { "$elemMatch": { "a1": { "$regex": "t.*t", "$options": "i" } } } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), objDocs.slice( 2 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } )
      , [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   var othDocs = [{ "a": 100, "b": 100 }];
   mcl.insert( othDocs );

   // $et+$field组合查询
   var findCond = { "a": { "$et": { "$field": "b" } } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), othDocs.slice( 0, 1 ) );
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
