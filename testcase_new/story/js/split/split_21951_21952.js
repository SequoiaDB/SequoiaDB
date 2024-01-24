/************************************
*@description： seqDB-21951:主子表，主表查询命中多个子表且命中同一个数据组
                seqDB-21952:主子表，主表查询的数据未命中子表
*@author ：2020-4-22 liyuanyue
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   dataGroupNames.sort();
   var testcaseID = "21951_21952";
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

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mcl = commCreateCL( db, csName, mclName, options, false );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   var scl1 = commCreateCL( db, csName, sclName1, options, false );
   var scl2 = commCreateCL( db, csName, sclName2, options, false );
   mcl.attachCL( sclFullName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mcl.attachCL( sclFullName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i } );
   }
   mcl.insert( docs );

   // scl1 [min,50) [50,max)
   // scl2 [min,50) [50,max)
   scl1.split( dataGroupNames[0], dataGroupNames[1], { "a": 50 }, { "a": { "$maxKey": 1 } } );
   scl2.split( dataGroupNames[0], dataGroupNames[1], { "a": 50 }, { "a": { "$maxKey": 1 } } );

   // 主表查询命中多个子表且命中同一个数据组
   var findCond = { "a": { "$in": [50, 150] } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 50, 51 ).concat( docs.slice( 150, 151 ) ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName2, sclFullName1]] );

   // 主表查询的数据未命中子表
   var findCond = { "a": 2000 };
   commCompareResults( mcl.find( findCond ), [] );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), ['SYSCoord'] );

   commDropCL( db, csName, mclName, false );
}