/************************************
*@Description: 创建固定集合，并进行split操作
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11829
**************************************/

main( test );
function test ()
{
   //standalone can not split
   if( true === commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = commGetGroups( db, "getGroups", "", true, true, true );
   if( 1 >= allGroupName.length )
   {
      return;
   }

   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11829";
   var srcGroup = allGroupName[0][0].GroupName;
   var tarGroup = allGroupName[1][0].GroupName;
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, Group: srcGroup };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //hash split
   assert.tryThrow( SDB_COLLECTION_NOTSHARD, function()
   {
      dbcl.split( srcGroup, tarGroup, { Partition: 10 }, { Partition: 20 } );
   } );

   //range split
   assert.tryThrow( SDB_COLLECTION_NOTSHARD, function()
   {
      dbcl.split( srcGroup, tarGroup, { a: 10 }, { a: 10000 } );
   } );

   //percent split
   assert.tryThrow( SDB_COLLECTION_NOTSHARD, function()
   {
      dbcl.split( srcGroup, tarGroup, 50 );
   } );

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}