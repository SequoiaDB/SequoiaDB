/************************************
*@Description: 创建固定集合，并创建索引
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11830
**************************************/

main( test );
function test ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11830";
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //createIndex
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.createIndex( "ageIndex", { age: 1 }, true );
   } );

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}