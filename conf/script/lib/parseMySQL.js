import( catPath( getSelfPath(), 'parseSSQL.js' ) ) ;

function _string_encode_by_cmd( str ){
   str = strReplaceAll( str, "\\", "\\\\" ) ;
   str = strReplaceAll( str, '"', '\\"' ) ;
   return '"' + str + '"' ;
}

function _string_encode_by_no_quotes( str ){
   str = strReplaceAll( str, "\\", "\\\\" ) ;
   str = strReplaceAll( str, '"', '\\"' ) ;
   str = strReplaceAll( str, "'", "\\'" ) ;
   str = strReplaceAll( str, "!", "\\!" ) ;
   str = strReplaceAll( str, "&", "\\&" ) ;
   str = strReplaceAll( str, '(', '\\(' ) ;
   str = strReplaceAll( str, ')', '\\)' ) ;
   return str ;
}

function ExecSsql3( cmd, installPath, hostName, port, user, passwd, database, arg, timeout )
{
   var rc = SDB_OK ;
   var error = null ;
   var exec = installPath + '/bin/mysql' ;
   var result = {
      'rc': true,
      'field': {},
      'value': [],
      'attr': ''
   } ;

   if( isNaN( timeout ) )
   {
      timeout = 600000 ;
   }

   arg = _string_encode_by_cmd( arg ) ;
   user = _string_encode_by_cmd( user ) ;
   passwd = _string_encode_by_no_quotes( passwd ) ;

   arg = '-t -D ' + database + ' -h ' + hostName + ' -P ' + port + ' -e ' + arg + ' -u ' + user ;
   if( passwd && passwd.length > 0 )
   {
      exec = sprintf( 'export MYSQL_PWD=?;', passwd ) + exec ;
   }

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
   if( output.length == 0 )
   {
      return result ;
   }

   result = ParseSSQL( output ) ;
   if( result['rc'] != true )
   {
      error = new SdbError( SDB_SYS, "failed to exec sql" ) ;
      throw error ;
   }

   return result ;
}

function ExecSsql2( cmd, installPath, sockFile, user, database, arg, timeout )
{
   var rc = SDB_OK ;
   var error = null ;
   var exec = installPath + '/bin/mysql' ;
   var result = {
      'rc': true,
      'field': {},
      'value': [],
      'attr': ''
   } ;

   if( isNaN( timeout ) )
   {
      timeout = 600000 ;
   }

   user = _string_encode_by_cmd( user ) ;

   arg = _string_encode_by_cmd( arg ) ;
   arg = '-t -D ' + database + ' -S ' + sockFile + ' -u ' + user + ' -e ' + arg ;

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
      error = new SdbError( rc, arg ) ;
      throw error ;
   }

   var output = cmd.getLastOut() ;
   if( output.length == 0 )
   {
      return result ;
   }

   result = ParseSSQL( output ) ;
   if( result['rc'] != true )
   {
      error = new SdbError( SDB_SYS, "failed to exec sql" ) ;
      throw error ;
   }

   return result ;
}

function ExecSsql( cmd, installPath, port, user, passwd, database, arg, timeout )
{
   return ExecSsql3( cmd, installPath, '127.0.0.1', port, user, passwd, database, arg, timeout ) ;
}

/*
function test()
{
   var error = null ;
   var remote = new Remote( '192.168.3.232', '11790' ) ;
   var cmd = remote.getCmd() ;
   var installPath = '/opt/sequoiasql/mysql' ;
   var sql = '' ;

   try
   {
      var result = ExecSsql( cmd, installPath, "3306", "mysql", sql ) ;
      print( "\nResult:\n" + JSON.stringify( result, null, 3 ) + "\n\n" ) ;
   }
   catch( e )
   {
      print( "\nError:\n" + e.toString()+ "\n\n" );
   }
}

test() ;
*/

