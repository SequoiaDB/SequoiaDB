import( catPath( getSelfPath(), 'parseSSQL.js' ) ) ;

function ExecSsql( cmd, installPath, port, database, arg, timeout )
{
   var rc = SDB_OK ;
   var error = null ;
   var exec = installPath + '/bin/psql' ;
   //set LD_LIBRARY_PATH
   //export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/sequoiapostgresql/lib
   var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;

   libraryCmd += installPath + '/lib ;' ;

   exec = libraryCmd + exec ;

   if( isNaN( timeout ) )
   {
      timeout = 600000 ;
   }

   arg = database + ' -p ' + port + ' -c "' + arg + '" -P "border=2"' ;

   try
   {
      cmd.run( exec, arg, timeout ) ;
      rc = cmd.getLastRet() ;
      if( rc )
      {
         error = new SdbError( rc, cmd.getLastOut() ) ;
         throw error ;
      }
   }
   catch( e )
   {
      rc = cmd.getLastRet() ;
      error = new SdbError( rc, cmd.getLastOut() ) ;
      throw error ;
   }

   var output = cmd.getLastOut() ;
   var result = ParseSSQL( output ) ;
   if( result['rc'] != true )
   {
      error = new SdbError( SDB_SYS, "failed to exec sql" ) ;
      throw error ;
   }

   return result ;
}
/*
function test()
{
   var error = null ;
   var remote = new Remote( 'localhost', '11790' ) ;
   var cmd = remote.getCmd() ;
   var installPath = '/opt/sequoiapostgresql' ;
   var sql = '\\d' ;

   try
   {
      var result = ExecSsql( cmd, installPath, "5432", "postgres", sql ) ;
      print( "\nResult:\n" + JSON.stringify( result, null, 3 ) + "\n\n" ) ;
   }
   catch( e )
   {
      print( "\nError:\n" + e.toString()+ "\n\n" );
   }
}

test() ;
*/

