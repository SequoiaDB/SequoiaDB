/* *****************************************************************************
@discretion: Prepare before all test-case
@modify list:
   2014-3-1 Jianhui Xu  Init
***************************************************************************** */

var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

try
{
   main( db );
}
catch( e )
{
   println( "Before all test-cases environment prepare failed: " + e );
}

function main ( db )
{
   // 0. 生成 basic_operation 目录下的文件
   var cmd = new Cmd();
   var isMtRuntest = File.exist( "./local_test_report/.generateFiles.lock" );
   if( isMtRuntest )
   {
      // mt-runtest.sh
      // 当拿不到锁，会一直阻塞，拿到锁后，发现已经生成过一次，不再生成文件
      // grep 1 ./local_test_report/.generateFiles.lock 1>/dev/null 
      // || ( bin/sdb -f testcase_new/story/js/lib/generateFiles.js && echo 1 > ./local_test_report/.generateFiles.lock )
      try
      {
         cmd.run( "flock -x ./local_test_report/.generateFiles.lock -c ", "  \" grep 1 ./local_test_report/.generateFiles.lock 1>/dev/null || (  bin/sdb -f testcase_new/story/js/lib/generateFiles.js && echo 1 > ./local_test_report/.generateFiles.lock ) \" " );
      }
      catch( e )
      {

      }
   }
   else
   {
      // runtest.sh and CI 
      cmd.run( getExePath() + "/sdb -f " + getSelfPath() + "/generateFiles.js" );
   }

   // 1. 删除名称含 local_test 的 cs
   var cols = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES, { "Name": Regex( CHANGEDPREFIX, "i" ) }, { "Name": "" } );
   for( var i = 0; i < cols.length; ++i )
   {
      commDropCS( db, cols[i].Name, true, " before all test-cases" );
   }

   // 2. 创建 dummycl 
   var opt = {};
   if( !commIsStandalone( db ) )
   {
      opt = { ShardingKey: { a: 1 }, ShardingType: 'hash', AutoSplit: true };
   }
   commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME, opt, true, true, "Create dummy collection" );

   // 3. 创建临时目录 /tmp/jstest
   commMakeDir( COORDHOSTNAME, WORKDIR );

   // 4. 删除名称含 local_test 的 backup
   var backups = commGetBackups( db, CHANGEDPREFIX );
   for( var j = 0; j < backups.length; ++j )
   {
      try
      {
         db.removeBackup( { "Name": backups[j] } );
      }
      catch( e )
      {
         println( "Drop backup " + backups[j] + " failed before test-case: " + e );
      }
   }

   // 5. 删除名称含 local_test 的 domain
   var cursor = db.listDomains( { "Name": Regex( CHANGEDPREFIX, "i" ) }, { "Name": "" } );
   var domains = commCursor2Array( cursor );
   for( var j = 0; j < domains.length; ++j )
   {
      commDropDomain( db, domains[i].Name );
   }

   // 6. 删除名称含 local_test 的 procedure
   var procedures = commGetProcedures( db, CHANGEDPREFIX );
   for( var j = 0; j < procedures.length; ++j )
   {
      try
      {
         db.removeProcedure( procedures[j] );
      }
      catch( e )
      {
         println( "Drop procedure " + procedures[j] + " failed before test-case: " + e );
      }
   }


}


