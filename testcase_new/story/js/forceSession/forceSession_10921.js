/**************************************
 * @Description: seqDB-10921 :: 版本: 1 :: 指定sessionID为系统EDU
 * @author: Zhao xiaoni 
 * @Date: 2019-12-20
/***************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // 获取所有系统EDU类型的session，并随机从中取得一个用于force(Name为DATASYNC-JOB-D的系统会话可以被强杀)
   var sessions = db.list( 2, {
      Global: true, Status: { $ne: "Waiting" },
      Name: { $nin: ["DATASYNC-JOB-D"] },
      Type: { $nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"] }
   } );
   var obj = sessions.next().toObj();
   var sessionID = obj.SessionID;

   //list加条件是因为通过sessionid有可能也能查到非系统EDU类型的session
   var expResult = db.list( 2, {
      Global: true, SessionID: sessionID, Status: { $ne: "Waiting" },
      Name: { $nin: ["DATASYNC-JOB-D"] },
      Type: { $nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"] }
   } ).toArray();

   assert.tryThrow( [SDB_PMD_FORCE_SYSTEM_EDU, SDB_COORD_NOT_ALL_DONE], function()
   {
      db.forceSession( sessionID, { Global: true } );
   } );

   var actResult = db.list( 2, {
      Global: true, SessionID: sessionID, Status: { $ne: "Waiting" },
      Name: { $nin: ["DATASYNC-JOB-D"] },
      Type: { $nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"] }
   } ).toArray();
   if( expResult.length !== actResult.length )
   {
      throw new Error( "expResult: " + JSON.stringify( expResult ) + "\nactResult: " + JSON.stringify( actResult ) + "\nsession info: " + JSON.stringify( obj ) );
   }
}
