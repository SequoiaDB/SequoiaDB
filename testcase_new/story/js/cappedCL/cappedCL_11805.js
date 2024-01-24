/************************************
*@Description:capped cl findandUpdate/findandRemove
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11805
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11805";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      dbcl.find().update( { $set: { a: 1 } } ).toArray();
   } );

   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      dbcl.find().remove().toArray();
   } );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}