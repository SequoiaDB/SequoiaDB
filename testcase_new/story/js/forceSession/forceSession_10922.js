/**************************************
 * @Description: seqDB-10922 :: 版本: 1 :: 指定sessionID不存在
 * @author: Zhao xiaoni
 * @Date: 2019-12-20
 *************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var sessionId = 1000000000;
   assert.tryThrow( SDB_PMD_SESSION_NOT_EXIST, function()
   {
      db.forceSession( sessionId );
   } );

   //force一个集群中存在的sessionid，但是options不存在
   var sessions = db.list( SDB_LIST_SESSIONS, {
      Status: { $ne: "Waiting" },
      Type: { $nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"] }
   } );
   var sessionId = sessions.next().toObj().SessionID;
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      db.forceSession( sessionId, { HostName: "sdbserver01", svcname: "21810" } );
   } );
}
