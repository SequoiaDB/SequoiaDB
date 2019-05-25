function _sqlStrlen( str )
{
   var len = 0;
   for ( var i = 0; i < str.length; ++i )
   {
      var c = str.charCodeAt( i ) ;
      if( ( c >= 0x0001 && c <= 0x007e ) || ( 0xff60 <= c && c <= 0xff9f ) )
      {
         ++len ;
      }
      else
      {
         len += 2 ;
      }
   }

   return len ;
}

function trim( str )
{
   if( typeof( str ) == 'string' )
   {
      return str.replace( /(^\s*)|(\s*$)/g, '' ) ;
   }
   return str ;
}

var _successOperate = [
   'ABORT',
   'ALTER ',
   'ANALYZE',
   'BEGIN',
   'CHECKPOINT',
   'CLOSE',
   'CLUSTER',
   'COMMENT',
   'COMMIT',
   'COMMIT PREPARED',
   'COPY',
   'CREATE ',
   'DEALLOCATE',
   'DECLARE',
   'DELETE',
   'DISCARD',
   'DO',
   'DROP ',
   'END',
   'EXECUTE',
   'EXPLAIN',
   'FETCH',
   'GRANT',
   'INSERT',
   'LISTEN',
   'LOAD',
   'LOCK',
   'MOVE',
   'NOTIFY',
   'PREPARE',
   'PREPARE TRANSACTION',
   'REASSIGN OWNED',
   'REFRESH MATERIALIZED VIEW',
   'REINDEX',
   'RELEASE SAVEPOINT',
   'RESET',
   'REVOKE',
   'ROLLBACK',
   'ROLLBACK PREPARED',
   'ROLLBACK TO SAVEPOINT',
   'SAVEPOINT',
   'SECURITY LABEL',
   'SELECT',
   'SELECT INTO',
   'SET',
   'SET CONSTRAINTS',
   'SET ROLE',
   'SET SESSION AUTHORIZATION',
   'SET TRANSACTION',
   'SHOW',
   'START TRANSACTION',
   'TABLE',
   'TRUNCATE',
   'UNLISTEN',
   'UPDATE',
   'VACUUM',
   'VALUES',
   'WITH',
   'No relations found.'
] ;

function _parseSSQLResult( result, state )
{
   if( state['status'] != 8 )
   {
      if( result.indexOf( 'ERROR:' ) == 0 )
      {
         state['status'] = 8 ;
         state['rc'] = false ;
      }
      else if( result.indexOf( 'psql:' ) == 0 )
      {
         state['status'] = 8 ;
         state['rc'] = false ;
      }
      else
      {
         var isOperate = false ;

         for( var i in _successOperate )
         {
            if( result.indexOf( _successOperate[i] ) == 0 )
            {
               state['status'] = 8 ;
               state['rc'] = true ;
               isOperate = true ;
               break ;
            }
         }

         if( isOperate == false )
         {
            var index = result.indexOf( "\n" ) ;

            if( index >= 0 )
            {
               ++index ;
               state['lastLen'] -= index ;
               return result.slice( index ) ;
            }
            else
            {
               state['status'] = 7 ;
               state['rc'] = false ;
            }
         }
      }
   }

   if( state['status'] == 8 )
   {
      if( typeof( state['result'] ) == 'undefined' )
      {
         state['result'] = '' ;
      }
      else
      {
         state['result'] += ', ' ;
      }

      state['result'] += result ;
      if( result.indexOf( 'LINE ' ) == 0 )
      {
         state['status'] = 0 ;
         state['rc'] = false ;
      }
   }

   return null ;
}

