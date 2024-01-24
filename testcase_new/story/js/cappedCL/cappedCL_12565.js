/************************************
*@Description: pop empty cappedCL
*@author:      liuxiaoxuan
*@createdate:  2017.8.28
*@testlinkCase: seqDB-12565
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_12565";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

   checkPopResult( dbcl, 0, 1 );
   checkPopResult( dbcl, 0, -1 );
   checkPopResult( dbcl, 100, -1 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}


function checkPopResult ( dbcl, logicalID, direction )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.pop( { LogicalID: logicalID, Direction: direction } ).toArray();
   } );
}