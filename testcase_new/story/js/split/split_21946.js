/******************************************************************************
@Description : seqDB-21946:range表，分区键为多字段正逆序混合，带匹配符查询
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

   var testcaseID = 21946;
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_" + testcaseID;
   var recsNum = 300;
   // ready docs
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }

   // create cl   
   commDropCL( db, csName, clName, true );
   var options = { "ShardingKey": { "a": 1, "b": -1 }, "ShardingType": "range", "Group": dataGroupNames[0] };
   var cl = commCreateCL( db, csName, clName, options, true );

   // insert
   cl.insert( docs );

   // subCL split to multi group, e.g: [min:max, 100:200), [100:200, 200:100), [200:100, max:min)
   // a: [min,100), [100,200), [200,max)
   // b: [max,200), [200,100), [100,min)
   cl.split( dataGroupNames[0], dataGroupNames[1], { "a": 100, "b": 200 }, { "a": 200, "b": 100 } );
   cl.split( dataGroupNames[0], dataGroupNames[2], { "a": 200, "b": 100 }, { "a": { "$maxKey": 1 }, "b": { "$minKey": 1 } } );

   // $gt   
   var findCond = { "a": { "$gt": 200 }, "b": { "$gt": 200 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[2]] );

   // $gte
   var findCond = { "a": { "$gte": 200 }, "b": { "$gt": 200 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $lt
   var findCond = { "a": { "$lt": 100 }, "b": { "$lt": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $lte
   var findCond = { "a": { "$lte": 100 }, "b": { "$lte": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // $et
   var findCond = { "a": { "$et": 100 }, "b": { "$et": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // $ne, hit all dataGroups
   var findCond = { "a": { "$ne": 100 }, "b": { "$ne": 100 } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 101 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $in
   var findCond = { "a": { "$in": [99, 100] }, "b": { "$in": [99, 100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 99, 101 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // $nin, hit all dataGroups
   var ninDocs = tmpDocs( 100, 200 );
   var findCond = { "a": { "$nin": ninDocs }, "b": { "$nin": ninDocs } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 200 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $and, 子集相交
   var findCond = { "$and": [{ "a": { "$gte": 100 } }, { "b": { "$lte": 200 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $and, 子集包含
   var findCond = { "$and": [{ "a": { "$gte": 200 } }, { "b": { "$gte": 201 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $and, 子集相离
   var findCond = { "$and": [{ "a": { "$gt": 100 } }, { "b": { "$in": [100] } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $and + other machers
   var findCond = { "$and": [{ "a": { "$gte": 100 }, "b": { "$isnull": 0 } }, { "a": { "$gte": 200 }, "b": { "$isnull": 0 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 200 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   var findCond = { "$and": [{ "a": { "$gte": 100 }, "b": { "$exists": 1 } }, { "a": { "$gte": 200 }, "b": { "$exists": 1 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 200 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // $or + other machers
   var findCond = { "$or": [{ "a": { "$gte": 100 }, "a": { "$isnull": 1 } }, { "a": { "$gte": 200 }, "b": { "$exists": 0 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), [] );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );

   // $not, hit all dataGroups
   var findCond = { "$not": [{ "a": { "$lt": 100 }, "b": { "$lt": 200 } }, { "a": { "$gte": 200 }, "b": { "$gte": 100 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $or
   var findCond = { "$or": [{ "a": { "$lt": 100 }, "b": { "$lt": 100 } }, { "a": { "$gt": 200 }, "b": { "$gt": 200 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 201 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );

   // $mod, hit all dataGroups
   var findCond = { "a": { "$mod": [101, 100] }, "b": { "$mod": [101, 100] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 101 ).concat( docs.slice( 201, 202 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // multi matches
   // $or + $and
   var findCond = {
      "$and": [
         { "$or": [{ "a": { "$gte": 200 }, "b": { "$gt": 200 } }, { "a": { "$lt": 100 }, "b": { "$lt": 200 } }] },
         { "a": { "$in": [99, 100] }, "b": { "$in": [99, 200] } }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 99, 100 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $not + $or + $and   
   var findCond = {
      "$not": [{
         "$and": [
            { "$or": [{ "a": { "$gte": 200 }, "b": { "$gt": 200 } }, { "a": { "$lt": 100 }, "b": { "$lt": 200 } }] }]
      }]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100, 201 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );


   // others
   var objDocs = [{ "a": { "a1": 1 }, "b": { "b1": 1 } },
   { "a": { "a1": 2 }, "b": { "b1": 2 } },
   { "a": { "a1": "test" }, "b": { "b1": "test" } }];
   cl.insert( objDocs );
   // $regex + $elemMatch, hit all dataGroups
   var findCond = {
      "a": { "$elemMatch": { "a1": { "$regex": "t.*t", "$options": "i" } } },
      "b": { "$elemMatch": { "b1": { "$regex": "t.*t", "$options": "i" } } }
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), objDocs.slice( 2 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), dataGroupNames );

   // $et + $field, hit all dataGroups
   var findCond = { "a": { "$et": { "$field": "b" } } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs );
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