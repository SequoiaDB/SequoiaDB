/************************************
*@Description: 不同coord连接，在一个coord节点上attach完子表，马上在另一个coord节点上写入数据 
*@author:      wangkexin
*@createDate:  2019.3.16
*@testlinkCase: seqDB-65
**************************************/
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mainCL_Name = "maincl65";
   var subCL_Name = "subcl65";
   var coordGroup = commGetGroups( db, true, "SYSCoord", true, false, true );
   var nodeNum = eval( coordGroup[0].length - 1 );
   if( nodeNum < 2 )
   {
      return;
   }

   var hostname1 = coordGroup[0][1].HostName;
   var svcname1 = coordGroup[0][1].svcname;

   var hostname2 = coordGroup[0][2].HostName;
   var svcname2 = coordGroup[0][2].svcname;

   var db1 = new Sdb( hostname1, svcname1 );
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var maincl = commCreateCL( db1, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
   var subcl = commCreateCL( db1, COMMCSNAME, subCL_Name, subClOption, true, true );

   //挂载子表
   var options = { LowBound: { a: 1 }, UpBound: { a: 100 } };
   maincl.attachCL( COMMCSNAME + "." + subCL_Name, options );

   //连接另一个coord节点写入数据（范围内，范围外）
   var db2 = new Sdb( hostname2, svcname2 );
   var maincl2 = db2.getCS( COMMCSNAME ).getCL( mainCL_Name );
   insertDataAndCheck( maincl2 );

   //清除环境
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "drop CL in the end" );
}

function insertDataAndCheck ( cl )
{
   var insertObj = { a: 10 };
   cl.insert( insertObj );

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      cl.insert( { a: 200 } );
   } );

   //check
   assert.equal( 1, cl.count() );

   var cursor = cl.find( insertObj );
   assert.notEqual( cursor.next(), undefined );
   cursor.close();
}