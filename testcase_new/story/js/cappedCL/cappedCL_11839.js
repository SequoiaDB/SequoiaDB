/************************************
*@Description: 创建普通集合空间，并创建与该集合空间名同名的固定集合空间
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11839
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME + "_11839";

   //drop CS and create cappedCS
   commDropCS( db, csName, true, "drop CS in the beginning" );

   //create normal CS
   commCreateCS( db, csName, false, "beginning to create CS", null );

   //create capped CS
   var optionObj = { Capped: true };
   assert.tryThrow( SDB_DMS_CS_EXIST, function()
   {
      db.createCS( csName, optionObj );
   } );

   //clean environment after test
   commDropCS( db, csName, true, "drop CS in the end" );
}