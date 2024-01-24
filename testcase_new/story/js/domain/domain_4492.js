/************************************
*@Description: createDomain，options:AutoSplit取值非法_st.verify.domain.009
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4492
**************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = [groupsArray[0][0].GroupName];
   var domainName = "domain4492";

   checkInvalidAutoSplit( domainName, groupName, "test_4492" );
   checkInvalidAutoSplit( domainName, groupName, "" );
}

function checkInvalidAutoSplit ( domainName, groupName, autosplit )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDomain( domainName, groupName, { AutoSplit: autosplit } );
   } );
}