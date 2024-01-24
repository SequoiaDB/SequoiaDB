/************************************
*@Description:  不同coord节点删除同一张子表
*@author:      wangkexin
*@createDate:  2019.3.12
*@testlinkCase: seqDB-10934
**************************************/
main( test );
function test ()
{
   var mainCL_Name = "maincl10934";
   var subCL_Name = "subcl10934";
   var clNum = 10;

   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //连接1挂载子表后删除子表
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var maincl = commCreateCL( db1, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
   commCreateCL( db1, COMMCSNAME, subCL_Name, subClOption, true, true );

   var options = { LowBound: { a: 1 }, UpBound: { a: 100 } };
   maincl.attachCL( COMMCSNAME + "." + subCL_Name, options );

   maincl.insert( { a: 10 } );

   db1.getCS( COMMCSNAME ).dropCL( subCL_Name );

   //连接2再次删除子表，并通过主表查询
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db2.getCS( COMMCSNAME ).dropCL( subCL_Name );
   } );


   var cursor = db2.getCS( COMMCSNAME ).getCL( mainCL_Name ).find();
   assert.equal( cursor.next(), undefined );

   //清除环境
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "drop CL in the end" );
}