function _parseSSQLHeader( header, state )
{
   var len, character, start, end, fieldMaxLen, readLen ;

   len = state['lastLen'] ;

   if( len < 5 )
   {
      return _parseSSQLResult( header, state ) ;
   }

   if( header.charAt(0) != '+' )
   {
      return _parseSSQLResult( header, state ) ;
   }

   state['header'] = [] ;
   start = 0 ;
   readLen = 1 ;

   for( var i = 1; i < len; ++i )
   {
      character = header.charAt(i) ;
      ++readLen ;

      if( character == '-' )
      {
         continue ;
      }
      else if( character == '+' )
      {
         end = i ;
         if( end - start < 4 )
         {
            state['status'] = 7 ;
            return state ;
         }
         state['header'].push( end - start - 1 ) ;
         start = i ;
      }
      else
      {
         state['lineLen'] = i ;
         state['lastLen'] -= i ;
         break ;
      }
   }

   if( state['lineLen'] <= 0 )
   {
      state['status'] = 7 ;
      return null ;
   }

   state['status'] = 1 ;

   return header.slice( readLen ) ;
}

function _parseSSQLField( fields, state )
{
   var start, readLen ;

   if( state['lastLen'] < state['lineLen'] )
   {
      state['status'] = 7 ;
      return null ;
   }

   if( fields.charAt(0) != '|' )
   {
      state['status'] = 7 ;
      return null ;
   }

   if( state['field'] && state['field'].length > 0 )
   {
      state['status'] = 2 ;
      return fields ; 
   }

   state['field'] = [] ;
   start = 0 ;
   readLen = 0 ;

   for( var i = 0; i < state['header'].length; ++i )
   {
      start += 1 ;
      state['field'].push( trim( fields.substr( start, state['header'][i] ) ) ) ;
      start += state['header'][i] ;

      readLen += 1 + state['header'][i] ;
   }

   readLen += 2 ;

   state['lastLen'] -= readLen ;
   state['status'] = 2 ;

   return fields.slice( readLen ) ;
}

function _parseSSQLEndHeader( header, state )
{
   var len, character, start, end, readLen ;

   if( state['lastLen'] < state['lineLen'] )
   {
      state['status'] = 7 ;
      return null ;
   }

   if( header.charAt(0) != '+' )
   {
      state['status'] = 7 ;
      return null ;
   }

   start = 0 ;
   readLen = 1 ;

   len = state['lastLen'] ;

   for( var i = 1; i < len; ++i )
   {
      character = header.charAt( i ) ;
      ++readLen ;

      if( character == '-' )
      {
         continue ;
      }
      else if( character == '+' )
      {
         end = i ;
         if( end - start < 4 )
         {
            state['status'] = 7 ;
            return null ;
         }
         start = i ;
      }
      else
      {
         state['lastLen'] -= i ;
         break ;
      }
   }

   state['status'] = 3 ;

   return header.slice( readLen ) ;
}
function _parseSSQLContent( content, state )
{
   var start, end, readLen, data ;

   readLen = 0 ;

   if( content.charAt(0) == '|' )
   {
      var isJoin = false ;
      state['status'] = 4 ;
      start = 0 ;
      data = {} ;

      for( var i = 0; i < state['header'].length; ++i )
      {
         start += 1 ;
         var tmp = content.substr( start, state['header'][i] ) ;
         if( tmp.charAt( state['header'][i] - 1 ) == '+' )
         {
            tmp = tmp.substr( 0, state['header'][i] - 2 )
            isJoin = true ;
         }

         data[ state['field'][i] ] = trim( tmp ) ;
         start += state['header'][i] ;

         readLen += 1 + state['header'][i] ;
      }

      if( state['isJoin'] === true )
      {
         var index = state['value'].length - 1 ;

         for( var i = 0; i < state['header'].length; ++i )
         {
            if( data[ state['field'][i] ].length > 0 )
            {
               state['value'][index][ state['field'][i] ] += "\n" + data[ state['field'][i] ] ;
            }
         }
      }
      else
      {
         state['value'].push( data ) ;
      }
      state['isJoin'] = isJoin ;
   }
   else if( content.charAt(0) == '+' )
   {
      var index = content.indexOf( "\n" ) ;
      if( index >= 0 )
      {
         ++index ;
         state['lastLen'] -= index ;
         state['status'] = 5 ;
         return content.slice( index ) ;
      }
      else
      {
         state['status'] = 7 ;
         state['rc'] = false ;
         return null ;
      }
      
   }
   else
   {
      state['status'] = 7 ;
      return null ;
   }

   readLen += 2 ;

   state['lastLen'] -= readLen ;

   return content.slice( readLen ) ;
}

