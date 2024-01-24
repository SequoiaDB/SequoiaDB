/************************************
*@Description:  不指定cl和index生成默认统计信息
*@author:      liuxiaoxuan
*@createdate:  2017.11.10
*@testlinkCase: seqDB-11622
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "11622";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl
   var clName = COMMCLNAME + "11622";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;
   insertDiffDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //check before invoke analyze
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //check the query explain of master/slave nodes
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //analyze with Mode:3
   var groupName = commGetCLGroups( db, csName + "." + clName );
   var groupDetail = commGetGroups( db, false, groupName );
   var priNode = groupDetail[0][0].PrimaryNode;

   var options = [{ Mode: 3 },
   { Mode: 3, CollectionSpace: csName },
   { Mode: 3, GroupName: groupName },
   { Mode: 3, NodeID: priNode }]

   for( var i in options )
   {
      checkAnalyze( options[i] );
   }

   //check after analyze
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain of master/slave nodes
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyze ( options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }
}

