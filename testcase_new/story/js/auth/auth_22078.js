/******************************************************************************
@Description : seqDB-22078:不鉴权执行node.connect()，创建鉴权用户后再次执行node.connect()
@Athor : XiaoNi Huang 2020-04-22
******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var testcaseID = 22078;
   var user1 = CHANGEDPREFIX + "_auth_" + testcaseID + "_1";
   var user2 = CHANGEDPREFIX + "_auth_" + testcaseID + "_2";
   var groupName = commGetDataGroupNames( db )[0];
   // clean
   cleanUsers( user1, user2 );

   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   try
   {
      // no auth user, node.connect
      var nodeDB = sdb.getRG( groupName ).getMaster().connect();
      nodeDB.close();

      // create user1
      sdb.createUsr( user1, user1 );
      // use old sdb
      var nodeDB = sdb.getRG( groupName ).getMaster().connect();
      nodeDB.close();
      // new db1 by user1
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME, user1, user1 );
      // use old sdb
      var nodeDB = sdb.getRG( groupName ).getMaster().connect();
      nodeDB.close();
      // use db1      
      var nodeDB = db1.getRG( groupName ).getMaster().connect();
      nodeDB.close();

      // create user2
      sdb.createUsr( user2, user2 );
      // use db1
      var nodeDB = db1.getRG( groupName ).getMaster().connect();
      nodeDB.close();
      // new db2 by user2
      var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME, user2, user2 );
      // use db2
      var nodeDB = db2.getRG( groupName ).getMaster().connect();
      nodeDB.close();

      // drop user1
      sdb.dropUsr( user1, user1 );
      // use db1
      assert.tryThrow( SDB_AUTH_AUTHORITY_FORBIDDEN, function()
      {
         db1.getRG( groupName ).getMaster().connect();
      } );
      // use db2
      var nodeDB = db2.getRG( groupName ).getMaster().connect();
      nodeDB.close();

      // drop all users
      sdb.dropUsr( user2, user2 );
      // use old sdb
      var nodeDB = sdb.getRG( groupName ).getMaster().connect();
      nodeDB.close();
      // use db1      
      var nodeDB = db1.getRG( groupName ).getMaster().connect();
      nodeDB.close();
      // use db2
      var nodeDB = db2.getRG( groupName ).getMaster().connect();
      nodeDB.close();
   }
   finally 
   {
      cleanUsers( user1, user2 );
      sdb.close();
   }
}

function cleanUsers ( user1, user2 )
{
   var users = [user1, user2];
   for( var i = 0; i < users.length; i++ )
   {
      try
      {
         db.dropUsr( users[i], users[i] );
      }
      catch( e )
      {
         if( e.message != SDB_AUTH_USER_NOT_EXIST )
         {
            throw new Error( e );
         }
      }
   }
}
