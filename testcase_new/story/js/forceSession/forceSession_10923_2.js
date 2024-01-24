/**************************************
 * @Description: 测试用例 seqDB-10923 : options参数非法校验
 * @author: Zhao xiaoni
 * @Date: 2019-12-20
 **************************************/
testConf.skipStandAlone = true;

//SEQUOIADBMAINSTREAM-5309
//main( test );

function test ()
{
   var group = commGetGroups( db )[0];
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;

   var options = { Host: "test" };
   forceSession( hostName, svcName, options );

   options = {};
   forceSession( hostName, svcName, options );
}

function forceSession ( hostName, svcName, options )
{
   var dataDB = new Sdb( hostName, svcName );
   var sessionID = dataDB.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj().SessionID;
   assert.tryThrow( SDB_PMD_SESSION_NOT_EXIST, function()
   {
      db.forceSession( sessionID, options );
   } );
}
