/************************************
*@Description: createDomain，name长度无效_st.verify.domain.004
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4487
**************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = [groupsArray[0][0].GroupName];
   var domainName = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890test4487";

   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      db.createDomain();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDomain( "", groupName );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDomain( domainName );
   } );
}
