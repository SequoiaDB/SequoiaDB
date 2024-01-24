/************************************
*@Description:  指定2个参数组合收集统计信息、清空缓存 
*@author:      liuxiaoxuan
*@createdate:  2017.11.11
*@testlinkCase: seqDB-11637
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "11637";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl	
   var clName = COMMCLNAME + "11637";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //get master/slave datanode          
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //get Group and Node info
   var groupName = commGetCLGroups( db, csName + "." + clName );
   var groupDetail = commGetGroups( db, false, groupName[0] );

   var groupId = groupDetail[0][0].GroupID;
   var priNodeId = groupDetail[0][0].PrimaryNode;

   //check before analyze success
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //check the query explain before analyze
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


   //analyze with cs+group
   var options = { CollectionSpace: csName, GroupID: groupId };
   db.analyze( options );

   //check after analyze success with cs+group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];


   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );


   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   db.analyze( options );

   //check after truncate   
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

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

   //analyze with cs+node
   var options = { CollectionSpace: csName, NodeID: priNodeId };
   db.analyze( options );

   //check after analyze success with cs+node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );


   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   db.analyze( options );

   //check after truncate  
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

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

   //analyze with cl+group
   var options = { Collection: csName + "." + clName, GroupName: groupName[0] };
   db.analyze( options );

   //check after analyze success with cl+group
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );


   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   db.analyze( options );

   //check after truncate      
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

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

   //analyze with cl+node
   var options = { Collection: csName + "." + clName, NodeID: priNodeId };
   db.analyze( options );

   //check after analyze success with cl+node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query                            
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );


   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   db.analyze( options );

   //check after truncate   
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

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

   //analyze with group+node
   var options = { GroupID: groupId, NodeID: priNodeId };
   db.analyze( options );

   //check after analyze success with group+node
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //check the query explain after analyze
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //query                            
   query( dbclPrimary, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );


   //truncate analyze info
   var options = { Mode: 3, Collection: csName + "." + clName };
   db.analyze( options );

   //check after truncate      
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

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

   //check invalid analyze, cs+cl, cs+index, index+group, index+node
   var options = [{ CollectionSpace: csName, Collection: csName + "." + clName },
   { CollectionSpace: csName, Index: "a" },
   { Index: "a", GroupID: groupId },
   { Index: "a", GroupName: groupName[0] },
   { Index: "a", NodeID: priNodeId }];

   for( var i in options )
   {
      checkAnalyzeInvalidResult( options[i] );
   }


   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeInvalidResult ( options )
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
