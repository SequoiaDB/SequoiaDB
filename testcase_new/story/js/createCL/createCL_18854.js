/************************************
*@Description: seqDB-18854:创建cl，校验cl名相关限制
*@author:      wangkexin
*@createDate:  2019.7.17
*@testlinkCase: seqDB-18854
**************************************/
main( test );
function test ()
{
   var csName = "18854cs01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789length127B";
   var clName_126 = "18854cl0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678length126B";
   var clName_127 = clName_126 + "a";
   var cl_rename = "newcl18854";
   var cl_rename_127 = clName_126 + "b";
   var clNames = [clName_126, clName_127, cl_rename];

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning." );

   var cs = db.createCS( csName );
   for( var i = 0; i < clNames.length; i++ )
   {
      cs.createCL( clNames[i] );
      checkCL( cs, clNames[i] )
   }

   //rename collection
   cs.renameCL( cl_rename, cl_rename_127 );
   checkReNameCL( cs, cl_rename, cl_rename_127 );

   //清理环境
   commDropCS( db, csName, false, "drop CS in the end." );
}

function checkCL ( cs, clName )
{
   var cl = cs.getCL( clName );
   var records = [];
   for( var i = 0; i < 100; i++ )
   {
      records.push( { a: i, str: "test18824" } );
   }
   cl.insert( records );

   var count = cl.count();
   assert.equal( count, 100 );
}

function checkReNameCL ( cs, clName, renameClName )
{
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.getCL( clName );
   } );

   var renameCL = cs.getCL( renameClName );
   var count = renameCL.count();
   assert.equal( count, 100 );
}