/******************************************************************************
 * @Description   :  seqDB-28010 :: 创建监控用户执行内置sql监控类操作 
 * @Author        : Wu Yan
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2022.10.08
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   try
   {
      var adminUser = "admin28010"
      var userName = "user28010";
      var passwd = "passwd28010";
      db.createUsr( adminUser, passwd, { Role: "admin" } );
      db.createUsr( userName, passwd, { Role: "monitor" } );
      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );
      var adminDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );

      //快照操作
      execSnapshot( authDB );

      //列表操作
      execList( authDB );

      //列取cs和cl
      //TODO:http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-8825
      //authDB.exec( "list collectionspaces" );
      //authDB.exec( "list collections" )

   }
   finally
   {
      adminDB.dropUsr( userName, passwd );
      adminDB.dropUsr( adminUser, passwd );
      authDB.close();
      adminDB.close();
   }
}

function execSnapshot ( sdb )
{
   var snapshotType = ["$SNAPSHOT_CONTEXT", "$SNAPSHOT_CONTEXT_CUR", "$SNAPSHOT_SESSION", "$SNAPSHOT_SESSION_CUR", "$SNAPSHOT_CL",
      "$SNAPSHOT_CS", "$SNAPSHOT_DB", "$SNAPSHOT_SYSTEM", "$SNAPSHOT_CATA", "$SNAPSHOT_TRANS", "$SNAPSHOT_TRANS_CUR",
      "$SNAPSHOT_ACCESSPLANS", "$SNAPSHOT_HEALTH", "$SNAPSHOT_CONFIGS", "$SNAPSHOT_SVCTASKS", "$SNAPSHOT_SEQUENCES", "$SNAPSHOT_LATCHWAITS", "$SNAPSHOT_LOCKWAITS",
      "$SNAPSHOT_QUERIES", "$SNAPSHOT_INDEXSTATS", "$SNAPSHOT_TRANSWAIT", "$SNAPSHOT_TRANSDEADLOCK"];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      println( "---exec snapshot is " + type )
      sdb.exec( "select * from " + type );
   }
}

function execList ( sdb )
{
   var listType = ["$LIST_CONTEXT", "$LIST_CONTEXT_CUR", "$LIST_SESSION", "$LIST_SESSION_CUR", "$LIST_CL", "$LIST_CS",
      "$LIST_SU", "$LIST_GROUP", "$LIST_TRANS", "$LIST_TRANS_CUR", "$LIST_SVCTASKS", "$LIST_SEQUENCES", "$LIST_USER",
      "$LIST_BACKUP", "$LIST_DATASOURCE"];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      println( "---exec list is " + type );
      sdb.exec( "select * from " + type );
   }
}