function _parseSSQLAttr( attr, state )
{
   var len, character, readLen ;

   state['status'] = 6 ;
   readLen = 0 ;

   len = state['lastLen'] ;
   if ( len == 0 )
   {
      return null ;
   }

   for( var i = 0; i < len; ++i )
   {
      character = attr.charAt( i ) ;

      if( character === "\r" ||
          character === "\n" )
      {
         var newAttr = attr.substr( 0, i ) ;

         newAttr = trim( newAttr ) ;

         if( newAttr.length > 0 )
         {
            state['attr'].push( newAttr ) ;
         }
         readLen = i ;
         break ;
      }
   }

   readLen += 1 ;

   state['lastLen'] -= readLen ;

   return attr.slice( readLen ) ;
}

/*
   str    是字符串
   state  是状态机，结构如下
   {
      status: 0,         <---  状态
      length: 0,         <---  整个字符串长度
      lineLen: 0,        <---  一行的长度
      lastLen: 0,        <---  剩余长度
      isJoin: false,     <---  当前行是不是上一行的追加内容
      header: [          <---  表头
         6, 4, 5 , 10    <---  对应每列字段的最大长度
      ],
      field: [           <---  字段列表
         { name: "field_1", type: "int" }  <---  字段名 和 字段类型
         { name: "field_2", type: "text" }
      ],
      value: [           <---  获得的数据
         "aaaa",
         "bbbb"
      ],
      attr: [
         "Has OIDs: no",
         "Options: appendonly=true",
         "Distributed randomly"
      ],
      rc: true           <---  执行成功或失败
      result: 'ERROR:  syntax error at or near "aaa"'   <---  执行成功或失败返回的内容
   }
   status 当前状态 0: 未发现表头， 1：找到表头， 2：找到字段表， 3：表头结束， 4：内容， 5：内容结束， 6：其他数据， 7:解析错误, 8:解析返回结果
*/
function ParseSSQL( str )
{
   var result = {
      'rc': true,
      'field': null,
      'value': null,
      'attr': null
   } ;
   var state = { 'rc': true, 'status': 0 } ;

   if( typeof( str ) != 'string' )
   {
      result['rc'] = false ;
      return result ;
   }

   var len = str.length ;
   if( len <= 0 )
   {
      result['rc'] = false ;
      return result ;
   }

   state['length']  = len ;
   state['lastLen'] = len ;
   state['field']   = [] ;
   state['value']   = [] ;
   state['attr']    = [] ;

   while( true )
   {
      switch( state['status'] )
      {
      case 0:
         //0: 未发现表头
         str = _parseSSQLHeader( str, state ) ;
         break ;
      case 1:
         //1：找到表头
         str = _parseSSQLField( str, state ) ;
         break ;
      case 2:
         //2：找到字段表
         str = _parseSSQLEndHeader( str, state ) ;
         break ;
      case 3:
      case 4:
         //3：表头结束， 4：内容
         str = _parseSSQLContent( str, state ) ;
         break ;
      case 5:
      case 6:
         //5：内容结束， 6：其他数据
         str = _parseSSQLAttr( str, state ) ;
         break ;
      case 7:
         //7:解析错误
         break ;
      case 8:
         //8:解析返回结果
         str = _parseSSQLResult( str, state ) ;
         break ;
      }

      if( state['status'] == 7 )
      {
         state['rc'] = false ;
         break ;
      }

      if( str === null )
      {
         break ;
      }
   }

   result['rc']    = state['rc'] ;
   result['field'] = state['field'] ;
   result['value'] = state['value'] ;
   result['attr']  = state['attr'] ;

   return result ;
}

function ExecSsql( cmd, installPath, port, database, arg, timeout )
{
   var rc = SDB_OK ;
   var error = null ;
   var exec = installPath + '/bin/psql' ;
   //set LD_LIBRARY_PATH
   //export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/sequoiasqloltp/lib
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
   var installPath = '/opt/sequoiasqloltp' ;
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

