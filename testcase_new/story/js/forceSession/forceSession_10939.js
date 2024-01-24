/**************************************
 * @Description: seqDB-10939: 指定options参数为GroupID/GroupName
 * @author: Zhao xiaoni 
 * @Date: 2019-12-20
 **************************************/
//若后续跑此用例出现杀错会话，可删除此用例，因为options不能精确匹配sessionID
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var group = commGetGroups( db )[0];
   var groupName = group[0].GroupName;
   var groupID = group[0].GroupID;
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;

   var options = { "GroupName": groupName };
   forceSession( hostName, svcName, options );

   options = { "GroupID": groupID };
   forceSession( hostName, svcName, options );
}

function forceSession ( hostName, svcName, options )
{
   var dataDB = new Sdb( hostName, svcName );
   var sessionID = dataDB.list( 3 ).next().toObj().SessionID;
   try
   {
      db.forceSession( sessionID, options );
   }
   catch( e )
   {
      if( e.message != SDB_COORD_NOT_ALL_DONE )
      {
         throw e;
      }
   }

   var res = db.list( 3, { SessionId: sessionID } ).toArray();
   assert.equal( res.length, 0 );
}
