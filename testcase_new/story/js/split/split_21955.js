/******************************************************************************
@Description : seqDB-21955:hash分区表，多个分区范围（如10个），带匹配符查询
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

   var testcaseID = 21955;
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
   var options = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Partition": 1024, "Group": dataGroupNames[0] };
   var cl = commCreateCL( db, csName, clName, options, true );

   // insert
   cl.insert( docs );

   // subCL split to multi group, e.g: 
   // group1 [0,100), [300,400), [600,700)
   // group2 [100,200), [400,500), [700,800)
   // group3 [200,300), [500,600), [800,1024)
   var part = 100;
   for( var i = 0; i < 3; i++ )
   {
      cl.split( dataGroupNames[0], dataGroupNames[1], { "Partition": part }, { "Partition": part + 100 } );
      part += 100;
      if( part < 800 )
      {
         cl.split( dataGroupNames[0], dataGroupNames[2], { "a": part }, { "a": part + 100 } );
         part += 100;
      }
      else 
      {
         cl.split( dataGroupNames[0], dataGroupNames[2], { "a": part }, { "a": 1024 } );
      }
      part += 100;
   }

   // 手工测试时创建10个组，查询数据落的数据组，自动化时相同的分区范围数据一样的，由此方法判断同一个数据组不同分区范围的数据
   // 如：10个组时{a:1}、{a:10}分别落在group1、group10，即[0,100)、[300,400)，自动化时也是落在这2个分区
   // hit multi range in the same group
   // $in
   var findCond = { "a": { "$in": [14, 15, 16] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 14, 17 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $and + $gte + $lt 
   var findCond = {
      "$and": [{ "$or": [{ "a": 14 }, { "a": 15 }, { "a": 16 }] },
      { "$and": [{ "a": { "$gte": 14 } }, { "a": { "$lte": 16 } }] }]
   };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 14, 17 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // $or + $et
   var findCond = { "$or": [{ "a": { "$et": 15 } }, { "a": { "$et": 16 } }] };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 15, 17 ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0]] );

   // hit multi range in the different group
   var findCond = { "a": { "$in": [15, 16, 91, 92] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 15, 17 ).concat( docs.slice( 91, 93 ) ) );
   checkHitDataGroups( cl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // hit min and max range
   var findCond = { "a": { "$in": [2, 16] } };
   commCompareResults( cl.find( findCond ).sort( { "a": 1 } ), docs.slice( 2, 3 ).concat( docs.slice( 16, 17 ) ) );
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