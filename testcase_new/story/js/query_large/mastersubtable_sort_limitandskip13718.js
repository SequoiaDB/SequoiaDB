/*******************************************************************************
*@Description:   seqDB-13718:主子表上sort+limit+skip执行查询
*@Author:        2019-5-14  wangkexin
********************************************************************************/
main( test );

function test ()
{
   rownums = 100000;
   var mainclName = "maincl13718";
   var subclName1 = "subcl13718a";
   var subclName2 = "subcl13718b";

   if( commIsStandalone( db ) )
   {
      return;
   }

   //less two groups to split
   var allGroupName = commGetGroups( db );
   if( 2 > allGroupName.length )
   {
      return;
   }

   var opt = new Object();
   opt.ReplSize = 0;
   opt.ShardingType = "range";
   opt.ShardingKey = { a: 1 };
   //subcl1:{a:[0-49999]}, subcl2:{a:[50000-99999]}
   var subcl1 = commCreateCL( db, COMMCSNAME, subclName1, opt, true );
   var subcl2 = commCreateCL( db, COMMCSNAME, subclName2, opt, true );

   opt.IsMainCL = true;
   var maincl = commCreateCL( db, COMMCSNAME, mainclName, opt, true );
   maincl.attachCL( COMMCSNAME + '.' + subclName1, { LowBound: { a: 0 }, UpBound: { a: 50000 } } )
   maincl.attachCL( COMMCSNAME + '.' + subclName2, { LowBound: { a: 50000 }, UpBound: { a: 100000 } } )

   var insetRecs = loadDataAndCreateIndex( maincl, rownums );
   getTwoGroupSplit( db, COMMCSNAME, subclName1, 50 );
   getTwoGroupSplit( db, COMMCSNAME, subclName2, 50 );

   //query 1 使用sort执行查询
   var sel_1 = maincl.find().sort( { a: 1 } );
   checkRec( sel_1, insetRecs );

   //query 2 使用limit执行查询
   var sel_2 = maincl.find().limit( 500 );
   checkResultNum( sel_2, 500 );

   //query 3 使用skip执行查询,覆盖表扫描和索引扫描  
   //a.使某个分区组一次返回的记录数小于skip的数目，这里指定skip为1000，2000
   var sel_3_1_table = maincl.find().skip( 1000 ).hint( { "": null } );
   checkResultNum( sel_3_1_table, rownums - 1000 );

   var sel_3_1_index = maincl.find().skip( 2000 ).hint( { "": "aIndex" } );
   checkResultNum( sel_3_1_index, rownums - 2000 );

   //b.使某个分区组一次返回的记录数大于skip的数目，这里指定skip为1
   var sel_3_2_table = maincl.find().skip( 1 ).hint( { "": null } );
   checkResultNum( sel_3_2_table, rownums - 1 );

   var sel_3_2_index = maincl.find().skip( 1 ).hint( { "": "aIndex" } );
   checkResultNum( sel_3_2_index, rownums - 1 );

   //query 4 使用sort+limit+skip执行查询，覆盖表扫描和索引扫描
   //a.使某个分区组一次返回的记录数小于skip的数目，这里指定skip为1000，3000
   var sel_4_1_table = maincl.find().sort( { a: 1 } ).limit( 1500 ).skip( 1000 ).hint( { "": null } );
   var expRec = getExpRec( insetRecs, 1000, 2499 );
   checkRec( sel_4_1_table, expRec );

   var sel_4_1_index = maincl.find().sort( { a: 1 } ).limit( 1500 ).skip( 3000 ).hint( { "": "aIndex" } );
   var expRec = getExpRec( insetRecs, 3000, 4499 );
   checkRec( sel_4_1_index, expRec );

   //b.使某个分区组一次返回的记录数大于skip的数目，这里指定skip为1
   var sel_4_2_table = maincl.find().sort( { a: 1 } ).limit( 1500 ).skip( 1 ).hint( { "": null } );
   var expRec = getExpRec( insetRecs, 1, 1500 );
   checkRec( sel_4_2_table, expRec );

   var sel_4_2_index = maincl.find().sort( { a: 1 } ).limit( 1500 ).skip( 1 ).hint( { "": "aIndex" } );
   var expRec = getExpRec( insetRecs, 1, 1500 );
   checkRec( sel_4_2_index, expRec );

   //drop cl
   commDropCL( db, COMMCSNAME, mainclName, false, false, "drop cl in the end" );
}

function loadDataAndCreateIndex ( cl, rownums )
{
   var record = [];
   for( var i = 0; i < rownums; i++ )
   {
      record.push( { _id: i, a: i, b: i, c: i } );
   }
   cl.insert( record );
   cl.createIndex( "aIndex", { a: 1 }, true );
   return record;
}

function checkResultNum ( sel, expResultNum )
{
   var act_resurnednum = sel.size();
   assert.equal( act_resurnednum, expResultNum );
}

function getExpRec ( record, start, end )
{
   var expRec = [];
   for( var i = start; i <= end; i++ )
   {
      expRec.push( record[i] );
   }
   return expRec;
}
function getTwoGroupSplit ( db, csName, clName, splitArg1, splitArg2 )
{
   // get collection
   var cs = db.getCS( csName );
   var cl = cs.getCL( clName );

   var listGroups = db.listReplicaGroups();
   var listGroupsArr = new Array();

   // Check over arguement "splitArg1" "splitArg2"
   if( "" == splitArg1 || undefined == splitArg1 )
   {
      throw new Error( "ErrArg" );
   }

   // argument : when the split is percent
   var argument = "";
   if( undefined == splitArg2 || "" == splitArg2 ) { argument = splitArg1; }
   // Get group where Collection Space located in
   while( listGroups.next() )
   {
      if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
      {
         listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
      }
   }
   var groupNum = listGroupsArr.length;

   var snapShotCL = db.snapshot( SDB_SNAP_COLLECTIONS );
   var snapShotClName = new Array();
   var snapShotClGroup = new Array();
   var group = "";
   while( snapShotCL.next() )
   {
      snapShotClName.push( snapShotCL.current().toObj()["Name"] );
      snapShotClGroup.push( snapShotCL.current().toObj()["Details"][0]["GroupName"] );
   }
   for( var i = 0; i < snapShotClGroup.length; i++ )
   {
      if( snapShotClName[i] == csName + "." + clName )
      {
         group = snapShotClGroup[i].toString();
         break;
      }
   }
   if( "" == group )
   {
      throw new Error( "Failed to get Group where CL located in, snapShotCL = " + snapShotCL );
   }
   // Get the other group where split to
   var groupSplit = "";
   var i = 0;
   do
   {
      if( group != listGroupsArr[i] )
      {
         groupSplit = listGroupsArr[i];
         break;
      }
      ++i;

   } while( i <= groupNum || i <= 8 );

   if( "" == groupSplit )
   {
      throw new Error( "Failed to get Split Group, Groups = " + listGroups );
   }
   if( "" == argument )
      cl.split( group, groupSplit, splitArg1, splitArg2 );
   else
      cl.split( group, groupSplit, argument );
}