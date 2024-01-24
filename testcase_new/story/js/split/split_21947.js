/******************************************************************************
@Description : seqDB-21947:range分区表，多个分区范围（如10个），带匹配符查询
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

   var testcaseID = 21947;
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_" + testcaseID;
   var recsNum = 1000;
   // ready docs
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }

   // create cl   
   commDropCL( db, csName, clName, true );
   var options = { "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": dataGroupNames[0] };
   var cl = commCreateCL( db, csName, clName, options, true );

   // insert
   cl.insert( docs );

   // subCL split to multi group, e.g: 
   // group1 [min,100), [300,400), [600,700)
   // group2 [100,200), [400,500), [700,800)
   // group3 [200,300), [500,600), [800,max)
   var range = 100;
   for( var i = 0; i < 3; i++ )
   {
      cl.split( dataGroupNames[0], dataGroupNames[1], { "a": range }, { "a": range + 100 } );
      range += 100;
      if( range < 800 )
      {
         cl.split( dataGroupNames[0], dataGroupNames[2], { "a": range }, { "a": range + 100 } );
         range += 100;
      }
      else 
      {
         cl.split( dataGroupNames[0], dataGroupNames[2], { "a": range }, { "a": { "$maxKey": 1 } } );
      }
      range += 100;
   }


   // hit multi range in the same group     
   // $in
   var findCond = { "a": { "$in": [400, 700] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 400, 401 ).concat( docs.slice( 700, 701 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // $and + $gte + $lt 
   var findCond = {
      "$or": [
         { "$and": [{ "a": { "$gte": 400 } }, { "a": { "$lt": 500 } }] },
         { "$and": [{ "a": { "$gte": 700 } }, { "a": { "$lt": 800 } }] }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 400, 500 ).concat( docs.slice( 700, 800 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // $or + $et
   var findCond = { "$or": [{ "a": 400 }, { "a": 700 }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 400, 401 ).concat( docs.slice( 700, 701 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // hit multi range in the different group
   // $and + $gt + $lte    
   var findCond = {
      "$or": [
         { "$and": [{ "a": { "$gt": 400 } }, { "a": { "$lte": 500 } }] },
         { "$and": [{ "a": { "$gt": 700 } }, { "a": { "$lte": 800 } }] }
      ]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 401, 501 ).concat( docs.slice( 701, 801 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1], dataGroupNames[2]] );

   // hit min and max range
   var findCond = { "$or": [{ "a": { "$lt": 100 } }, { "a": { "$gte": 800 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 0, 100 ).concat( docs.slice( 800 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[2]] );

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