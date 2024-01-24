/******************************************************************************
@Description : seqDB-21954:hash表，分区键为多字段正逆序混合，带匹配符查询
@Athor : XiaoNi Huang 2020-04-13
******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( arg )
{
   var dataGroupNames = commGetDataGroupNames( db );
   if( dataGroupNames.length < 3 )
   {
      return;
   }
   dataGroupNames.sort();
   dataGroupNames = dataGroupNames.slice( 0, 3 ); // for check

   var testcaseID = 21954;
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_" + testcaseID;
   var recsNum = 300;
   // ready docs
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i + 1 } );
   }

   // create cl   
   commDropCL( db, csName, clName, true );
   var options = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Partition": 256, "Group": dataGroupNames[0] };
   var cl = commCreateCL( db, csName, clName, options, true );

   // insert
   cl.insert( docs );

   // subCL split to multi group, e.g: [min,80), [80,160), [160,256)
   cl.split( dataGroupNames[0], dataGroupNames[1], { "Partition": 80 }, { "Partition": 160 } );
   cl.split( dataGroupNames[0], dataGroupNames[2], { "Partition": 160 }, { "Partition": 256 } );

   var cl = db.getCS( csName ).getCL( clName );

   // $gt
   var findCond = { "a": { "$gt": 6 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 7 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $lt
   var findCond = { "a": { "$lt": 6 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 6 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $gte
   var findCond = { "a": { "$gte": 6 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 6 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $lte
   var findCond = { "a": { "$lte": 6 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 7 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $et
   var findCond = { "a": { "$et": 6 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 6, 7 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $in
   var findCond = { "a": { "$in": [0, 5, 6] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 1 ).concat( docs.slice( 5, 7 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // $and + $or + $gt + $lt
   // $and, 子集相交
   var findCond = { "$and": [{ "$or": [{ "a": 25 }, { "a": 26 }, { "a": 27 }] }, { "$and": [{ "a": { "$gt": 25 } }, { "a": { "$lt": 27 } }] }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 26, 27 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $and, 子集包含
   var findCond = { "$and": [{ "$or": [{ "a": 25 }, { "a": 26 }] }, { "$and": [{ "a": { "$gte": 25 } }, { "a": { "$lte": 27 } }] }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 25, 27 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $and, 子集相离
   var findCond = { "$and": [{ "$or": [{ "a": 25 }, { "a": 26 }] }, { "$and": [{ "a": { "$gt": 26 } }, { "a": { "$lte": 27 } }] }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $or
   var findCond = { "$or": [{ "a": 5 }, { "a": 6 }, { "a": 7 }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 5, 8 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );


   // multi matches   
   // $not + $or + $and   
   var findCond = {
      "$not": [
         {
            "$and": [{ "$or": [{ "a": 25 }, { "a": 26 }, { "a": 27 }] },
            { "$and": [{ "a": { "$gt": 25 } }, { "a": { "$lt": 27 } }] }]
         }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 26 ).concat( docs.slice( 27 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // array
   var arrDocs = [{ "a": [99] }, { "a": [100] }, { "a": [200] }];
   cl.insert( arrDocs );
   // $all
   var findCond = { "a": { "$all": [100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( [{ "a": [100] }] ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $size
   var findCond = { "$and": [{ "a": { "$size": 1, "$et": 1 } }, { "a": { "$in": [100] } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [{ "a": [100] }] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // ($+标识符) + $expand, hit all dataGroups
   var findCond = { "$and": [{ "a.$1": 100 }, { "a": { "$expand": 1 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [{ "a": 100 }] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // ($+标识符) + $returnMatch
   var findCond = { "a": { "$returnMatch": 0, "$in": [100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( [{ "a": [100] }] ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // subobj
   var objDocs = [{ "a": { "a1": 1 } }, { "a": { "a1": 2 } }, { "a": { "a1": "test" } }];
   cl.insert( objDocs );
   // $elemMatch, hit all dataGroups
   var findCond = { "a": { "$elemMatch": { "a1": 2 } } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), objDocs.slice( 1, 2 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $regex + $elemMatch, hit all dataGroups
   var findCond = { "a": { "$elemMatch": { "a1": { "$regex": "t.*t", "$options": "i" } } } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), objDocs.slice( 2 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // others
   var othDocs = [{ "a": 100, "b": 100 }, { "a": "notNull" }, { "b": 1 }, { "a": null }];
   cl.insert( othDocs );

   // $isnull
   var findCond = { "a": { "$isnull": 1 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), othDocs.slice( 2 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $exists
   var findCond = { "a": { "$exists": 0 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), othDocs.slice( 2, 3 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $type
   var findCond = { "a": { "$type": 1, "$et": 4 } }; // 4: array
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), arrDocs );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $regex
   var findCond = { "a": { "$regex": "not.*l", "$options": "i" } }; // 4: array
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), othDocs.slice( 1, 2 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $et + $field, hit all dataGroups
   var findCond = { "a": { "$et": { "$field": "b" } } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), othDocs.slice( 0, 1 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   commDropCL( db, csName, clName, false );
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