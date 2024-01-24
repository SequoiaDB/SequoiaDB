/************************************
*@Description: 修改cs名，新名参数校验
*@author:      luweikang
*@createdate:  2018.12.13
*@testlinkCase:seqDB-16552
**************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME + "_16552";

   var cs = commCreateCS( db, csName, false, "create cs in begine" );

   // rename cs new name is begin with $
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, "$csName16552" );
   } );

   // rename cs new name is contains .
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, "csName.16552" );
   } );

   // rename cs new name is ""
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, "" );
   } );

   // rename cs new name is long str
   var longStr = "a";
   for( var i = 0; i < 1000; i++ )
   {
      longStr += "a";
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, longStr );
   } );

   // rename cs new name is 128 str
   var boundStr = "";
   for( var i = 0; i < 128; i++ )
   {
      boundStr += "s";
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, boundStr );
   } );

   // rename cs new name is 127 str
   var shotStr = "";
   for( var i = 0; i < 127; i++ )
   {
      shotStr += "a";
   }
   db.renameCS( csName, shotStr );
   db.renameCS( shotStr, csName );

   // rename cs new name is contains ~!@#$%^()_+
   var nameStr = "~!@#$%^()_+"
   db.renameCS( csName, nameStr );
   db.renameCS( nameStr, csName );

   // rename cs new name is begin with SYS
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameCS( csName, "SYScsName16552" );
   } );

   commDropCS( db, csName, true, "clean cs---" );
}