/************************************
*@Description: 多次修改cs名
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16097
**************************************/

main( test );

function test ()
{
   //@ clean before
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var oldcsName = CHANGEDPREFIX + "_16097_oldcs";
   var newcsName = CHANGEDPREFIX + "_16097_newcs";
   var clName = CHANGEDPREFIX + "_16097_maincl";
   var subclName1 = CHANGEDPREFIX + "_16097_subcl1";
   var subclName2 = CHANGEDPREFIX + "_16097_subcl2";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var cl = commCreateCL( db, oldcsName, clName, { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true }, false, false, "create CL in the begin" );
   var subcl1 = commCreateCL( db, oldcsName, subclName1, { ShardingKey: { no: 1 }, ShardingType: "range" }, false, false, "create CL in the begin" );
   var subcl2 = commCreateCL( db, oldcsName, subclName2, { ShardingKey: { no: 1 }, ShardingType: "hash" }, false, false, "create CL in the begin" );
   cl.attachCL( oldcsName + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   cl.attachCL( oldcsName + "." + subclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   insertData( cl, 2000 );

   for( i = 0; i < 10; i++ )
   {
      db.renameCS( oldcsName, newcsName );
      cl = db.getCS( newcsName ).getCL( clName );
      cl.update( { $set: { c: "test1" } } );
      db.renameCS( newcsName, oldcsName );
      cl = db.getCS( oldcsName ).getCL( clName );
      cl.update( { $set: { c: "test2" } } );
   }

   db.renameCS( oldcsName, newcsName );
   checkRenameCSResult( oldcsName, newcsName, 2 );

   cl = db.getCS( newcsName ).getCL( clName );
   checkRecord( cl, 2000 );

   commDropCS( db, newcsName, true, "clean cs---" );
}

function checkRecord ( dbcl, recordNum )
{
   var actNum = dbcl.count( { c: "test2" } );
   assert.equal( actNum, recordNum );
}