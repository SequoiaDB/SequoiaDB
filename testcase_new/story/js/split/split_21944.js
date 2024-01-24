/******************************************************************************
@Description : seqDB-21944:range表，分区键为单字段正序，带匹配符查询 
@Athor : XiaoNi Huang 2020-04-13
******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   if( dataGroupNames.length < 3 )
   {
      return;
   }
   dataGroupNames.sort();
   dataGroupNames = dataGroupNames.slice( 0, 3 ); // for check

   var testcaseID = 21944;
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
   var options = { "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": dataGroupNames[0] };
   var cl = commCreateCL( db, csName, clName, options, true );

   // insert
   cl.insert( docs );

   // subCL split to multi group, e.g: [min,100), [100,200), [200,max)
   cl.split( dataGroupNames[0], dataGroupNames[1], { "a": 100 }, { "a": 200 } );
   cl.split( dataGroupNames[0], dataGroupNames[2], { "a": 200 }, { "a": { "$maxKey": 1 } } );

   // $gt
   var findCond = { "a": { "$gt": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $gte
   var findCond = { "a": { "$gte": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $lt
   var findCond = { "a": { "$lt": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $lte
   var findCond = { "a": { "$lte": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // $et
   var findCond = { "a": { "$et": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // $ne, hit all dataGroups
   var findCond = { "a": { "$ne": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 101 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $in
   var findCond = { "a": { "$in": [99, 100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 99, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // $nin, hit all dataGroups
   var ninDocs = tmpDocs( 100, 200 );
   var findCond = { "a": { "$nin": ninDocs } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 200 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $and, 子集相交
   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "a": { "$lte": 200 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $and, 子集包含
   var findCond = { "$and": [{ "a": { "$gte": 99 } }, { "a": { "$gte": 100 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $and, 子集相离
   var findCond = { "$and": [{ "a": { "$gt": 100 } }, { "a": { "$in": [100] } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), ['SYSCoord'] );

   // $and + other machers
   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "a": { "$isnull": 0 } }, { "a": { "$exists": 1 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "a": { "$isnull": 1 } }, { "a": { "$exists": 1 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), ['SYSCoord'] );

   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "a": { "$isnull": 0 } }, { "a": { "$exists": 0 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), ['SYSCoord'] );

   // $not, hit all dataGroups
   var findCond = { "$not": [{ "a": { "$gte": 100 } }, { "a": { "$lte": 200 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 201 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $or
   var findCond = { "$or": [{ "a": { "$lt": 100 } }, { "a": { "$gte": 200 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 200 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );

   // $mod, hit all dataGroups
   var findCond = { "a": { "$mod": [101, 100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( docs.slice( 201, 202 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // multi matches
   // $or + $and
   var findCond = {
      "$or": [
         { "$and": [{ "a": { "$gt": 0 } }, { "a": { "$lt": 100 } }] },
         { "$and": [{ "a": { "$gte": 200 } }, { "a": { "$lte": 300 } }] }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 1, 100 ).concat( docs.slice( 200, 301 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );

   // $not + $or + $and   
   var findCond = {
      "$not": [
         {
            "$or": [
               { "$and": [{ "a": { "$gt": 0 } }, { "a": { "$lt": 100 } }] },
               { "$and": [{ "a": { "$gte": 200 } }, { "a": { "$lte": 300 } }] }
            ]
         }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 1 ).concat( docs.slice( 100, 200 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // array
   var arrDocs = [{ "a": [99] }, { "a": [100] }, { "a": [200] }];
   cl.insert( arrDocs );
   // $all
   var findCond = { "a": { "$all": [100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( [{ "a": [100] }] ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

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
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[2]] );

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