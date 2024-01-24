/************************************
*@Description: 分区表修改AutoSplit
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14961, seqDB-14962, seqDB-14963
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName1 = CHANGEDPREFIX + "_14961_1";
   var clName2 = CHANGEDPREFIX + "_14961_2";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 }, AutoSplit: false };
   var cl1 = commCreateCL( db, csName, clName1, options, true, false, "create CL in the begin" );

   var options2 = { ShardingType: 'hash', ShardingKey: { a: 1 }, AutoSplit: false };
   var cl2 = commCreateCL( db, csName, clName2, options2, true, false, "create CL in the begin" );
   for( i = 0; i < 5000; i++ )
   {
      cl2.insert( { a: i, b: "sequoiadh test hash cl1 alter option" } );
   }

   //修改AutoSplit值，hash表中无数据
   cl1.setAttributes( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName1, "AutoSplit", true );

   clSetAttributes( cl1, { AutoSplit: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName1, "AutoSplit", true );

   //修改AutoSplit值，hash表中有数据
   cl2.setAttributes( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName2, "AutoSplit", true );
   checkData( csName, clName2, 5000 );

   clSetAttributes( cl2, { AutoSplit: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName2, "AutoSplit", true );

   commDropCL( db, csName, clName1, true, false, "clean cl1" );
   commDropCL( db, csName, clName2, true, false, "clean cl1" );
}

function checkData ( csName, clName, expDataNum )
{
   var groupNameList = getGroupName( db );
   var actDataNum = 0;
   for( i = 0; i < groupNameList.length; i++ )
   {
      var groupName = groupNameList[i];
      var dataNode = new Sdb( db.getRG( groupName ).getMaster() );
      var checkCL = dataNode.getCS( csName ).getCL( clName );
      var recordNum = checkCL.count();
      actDataNum = actDataNum + recordNum;
      dataNode.close();
   }
   assert.equal( actDataNum, expDataNum );
}