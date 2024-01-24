import( "lib/httpHeader.js" ) ;
import( "lib/parseJson.js" ) ;
import( "lib/md5.js" ) ;

function _hasKey( obj, key ) {
   return !( typeof( obj.key ) == 'undefined' ) ;
}

function _string_encode( str ){
   str = str.replace( new RegExp("\\\\",'g'), "\\\\" ) ;
   str = str.replace( new RegExp('"','g'), "\\\"" ) ;
   return '"' + str + '"' ;
}

function _sprintf( format )
{
   var len = arguments.length;
   var strLen = format.length ;
   var newStr = '' ;
   for( var i = 0, k = 1; i < strLen; ++i )
   {
      var chars = format.charAt( i ) ;
      if( chars == '\\' && ( i + 1 < strLen ) && format.charAt( i + 1 ) == '?' )
      {
         newStr += '?' ;
         ++i ;
      }
      else if( chars == '?' && k < len )
      {
         newStr += ( '' + arguments[k] ) ;
         ++k ;
      }
      else
      {
         newStr += chars ;
      }
   }
   return newStr ;
} ;

function var_dump( data ){
   println( JSON.stringify( data, null, 3 ) ) ;
}


/******************************* Request **************************************/
/*
   type: GET, POST, DELETE, PUT
*/
var SdbRequest = function( type, url ){
   this.url = url ;

   //请求方式POST, GET, PUT, DELETE等
   this.type = type ;

   //rest请求头
   this.headers = {
      'Connection': 'close',
      'Pragma': 'no-cache',
      'Accept': 'application/x-www-form-urlencoded'
   } ;

   //发送的数据
   this.data = '' ;

   //命令行
   this.cmd = '' ;
} ;

SdbRequest.prototype = {} ;
SdbRequest.prototype.constructor = SdbRequest;

SdbRequest.prototype.convert = function( url ){
   var args = '' ;

   //method
   if( this.type != 'GET' )
   {
      args += _sprintf( "-X ? ", this.type ) ;
   }

   //headers
   for( var key in this.headers )
   {
      args += _sprintf( "-H ? ", _string_encode( key + ":" + this.headers[key] ) ) ;
   }

   //post
   if( this.type == 'POST' && this.data.length > 0 )
   {
      args += _sprintf( "-d ? ", _string_encode( this.data ) ) ;
   }

   //url
   if( typeof( url ) == 'string' && url.length > 0 )
   {
      args += _sprintf( "-i ?", _string_encode( this.url + url ) ) ;
   }
   else
   {
      args += _sprintf( "-i ?", _string_encode( this.url ) ) ;
   }

   this.cmd = args ;

   return args ;
}

SdbRequest.prototype.setHeader = function( key, value ){
   if( typeof( value ) == 'string' )
   {
      if( value.length > 0 )
      {
         this.headers[key] = value ;
      }
      else
      {
         delete this.headers[key] ;
      }
   }
}

SdbRequest.prototype.setData = function( data ){
   if( typeof( data ) == 'string' )
   {
      this.data = data ;
   }
   else if( typeof( data ) == 'object' )
   {
      this.data = _object2postArg( data ) ;
   }
}

SdbRequest.prototype.getRequestCmd = function(){
   return 'curl ' + this.cmd ;
}

/******************************* Response **************************************/
var SdbResponse = function(){
   //rest应答结果
   this.code = 200 ;

   //rest应答头
   this.headers = {} ;

   //接收的数据
   this.data = '' ;

   //接收数据的类型
   this.dataType = 'string' ;
} ;

SdbResponse.prototype = {} ;
SdbResponse.prototype.constructor = SdbResponse;

SdbResponse.prototype.getCode = function(){
   return this.code ;
}


SdbResponse.prototype.getHeader = function( key ){
   return this.headers[key] ;
}

SdbResponse.prototype.getData = function(){
   return this.data ;
}

SdbResponse.prototype.getDataType = function(){
   return this.dataType ;
}

SdbResponse.prototype.parse = function( str ) {
   str = str.split( "\r\n\r\n" ) ;
   var header = str[0] ;
   var body = '' ;
   var obj = httpHeader( header ) ;

   for( var i = 1; i < str.length; ++i )
   {
      body += str[i] ;
   }
   str = null ;

   this.code = obj['statusCode'] ;
   this.headers = obj['headers'] ;

   if ( body.charAt( 0 ) == '{' )
   {
      this.data = _parseJson2( body, true ) ;
      this.dataType = 'json' ;
      if( this.data.length == 0 )
      {
         this.data = body ;
         this.dataType = 'string' ;
      }
   }
   else if ( body.charAt( 0 ) == '[' )
   {
      this.data = JSON.parse( body ) ;
      this.dataType = typeof( this.data ) ;
      if( this.dataType == 'object' )
      {
         this.dataType = 'json' ;
      }
   }
   else
   {
      this.data = body ;
      this.dataType = 'string' ;
   }
}

SdbResponse.prototype.toString = function(){
   var res = "Response: \n" ;

   res += _sprintf( "Code: ?\n\n", this.code ) ;

   res += "Header: \n" ;
   for( var key in this.headers )
   {
      res += _sprintf( "? : ?\n", key, this.headers[key] ) ;
   }

   res += "\n" ;

   res += "Body: \n" ;
   if( typeof( this.data ) == 'string' )
   {
      res += _sprintf( "?\n", this.data ) ;
   }
   else if ( typeof( this.data ) == 'object'  )
   {
      if ( _isArray( this.data ) )
      {
         for( var key in this.data )
         {
            res += _sprintf( "?\n", JSON.stringify( this.data[key], null, 3 ) ) ;
         }
      }
      else
      {
         for( var key in this.data )
         {
            res += _sprintf(  "? : ?\n", key, this.data[key] ) ;
         }
      }
   }
   else
   {
      res += _sprintf( "?\n", this.data ) ;
   }

   res += "\n" ;

   return res ;
}

/******************************** Rest ****************************************/
var SdbRest = function(){

   //执行超时时间(毫秒)
   this.timeout = 5 * 60 * 1000 ; //5分钟

} ;

SdbRest.prototype = {} ;
SdbRest.prototype.constructor = SdbRest;

SdbRest.prototype.send = function( request, url ){
   var cmd = new Cmd() ;

   try
   {
      if( this.timeout > 0 )
      {
         cmd.run( 'curl', request.convert( url ) + ' 2>/dev/null', this.timeout ) ;
      }
      else
      {
         cmd.run( 'curl', request.convert( url ) + ' 2>/dev/null' ) ;
      }
   }
   catch( e )
   {
      try
      {
         if( this.timeout > 0 )
         {
            cmd.run( 'curl', request.convert( url ), this.timeout ) ;
         }
         else
         {
            cmd.run( 'curl', request.convert( url ) ) ;
         }
      }
      catch( e )
      {
         throw new Error( "rest error: " + cmd.getLastOut() + ', error:' + e ) ;
      }
   }

   if ( cmd.getLastRet() != 0 )
   {
      throw new Error( "rest error: " + cmd.getLastRet() ) ;
   }

   return cmd.getLastOut() ;
} ;

/*
   settings:
      timeout: 0不超时, > 0超时
   url: 使用自定义url
*/
SdbRest.prototype.ajax = function( request, settings, url ) {

   if ( typeof( settings ) == 'object' && settings !== null )
   {
      if( _hasKey( settings, 'timeout' ) )
      {
         this.timeout = settings['timeout'] ;
      }
   }

   var response = new SdbResponse() ;

   response.parse( this.send( request, url ) ) ;

   return response ;
} ;







