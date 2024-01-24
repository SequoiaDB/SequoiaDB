/************************************
*@Description:  指定id索引收集统计信息
*@author:      liuxiaoxuan
*@createdate:  2017.11.10
*@testlinkCase: seqDB-11617
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "11617";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { PageSize: 4096 };
   commCreateCS( db, csName, false, "", csOption );

   //create cl
   var clName = COMMCLNAME + "11617";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //insert
   var insertNums = 5000;
   insertDatas( dbcl, insertNums );

   //check before invoke analyze
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$id", false, false );

   //check the query explain of master/slave nodes
   var findConf = { _id: 4000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "$id", ReturnNum: 1 }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, 1 );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$id" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   //invoke analyze
   var options = { Collection: csName + "." + clName, Index: "$id" };
   db.analyze( options );

   //check after analyze
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$id", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain of master/slave nodes
   var findConf = { _id: 4000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "$id", ReturnNum: 1 }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, 1 );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$id" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function insertDatas ( dbcl, insertNum )
{
   var doc = [];
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push( { _id: i, a: "test" + i } );
   }
   dbcl.insert( doc );

}