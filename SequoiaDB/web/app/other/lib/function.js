var getBrowserInfo = function()
{
	var agent = window.navigator.userAgent.toLowerCase() ;
	var regStr_ie = /msie [\d.]+;/gi ;
	var regStr_ff = /firefox\/[\d.]+/gi ;
	var regStr_chrome = /chrome\/[\d.]+/gi ;
	var regStr_saf = /safari\/[\d.]+/gi ;
	var temp = '' ;
	var info = [] ;
	if( agent.indexOf( 'msie' ) > 0 )
	{
		temp = agent.match( regStr_ie ) ;
		info.push( 'ie' ) ;
	}
	else if( agent.indexOf( 'firefox' ) > 0 )
	{
		temp = agent.match( regStr_ff ) ;
		info.push( 'firefox' ) ;
	}
	else if( agent.indexOf( 'chrome' ) > 0 )
	{
		temp = agent.match( regStr_chrome ) ;
		info.push( 'chrome' ) ;
	}
	else if( agent.indexOf( 'safari' ) > 0 && agent.indexOf( 'chrome' ) < 0 )
	{
		temp = agent.match( regStr_saf ) ;
		info.push( 'safari' ) ;
	}
	else
	{
		if( agent.indexOf( 'trident' ) > 0 && agent.indexOf( 'rv' ) > 0 )
		{
			info.push( 'ie' ) ;
			temp = '11' ;
		}
		else
		{
			temp = '0' ;
			info.push( 'unknow' ) ;
		}
	}
	verinfo = ( temp + '' ).replace( /[^0-9.]/ig, '' ) ;
	info.push( parseInt( verinfo ) ) ;
	return info ;
}

var setBrowserStorage = function()
{
	var browser = getBrowserInfo() ;
   var storageType ;
	if( browser[0] === 'ie' && browser[1] <= 7 )
	{
      storageType = 'cookie' ;
	}
	else
	{
		if( window.localStorage )
		{
			storageType = 'localStorage' ;
		}
		else
		{
			if( navigator.cookieEnabled === true )
			{
				storageType = 'cookie' ;
			}
			else
			{
				storageType = '' ;
			}
		}
	}
	return storageType ;
}

var localLocalData = function( key, value )
{
   var storageType = setBrowserStorage() ;

   if( typeof( value ) == 'undefined' )
   {
      //读取本地数据
      var newValue = null ;
	   if ( storageType === 'localStorage' )
	   {
		   newValue = window.localStorage.getItem( key ) ;
	   }
	   else if ( storageType === 'cookie' )
	   {
		   newValue = $.cookie( key ) ;
	   }
	   return newValue ;
   }
   else if( value == null )
   {
      //删除本地数据
      if ( storageType === 'localStorage' )
	   {
		   window.localStorage.removeItem( key ) ;
	   }
	   else if ( storageType === 'cookie' )
	   {
		   $.removeCookie( key ) ;
	   }
   }
   else
   {
      //写入本地数据
      if ( storageType === 'localStorage' )
	   {
		   window.localStorage.setItem( key, value ) ;
	   }
	   else if ( storageType === 'cookie' )
	   {
		   var saveTime = new Date() ;
		   saveTime.setDate( saveTime.getDate() + 365 ) ;
		   $.cookie( key, value, { 'expires': saveTime } ) ;
	   }
   }
}

var isPluginPath = function( fileName )
{
   var files = fileName.split( '/' ) ;

   if( files[0] == '.' || files[0] == '' )
   {
      files.splice( 0, 1 ) ;
   }

   if( files.length < 3 )
   {
      return false ;
   }

   if( files[0] != 'app' )
   {
      return false ;
   }

   if( files[1] != 'controller' )
   {
      return false ;
   }

   if( files[2] != 'Data' &&
       files[2] != 'Monitor' )
   {
      return false ;
   }

   return true ;
}

//动态加载模块文件
var resolveFun = function( files ){
   return {
      deps: function( $q, $rootScope ){
         var deferred = $q.defer();
         var dependencies = files ;
         var i = 0 ;

         function loadjs( fileName )
         {
            $.ajax( {
               'url': fileName,
               'dataType': 'script',
               'beforeSend': function( jqXHR ){
                  if( isPluginPath( fileName ) == false )
                  {
                     return ;
                  }

	               var clusterName = localLocalData( 'SdbClusterName' ) ;
	               if( clusterName !== null )
	               {
		               jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	               }
	               var businessName = localLocalData( 'SdbModuleName' )
	               if( businessName !== null )
	               {
		               jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	               }
               },
               'success': function( response, status ){
                  ++i ;
                  if( i == dependencies.length )
                  {
                     $rootScope.$apply( function(){
                        deferred.resolve() ;
                     } ) ;
                  }
                  else
                  {
                     loadjs( dependencies[i] ) ;
                  }
               },
               'error': function( XMLHttpRequest, textStatus, errorThrown ){
                  if( window.SdbDebug == true )
                  {
                     throw errorThrown ;
                  }
               }
            } ) ;
         }
         if( dependencies.length > 0 )
         {
            loadjs( dependencies[0] ) ;
         }
         return deferred.promise ;
      }
   }
} ;

//格式化
var sprintf = function( format )
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

//判断是否空
function isEmpty( val )
{
   switch( typeof( val ) )
   {
   case 'undefined':
      return true ;
   case 'number':
      return val == 0 ;
   case 'string':
      return val.length == 0 ;
   case 'object':
      for( var k in val )
      {
         return false ;
      }
      return true ;
   default:
      alert( val ) ;
      return false ;
   }
   return false ;
}

//保留多少位小数
function fixedNumber( x, num )
{
   if( isNaN( x ) )
   {
      return x ;
   }
   var y = parseFloat( x );
   var z = Math.pow( 10, num );
   y = Math.round( y * z ) / z ;
   return y ;
}

//字符串补位
function pad( num, n, chars )
{
   chars = ( typeof( chars ) == 'undefined' ? '0' : chars ) ;
   var len = num.toString().length;
   while( len < n )
   {
      num = chars + num ;
      ++len ;
   }
   return num ;
}

//获取对象的属性数量
function getObjectSize( obj )
{
   var len = 0 ;
   if( typeof( obj ) == 'object' )
   {
      $.each( obj, function(){
         ++len ;
      } ) ;
   }
   return len ;
}

//获取对象的属性是第几个
function getObjectAttrIndex( obj, attr )
{
   var index = 0 ;
   for( var key in obj )
   {
      if( key == attr )
      {
         return index ;
      }
      ++index ;
   }
   return -1 ;
}

//获取对象的第x个属性
function getObjetAttrByIndex( obj, index )
{
   var index2 = 0 ;
   for( var key in obj )
   {
      if( index == index2 )
      {
         return key ;
      }
      ++index2 ;
   }
   return undefined ;
}

//格式化日期
function timeFormat( date, fmt )
{
   var o = {
      "M+": date.getMonth() + 1,
      "d+": date.getDate(),
      "h+": date.getHours(),
      "m+": date.getMinutes(),
      "s+": date.getSeconds(),
      "q+": Math.floor( ( date.getMonth() + 3 ) / 3 ),
      "S" : date.getMilliseconds()
   } ;
   if( /(y+)/.test( fmt ) )
   {
      fmt = fmt.replace( RegExp.$1, ( date.getFullYear() + "" ).substr( 4 - RegExp.$1.length ) ) ;
   }
   for ( var k in o )
   {
      if( new RegExp( "(" + k + ")" ).test( fmt ) )
      {
         fmt = fmt.replace( RegExp.$1, ( RegExp.$1.length == 1) ? ( o[k] ) : ( ( "00" + o[k] ).substr( ( "" + o[k] ).length ) ) ) ;
      }
   }
   return fmt ;
}

//删除两端空格
function trim( str )
{
   if( typeof( str ) == 'string' )
   {
      return str.replace( /(^\s*)|(\s*$)/g, '' ) ;
   }
   return str ;
}

//判断是不是数组
function isArray( object ) {
   if( typeof( object ) == 'undefined' )
      return false ;
   if( object === null )
      return false ;
   //判断length属性是否是可枚举的 对于数组 将得到false
   return object && typeof( object ) === 'object' && typeof( object.length ) === 'number' &&
            typeof( object.splice ) === 'function' && !( object.propertyIsEnumerable( 'length' ) ) ;
}

//自动判断类型并转换
//hasQuotes 如果设置成true，那么如果带有 "xxx"，则转换成 xxxx 的字符串
function autoTypeConvert( val, hasQuotes )
{
   if( typeof( val ) == 'string' )
   {
      var valLen = val.length ;
      if( valLen > 0 )
      {
         if( hasQuotes == true )
         {
            if( valLen > 1 && val.charAt(0) == '"' && val.charAt(valLen - 1) == '"' )
            {
               return val.substr( 1, valLen - 2 ) ;
            }
         }
         if( val.toLowerCase() == 'null' )
         {
            val = null ;
         }
         else if( val.toLowerCase() == 'true' )
         {
            val = true ;
         }
         else if( val.toLowerCase() == 'false' )
         {
            val = false ;
         }
         else if( val.toLowerCase() == '$minkey' )
         {
            val = { '$minKey': 1 } ;
         }
         else if( val.toLowerCase() == '$maxkey' )
         {
            val = { '$maxKey': 1 } ;
         }
         else if( val.toLowerCase() == '$undefined' )
         {
            val = { '$undefined': 1 } ;
         }
         else if( !isNaN( val ) )
         {
            val = Number( val ) ;
         }
      }
   }
   return val ;
}

//解析decimal类型的字符串
function parseDecimal( str )
{
   var decimal = trim( str ) ;
   var precision = null ;
   var left = decimal.indexOf( '(' ) ;
   var right = decimal.indexOf( ')' )
   if( left > 0 && right > 0 && left < right && decimal.length - 1 == right )
   {
      precision = decimal.substring( left + 1, right ) ;
      precision = precision.split( ',' ) ;
      if( precision.length == 2 && isNaN( precision[0] ) == false && isNaN( precision[1] ) == false )
      {
         precision = [ parseInt( precision[0] ), parseInt( precision[1] ) ] ;
         decimal = decimal.substring( 0, left ) ;
      }
      else
      {
         precision = null ;
      }
   }
   if( precision == null )
   {
      return { '$decimal': decimal } ;
   }
   else
   {
      return { '$decimal': decimal, '$precision': precision } ;
   }
}

//解析regex类型的字符串
function parseRegex( str )
{
   var regex = trim( str ) ;
   var options = '' ;
   if( regex.charAt(0) == '/' && regex.indexOf( '/', 1 ) > 0 )
   {
      var right = regex.indexOf( '/', 1 ) ;
      options = regex.substr( right + 1 ) ;
      regex = regex.substr( 1, right - 1 ) ;
   }
   return { '$regex': regex, '$options': options } ;
}

//解析binary类型的字符串
function parseBinary( str )
{
   var binary = trim( str ) ;
   var binType = '' ;
   if( binary.length > 0 && binary.charAt(0) == '(' && binary.indexOf( ')' ) >= 0 )
   {
      var right = binary.indexOf( ')' ) ;
      binType = binary.substr( 1, right - 1 ) ;
      binary = binary.substr( right + 1 ) ;
   }
   return { '$binary': binary, '$type': binType } ;
}

//指定类型转换
function specifyTypeConvert( val, type )
{
   var retval = null ;
   switch( type )
   {
   case 'Auto':
      retval = autoTypeConvert( val, true ) ;
      break ;
   case 'Bool':
      if( val.toLowerCase() == 'true' )
      {
         retval = true ;
      }
      else if( val.toLowerCase() == 'false' )
      {
         retval = false ;
      }
      else
      {
         retval = val ? true : false ;
      }
      break ;
   case 'Number':
      if( isNaN( val ) == false )
      {
         retval = Number( val ) ;
      }
      else
      {
         retval = parseInt( val ) ;
      }
      break ;
   case 'Decimal':
      retval = parseDecimal( val ) ;
      break ;
   case 'String':
      retval = val ;
      break ;
   case 'ObjectId':
      val = pad( val, 24, '0' ) ;
      retval = { '$oid': val } ;
      break ;
   case 'Regex':
      retval = parseRegex( val ) ;
      break ;
   case 'Binary':
      retval = parseBinary( val ) ;
      break ;
   case 'Timestamp':
      retval = { '$timestamp': val } ;
      break ;
   case 'Date':
      retval = { '$date': val } ;
      break ;
   default:
      retval = val ;
      break ;
   }
   return retval ;
}

/*
   数组结构 -> json
   例:
   [
      { key: 'Object', type: 'Object', level: 0, isOpen: false, val: [
         { key: 'a', type: 'Auto', level: 1, isOpen: false, val: '123' },
         { key: 'b', type: 'Object', level: 1, isOpen: false, val: [
            { key: 'c', type: 'String', level: 2, isOpen: false, val: 'hello' },
            { key: 'd', type: 'Auto', level: 2, isOpen: false, val: 'true' }
         ] },
         { key: 'e', type: 'Array', level: 1, isOpen: false, val: [
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '7' },
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '8' },
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '9' }
         ] },
      ] }
   ]
   转成
   {
      a : 123,
      b : {
        c: "hello",
        d: true
      }
      e: [ 7, 8, 9 ]
   }
*/
function array2Json( array, parentType )
{
   if( typeof( parentType ) == 'undefined' || ( parentType != 'Object' && parentType != 'Array' ) ) parentType = 'Object' ;
   var json ;
   if( parentType == 'Object' )
   {
      json = {} ;
   }
   else
   {
      json = [] ;
   }
   $.each( array, function( index, field ){
      if( field['type'] == 'Object' && field['level'] == 0 )
      {
         json = array2Json( field['val'], field['type'] ) ;
         return false ;
      }
      else if( field['type'] == 'Object' || field['type'] == 'Array' )
      {
         var val = field['val'] ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = array2Json( val, field['type'] ) ;
         }
         else
         {
            json.push( array2Json( val, field['type'] ) ) ;
         }
      }
      else if( field['type'] == 'Auto' )
      {
         var val = trim( field['val'] ) ;
         val = autoTypeConvert( val ) ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = val ;
         }
         else
         {
            json.push( val ) ;
         }
      }
      else if( field['type'] == 'String' )
      {
         var val = field['val'] ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = val ;
         }
         else
         {
            json.push( val ) ;
         }
      }
      else if( field['type'] == 'Binary' )
      {
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = parseBinary( field['val'] ) ;
         }
         else
         {
            json.push( parseBinary( field['val'] ) ) ;
         }
      }
      else if( field['type'] == 'Timestamp' )
      {
         var val = trim( field['val'] ) ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = { '$timestamp': val } ;
         }
         else
         {
            json.push( { '$timestamp': val } ) ;
         }
      }
      else if( field['type'] == 'Date' )
      {
         var val = trim( field['val'] ) ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = { '$date': val } ;
         }
         else
         {
            json.push( { '$date': val } ) ;
         }
      }
      else if( field['type'] == 'ObjectId' )
      {
         var val = trim( field['val'] ) ;
         val = pad( val, 24, '0' ) ;
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = { '$oid': val } ;
         }
         else
         {
            json.push( { '$oid': val } ) ;
         }
      }
      else if( field['type'] == 'Regex' )
      {
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = parseRegex( field['val'] ) ;
         }
         else
         {
            json.push( parseRegex( field['val'] ) ) ;
         }
      }
      else if( field['type'] == 'Decimal' )
      {
         if( parentType == 'Object' )
         {
            json[ field['key'] ] = parseDecimal( field['val'] ) ;
         }
         else
         {
            json.push( parseDecimal( field['val'] ) ) ;
         }
      }
   } ) ;
   return json ;
}

/*
   json -> 数组结构
   例:
   {
      a : 123,
      b : {
        c: "hello",
        d: true
      }
      e: [ 7, 8, 9 ]
   }
   转成
   [
      { key: 'Object', type: 'Object', level: 0, isOpen: false, val: [
         { key: 'a', type: 'Auto', level: 1, isOpen: false, val: '123' },
         { key: 'b', type: 'Object', level: 1, isOpen: false, val: [
            { key: 'c', type: 'String', level: 2, isOpen: false, val: 'hello' },
            { key: 'd', type: 'Auto', level: 2, isOpen: false, val: 'true' }
         ] },
         { key: 'e', type: 'Array', level: 1, isOpen: false, val: [
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '7' },
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '8' },
            { key: '', type: 'Auto', level: 2, isOpen: false, val: '9' }
         ] },
      ] }
   ]
*/
function json2Array( json, level, exact )
{
   if( isNaN( level ) ) level = 0 ;
   if( typeof( exact ) == 'undefined' ) exact = false ;
   var array = [] ;
   if( level == 0 )
   {
      var child = json2Array( json, level + 1, exact ) ;
      array.push( { key: 'Object', type: 'Object', level: 0, isOpen: false, val: child } ) ;
      return array ;
   }
   $.each( json, function( key, value ){
      key = key + '' ;
      var valueType = typeof( value ) ;
      if( valueType == 'object' )
      {
         if( value == null )
         {
            value = 'null' ;
            valueType = 'Auto' ;
            if( exact == true )
            {
               valueType = 'Null' ;
            }
         }
         else if( isArray( value ) )
         {
            value = json2Array( value, level + 1, exact ) ;
            valueType = 'Array' ;
         }
         else if( typeof( value['$binary'] ) == 'string' && typeof( value['$type'] ) == 'string' )
         {
            var binary = value['$binary'] ;
            var binType = value['$type'] ;
            value = binary ;
            if( binType.length > 0 )
            {
                value = '(' + binType + ')' + value ;
            }
            valueType = 'Binary' ;
         }
         else if( typeof( value['$timestamp'] ) == 'string' )
         {
            value = value['$timestamp'] ;
            valueType = 'Timestamp' ;
         }
         else if( typeof( value['$date'] ) == 'string' )
         {
            value = value['$date'] ;
            valueType = 'Date' ;
         }
         /*
         else if( typeof( value['$code'] ) == 'string' )
         {
            value = value['$code'] ;
            valueType = 'Code' ;
         }
         */
         else if( typeof( value['$minKey'] ) == 'number' )
         {
            value = 'minKey' ;
            valueType = 'Auto' ;
            if( exact == true )
            {
               valueType = 'MinKey' ;
            }
         }
         else if( typeof( value['$maxKey'] ) == 'number')
         {
            value = 'maxKey' ;
            valueType = 'Auto' ;
            if( exact == true )
            {
               valueType = 'MaxKey' ;
            }
         }
         else if( typeof( value['$undefined'] ) == 'number' )
         {
            value = 'undefined' ;
            valueType = 'Auto' ;
            if( exact == true )
            {
               valueType = 'Undefined' ;
            }
         }
         else if( typeof( value['$oid'] ) == 'string' )
         {
            value = value['$oid'] ;
            value = pad( value, 24, '0' ) ;
            valueType = 'ObjectId' ;
         }
         else if( typeof( value['$regex'] ) == 'string' && typeof( value['$options'] ) == 'string' )
         {
            value = '/' + value['$regex'] + '/' + value['$options'] ;
            valueType = 'Regex' ;
         }
         else if( typeof( value['$decimal'] ) == 'string' )
         {
            var precision = value['$precision'] ;
            value = value['$decimal'] ;
            if( isArray( precision ) )
            {
               value = value + '(' + precision[0] + ',' + precision[1] + ')' ;
            }
            valueType = 'Decimal' ;
         }
         else
         {
            value = json2Array( value, level + 1, exact ) ;
            valueType = 'Object' ;
         }
      }
      else if( valueType == 'boolean' )
      {
         value = ( value ? 'true' : 'false' ) ;
         valueType = 'Auto' ;
         if( exact == true )
         {
            valueType = 'Bool' ;
         }
      }
      else if (valueType == 'number')
      {
         value = value + '' ;
         valueType = 'Auto' ;
         if( exact == true )
         {
            valueType = 'Number' ;
         }
      }
      else if (valueType == 'string')
      {
         value = value ;
         valueType = 'String' ;
      }
      else
      {
         
      }
      array.push( { key: key, type: valueType, level: level, isOpen: false, val: value } ) ;
   } ) ;
   return array ;
}

//打印调试
function printfDebug( text )
{
   try
   {
      if( window.SdbDebug == true )
         console.warn( text ) ;
   }
   catch( e ){}
}

//带小数就进位
function numberCarry( num )
{
   var intNum = parseInt( num ) ;
   if( intNum != num )
   {
      num = intNum + 1 ;
   }
   return num ;
}

//获取操作系统信息
function getSystemInfo()
{
   var nAgt = navigator.userAgent;
   var os = 'unknown' ;
   var clientStrings = [
       { s: 'Windows 10', r: /(Windows 10.0|Windows NT 10.0)/ },
       { s: 'Windows 8.1', r: /(Windows 8.1|Windows NT 6.3)/ },
       { s: 'Windows 8', r: /(Windows 8|Windows NT 6.2)/ },
       { s: 'Windows 7', r: /(Windows 7|Windows NT 6.1)/ },
       { s: 'Windows Vista', r: /Windows NT 6.0/ },
       { s: 'Windows Server 2003', r: /Windows NT 5.2/ },
       { s: 'Windows XP', r: /(Windows NT 5.1|Windows XP)/ },
       { s: 'Windows 2000', r: /(Windows NT 5.0|Windows 2000)/ },
       { s: 'Windows ME', r: /(Win 9x 4.90|Windows ME)/ },
       { s: 'Windows 98', r: /(Windows 98|Win98)/ },
       { s: 'Windows 95', r: /(Windows 95|Win95|Windows_95)/ },
       { s: 'Windows NT 4.0', r: /(Windows NT 4.0|WinNT4.0|WinNT|Windows NT)/ },
       { s: 'Windows CE', r: /Windows CE/ },
       { s: 'Windows 3.11', r: /Win16/ },
       { s: 'Android', r: /Android/ },
       { s: 'Open BSD', r: /OpenBSD/ },
       { s: 'Sun OS', r: /SunOS/ },
       { s: 'Linux', r: /(Linux|X11)/ },
       { s: 'iOS', r: /(iPhone|iPad|iPod)/ },
       { s: 'Mac OS X', r: /Mac OS X/ },
       { s: 'Mac OS', r: /(MacPPC|MacIntel|Mac_PowerPC|Macintosh)/ },
       { s: 'QNX', r: /QNX/ },
       { s: 'UNIX', r: /UNIX/ },
       { s: 'BeOS', r: /BeOS/ },
       { s: 'OS/2', r: /OS\/2/ },
       { s: 'Search Bot', r: /(nuhk|Googlebot|Yammybot|Openbot|Slurp|MSNBot|Ask Jeeves\/Teoma|ia_archiver)/ }
   ];
   for (var id in clientStrings)
   {
      var cs = clientStrings[id];
      if (cs.r.test(nAgt))
      {
         os = cs.s;
         break;
      }
   }
   var osVersion = 'unknown';
   if (/Windows/.test(os)) {
      osVersion = /Windows (.*)/.exec(os)[1];
      os = 'Windows';
   }
   switch( os )
   {
   case 'Mac OS X':
      osVersion = /Mac OS X (10[\.\_\d]+)/.exec(nAgt)[1];
      break;
   case 'Android':
      osVersion = /Android ([\.\_\d]+)/.exec(nAgt)[1];
      break;
   case 'iOS':
      osVersion = /OS (\d+)_(\d+)_?(\d+)?/.exec(nVer);
      osVersion = osVersion[1] + '.' + osVersion[2] + '.' + (osVersion[3] | 0);
      break;
   }
   return [os, osVersion] ;
}

//IE7不支持对象用indexOf，为了兼容所以加上代码
if(!Array.indexOf)
{
    Array.prototype.indexOf = function(obj)
    {              
        for(var i=0; i<this.length; i++)
        {
            if(this[i]==obj)
            {
                return i;
            }
        }
        return -1;
    }
}

//解析condition的值
function parseConditionValue( condition )
{
   var filter = {} ;
   var jobj = condition ;
   if( jobj['condition'].length > 1 || ( jobj['condition'].length == 1 && jobj['condition'][0]['field'].length > 0 ) )
   {
      filter = [] ;
      $.each( jobj['condition'], function( index, field ){
         var subCondition = {} ;
         var fieldValue = autoTypeConvert( trim( field['value'] ), true ) ;
         switch( field['logic'] )
         {
         case '>':
            subCondition[ field['field'] ] = { '$gt': fieldValue } ;
            break ;
         case '>=':
            subCondition[ field['field'] ] = { '$gte': fieldValue } ;
            break ;
         case '<':
            subCondition[ field['field'] ] = { '$lt': fieldValue } ;
            break ;
         case '<=':
            subCondition[ field['field'] ] = { '$lte': fieldValue } ;
            break ;
         case '!=':
            subCondition[ field['field'] ] = { '$ne': fieldValue } ;
            break ;
         case '=':
            subCondition[ field['field'] ] = fieldValue ;
            break ;
         case 'size':
            subCondition[ field['field'] ] = { '$size': 1, '$et': fieldValue } ;
            break ;
         case 'regex':
            var regex = trim( fieldValue ) ;
            var options = '' ;
            if( regex.charAt(0) == '/' && regex.indexOf( '/', 1 ) > 0 )
            {
               var right = regex.indexOf( '/', 1 ) ;
               options = regex.substr( right + 1 ) ;
               regex = regex.substr( 1, right - 1 ) ;
            }
            subCondition[ field['field'] ] = { '$regex': regex, '$options': options } ;
            break ;
         case 'type':
            if( isNaN( fieldValue ) )
            {
               fieldValue = fieldValue.toLowerCase() ;
            }
            switch( fieldValue )
            {
            case 'int':
            case 'int32':
            case 'integer':
               fieldValue = 16 ;
               break ;
            case 'long':
            case 'long long':
            case 'int64':
               fieldValue = 18 ;
               break ;
            case 'double':
            case 'float':
               fieldValue = 1 ;
               break ;
            case 'string':
            case 'char':
            case '""':
               fieldValue = 2 ;
               break ;
            case 'objectid':
            case '_id':
            case 'id':
               fieldValue = 7 ;
               break ;
            case 'bool':
            case 'boolean':
            case 'true':
            case 'false':
               fieldValue = 8 ;
               break ;
            case 'date':
               fieldValue = 9 ;
               break ;
            case 'time':
            case 'timestamp':
               fieldValue = 17 ;
               break ;
            case 'bin':
            case 'binary':
            case 'data':
            case 'binary data':
               fieldValue = 5 ;
               break ;
            case 'regex':
            case 'regular':
            case 'regular expression':
            case 'regexp':
            case 're':
               fieldValue = 11 ;
               break ;
            case 'object':
            case 'obj':
               fieldValue = 3 ;
               break ;
            case 'array':
            case 'arr':
               fieldValue = 4 ;
               break ;
            case 'null':
            case 'nil':
               fieldValue = 10 ;
               break ;
            default:
               fieldValue = parseInt( fieldValue ) ;
               break ;
            }
            subCondition[ field['field'] ] = { '$type': 1, '$et': fieldValue } ;
            break ;
         case 'null':
            subCondition[ field['field'] ] = { '$isnull': 1 } ;
            break ;
         case 'notnull':
            subCondition[ field['field'] ] = { '$isnull': 0 } ;
            break ;
         case 'exists':
            subCondition[ field['field'] ] = { '$exists': 1 } ;
            break ;
         case 'notexists':
            subCondition[ field['field'] ] = { '$exists': 0 } ;
            break ;
         }
         filter.push( subCondition ) ;
      } ) ;
      if( jobj['model'] == 'or' )
      {
         filter = { '$or': filter } ;
      }
      else
      {
         filter = { '$and': filter } ;
      }
   }
   return filter ;
}

//解析selector的值
function parseSelectorValue( jobj ){
   var selector = {} ;
   if( jobj.length > 1 || ( jobj.length == 1 && jobj[0]['field'].length > 0 ) )
   {
      $.each( jobj, function( index, field ){
         selector[ field['field'] ] = 1 ;
      } ) ;
   }
   return selector ;
}

//解析sort的值
function parseSortValue( jobj ){
   var sort = {} ;
   if( jobj.length > 1 || ( jobj.length == 1 && jobj[0]['field'].length > 0 ) )
   {
      $.each( jobj, function( index, field ){
         sort[ field['field'] ] = autoTypeConvert( field['order'], false )  ;
      } ) ;
   }
   return sort ;
}

//解析hint的值
function parseHintValue( jobj )
{
   var hint = {} ;
   if( jobj === 1 )
   {
      hint[''] = null ;
   }
   else if( jobj === 0 )
   {
   }
   else
   {
      hint[''] = jobj ;
   }
   return hint ;
}

//解析updator的值
function parseUpdatorValue( jobj ){
   var updator = {} ;
   $.each( jobj, function( index, field ){
      var fieldValue = autoTypeConvert( field['value'], true ) ;
      updator[ field['field'] ] = fieldValue ;
   } ) ;
   updator = { '$set': updator } ;
   return updator ;
}

//指定光标移动到最后
function set_focus( box )
{
   box.focus() ;
   if( $.support.msie )
   {
      var range = document.selection.createRange();
      this.last = range;
      range.moveToElementText( box );
      range.select();
      document.selection.empty(); //取消选中
   }
   else
   {
      var range = document.createRange();
      range.selectNodeContents( box );
      range.collapse(false);
      var sel = window.getSelection();
      sel.removeAllRanges();
      sel.addRange(range);
   }
}

//解析indexDef的值
function parseIndexDefValue( jobj ){
   var indexDef = {} ;
   if( jobj.length > 1 || ( jobj.length == 1 && jobj[0]['field'].length > 0 ) )
   {
      $.each( jobj, function( index, field ){
         indexDef[ field['field'] ] = autoTypeConvert( field['order'], false )  ;
      } ) ;
   }
   return indexDef ;
}

/*
   str    是字符串
   state  是状态机，结构如下
   {
      status: 0,         <---  状态
      length: 0,         <---  一行的长度
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
      isJoin: false,     <---  当前行是不是上一行的追加内容
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
function parseSSQL( str, state )
{
   if( !state )
   {
      state = { 'status': 0 } ;
   }
   if( typeof( str ) != 'string' )
   {
      state['status'] = 7 ;
      return state ;
   }
   var sqlStrlen = function(str){
      var len = 0;
      for (var i=0; i<str.length; i++) { 
         var c = str.charCodeAt(i); 
         if ((c >= 0x0001 && c <= 0x007e) || (0xff60<=c && c<=0xff9f)) { 
            len++; 
         } 
         else { 
            len+=2; 
         } 
      } 
      return len;
   }
   var parseSSQLResult = function( result, state ){
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
         else if( result.indexOf( 'CREATE DATABASE' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
         }
         else if( result.indexOf( 'DROP DATABASE' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
         }
         else if( result.indexOf( 'CREATE TABLE' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
         }
         else if( result.indexOf( 'DROP TABLE' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
         }
         else if( result.indexOf( 'ALTER TABLE' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
         }
         else if( result.indexOf( 'INSERT ' ) == 0 )
         {
            state['status'] = 8 ;
            state['rc'] = true ;
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
      return state ;
   }
   var parseSSQLHeader = function( header, state ){
      var len, char, start, end, fieldMaxLen ;
      len = header.length ;
      if( len < 5 )
      {
         return parseSSQLResult( header, state ) ;
      }
      if( header.charAt(0) != '+' )
      {
         return parseSSQLResult( header, state ) ;
      }
      state['header'] = [] ;
      state['length'] = len ;
      start = 0 ;
      for( var i = 1; i < len; ++i )
      {
         char = header.charAt(i) ;
         if( char == '-' )
         {
            continue ;
         }
         else if( char == '+' )
         {
            end = i ;
            if( end - start < 4 )
            {
               state['status'] = 7 ;
               return state ;
            }
            state['header'].push( end - start - 3 ) ;
            start = i ;
         }
         else
         {
            break ;
         }
      }
      state['status'] = 1 ;
      return state ;
   }
   var parseSSQLField = function( fields, state ){
      var len, start, end ;
      len = fields.length ;
      if( len != state['length'] )
      {
         state['status'] = 7 ;
         return state ;
      }
      if( str.charAt(0) != '|' )
      {
         state['status'] = 7 ;
         return state ;
      }
      if( state['field'] && state['field'].length > 0 )
      {
         state['status'] = 2 ;
         return state ; 
      }
      state['field'] = [] ;
      start = 0 ;
      for( var i = 0; i < state['header'].length; ++i )
      {
         start = start + 2 ;
         state['field'].push( { 'name': trim( fields.substr( start, state['header'][i] ) ) } ) ;
         start = start + state['header'][i] + 1 ;
      }
      state['status'] = 2 ;
      return state ; 
   }
   var parseSSQLEndHeader = function( header, state ){
      var len, char, start, end ;
      len = header.length ;
      if( len != state['length'] )
      {
         state['status'] = 7 ;
         return state ;
      }
      if( header.charAt(0) != '+' ) return state ;
      state['length'] = len ;
      start = 0 ;
      for( var i = 1; i < len; ++i )
      {
         char = header.charAt(i) ;
         if( char == '-' )
         {
            continue ;
         }
         else if( char == '+' )
         {
            end = i ;
            if( end - start < 4 )
            {
               state['status'] = 7 ;
               return state ;
            }
            start = i ;
         }
         else
         {
            break ;
         }
      }
      state['status'] = 3 ;
      return state ; 
   }
   var parseSSQLContent = function( content, state ){
      var len, start, end ;
      len = sqlStrlen( content ) ;
      state['isJoin'] = false ;
      if( len != state['length'] )
      {
         state['status'] = 7 ;
         return state ;
      }
      if( str.charAt(0) == '|' )
      {
         state['status'] = 4 ;
         state['value'] = [] ;
         start = 0 ;
         for( var i = 0; i < state['header'].length; ++i )
         {
            start = start + 2 ;
            state['value'].push( trim( content.substr( start, state['header'][i] ) ) ) ;
            start = start + state['header'][i] + 1 ;
         }
      }
      else if( str.charAt(0) == '+' )
      {
         state['status'] = 5 ;
      }
      else
      {
         state['status'] = 7 ;
      }
      return state ; 
   }
   var parseSSQLAttr = function( attr, state ){
      state['status'] = 6 ;
      state['value'] = [] ;
      if( typeof( state['attr'] ) == 'undefined' )
      {
         state['attr'] = [] ;
      }
      attr = trim( attr ) ;
      if( attr.length > 0 )
      {
         state['attr'].push( attr ) ;
      }
      return state ;
   }
   switch( state['status'] )
   {
   case 0:
      state = parseSSQLHeader( str, state ) ;
      break ;
   case 1:
      state = parseSSQLField( str, state ) ;
      break ;
   case 2:
      state = parseSSQLEndHeader( str, state ) ;
      break ;
   case 3:
   case 4:
      state = parseSSQLContent( str, state ) ;
      break ;
   case 5:
   case 6:
      state = parseSSQLAttr( str, state ) ;
      break ;
   case 8:
      state = parseSSQLResult( str, state ) ;
   }
   return state ;
}

//sql转义
function sqlEscape( sql )
{
   sql = sql.replace( /\\/g, "\\\\" ) ;
   return "'" + sql.replace( /'/g, "\\'" ) + "'" ;
}

//添加双引号
function addQuotes( field )
{
   return '"' + field + '"' ;
}

//判断IP地址
function IsIPAddress( ip )
{
   if( typeof( ip ) != 'string' )
   {
      return false ;
   }
   if( ip.indexOf( '.' ) <= 0 )
   {
      return false ;
   }
   var ipArr = ip.split( '.' ) ;
   if( ipArr.length != 4 )
   {
      return false ;
   }
   if( isNaN( ipArr[0] ) == true || parseInt( ipArr[0] ) < 0 || parseInt( ipArr[0] ) > 255 )
   {
      return false ;
   }
   if( isNaN( ipArr[1] ) == true || parseInt( ipArr[1] ) < 0 || parseInt( ipArr[1] ) > 255 )
   {
      return false ;
   }
   if( isNaN( ipArr[2] ) == true || parseInt( ipArr[2] ) < 0 || parseInt( ipArr[2] ) > 255 )
   {
      return false ;
   }
   if( isNaN( ipArr[3] ) == true || parseInt( ipArr[3] ) < 0 || parseInt( ipArr[3] ) > 255 )
   {
      return false ;
   }
   return true ;
}

/*
 * 解析ip ip段 hostname  hostname段
 * 返回数组  [ { 'HostName': 'xxx' }, { 'IP': 'xxx' } ]
 */
function parseAddress( address )
{
	var link_search = [] ;
	var splitAddress = address.split( /[,\s;]/ ) ;
	var splitLen = splitAddress.length ;
	
	for( var strNum = 0; strNum < splitLen; ++strNum )
	{
		var str = $.trim( splitAddress[ strNum ] ) ;
		var host_search = [] ;
		var matches = new Array() ;
		//识别主机字符串，扫描主机
		var reg = new RegExp(/^((.*)(\[[ ]*(\d+)[ ]*\-[ ]*(\d+)[ ]*\])(.*))$/) ;
		if ( ( matches = reg.exec( str ) ) != null )
		{
			host_search.push( matches[2] ) ;
			host_search.push( matches[4] ) ;
			host_search.push( matches[5] ) ;
			host_search.push( matches[6] ) ;
		}
		else
		{
			host_search = str ;
		}
	
		if ( host_search.length > 0 )
		{
			//转换hostname
			if ( isArray( host_search ) )
			{
				var str_start = host_search[0] ;
				var str_end   = host_search[3] ;
				var strlen_num = host_search[1].length ;
				var strlen_temp  = parseInt(host_search[1]).toString().length ;
				var need_add_zero = false ;
				if ( strlen_num > strlen_temp )
				{
					need_add_zero = true ;
				}
            var startNum = parseInt( host_search[1] ) ;
            var endNum   = parseInt( host_search[2] ) ;
            if( startNum > endNum )
            {
               startNum = endNum ;
               endNum   = parseInt( host_search[1] ) ;
            }
				for ( var i = startNum; i <= endNum ; ++i )
				{
               if( IsIPAddress( str_start + i + str_end ) )
               {
                  //IP地址
                  if ( need_add_zero && i.toString().length <= strlen_num )
					   {
						   link_search.push( { 'IP': str_start + pad(i,strlen_num) + str_end } ) ;
					   }
					   else
					   {
						   link_search.push( { 'IP': str_start + i + str_end } ) ;
					   }
               }
               else
               {
					   if ( need_add_zero && i.toString().length <= strlen_num )
					   {
						   link_search.push( { 'HostName': str_start + pad(i,strlen_num) + str_end } ) ;
					   }
					   else
					   {
						   link_search.push( { 'HostName': str_start + i + str_end } ) ;
					   }
               }
				}
			}
			else
			{
            if( IsIPAddress( host_search ) == true )
            {
               link_search.push( { 'IP': host_search } ) ;
            }
            else
            {
               link_search.push( { 'HostName': host_search } ) ;
            }
			}
		}
	}
	return link_search ;
}

//解析主机列表
function parseHostString( hostStr )
{
	var i = 0 ;
	var tempHostList = [] ;
	var hostList = [] ;
	if( hostStr.indexOf( ',' ) >= 0 )
   {
      addressList = hostStr.split( ',' ) ;
   }
	else if( hostStr.indexOf( "\n" ) >= 0 )
   {
      addressList = hostStr.split( "\n" ) ;
   }
   else
   {
      addressList = [ hostStr ] ;
   }
	$.each( addressList, function( index, address ){
		var temp = parseAddress( address ) ;
		$.each( temp, function( index2, hostInfo ){
			tempHostList.push( hostInfo ) ;
			++i ;
			if( i === 5 )
			{
				hostList.push( tempHostList ) ;
				tempHostList = [] ;
				i = 0 ;
			}
		} ) ;
	} ) ;
	if( tempHostList.length > 0 )
	{
		hostList.push( tempHostList ) ;
	}
	return hostList ;
}

//检查列表中是否存在主机或IP
//-1 不存在  -2 存在,并且重复  >= 0 存在，返回下标
function checkHostIsExist( _hostList, hostName, IP )
{
	var rc = -1 ;
	$.each( _hostList, function( index, hostInfo ){
		if( ( hostInfo['HostName'] !== '' && hostName !== '' &&
				hostInfo['HostName'] === hostName ) ||
			 ( hostInfo['IP'] !== '' && IP !== '' &&
				hostInfo['IP'] === IP ) )
		{
			if( ( hostInfo['HostName'] !== '' && hostName !== '' &&
				   hostInfo['HostName'] === hostName ) &&
				 ( hostInfo['IP'] !== '' && IP !== '' &&
				   hostInfo['IP'] !== IP ) )
			{
				rc = -2 ;
				return true ;
			}
			if( ( hostInfo['HostName'] !== '' && hostName !== '' &&
				   hostInfo['HostName'] !== hostName ) &&
				 ( hostInfo['IP'] !== '' && IP !== '' &&
				   hostInfo['IP'] === IP ) )
			{
				rc = -2 ;
				return true ;
			}
			rc = index ;
			return false ;
		}
	} ) ;
	return rc ;
}

//保留两位小数
function twoDecimalPlaces( num )
{
	return ( Math.round( num * 100 ) / 100 ) ;
}

/*
   自动换算容量
   num 单位 MB
*/
function sizeConvert( num )
{
	var rn = '0 MB' ;
	if ( num < 1 && num > 0 )
	{
		rn = ( num * 1024 ) + ' KB' ;
	}
	if( num >= 1 && num < 1024 )
	{
		rn = num + ' MB' ;
	}
	if( num >= 1024 && num < 1048576 )
	{
		rn = twoDecimalPlaces( num / 1024 ) + ' GB' ;
	}
	else if ( num >= 1048576 && num < 1073741824 )
	{
		rn = twoDecimalPlaces( num / 1048576 ) + ' TB' ;
	}
	else if ( num >= 1073741824 )
	{
		rn = twoDecimalPlaces( num / 1073741824 ) + ' PB' ;
	}
	return rn ;
}

//检测端口
function checkPort( str )
{
	var len = str.length ;
	if ( len <= 0 )
	{
		return false ;
	}
	if ( str.charAt( 0 ) == '0' )
	{
		return false ;
	}
	for ( var i = 0; i < len; ++i )
	{
		var char = str.charAt( i ) ;
		if ( char < '0' || char > '9' )
		{
			return false ;
		}
	}
	var port = parseInt( str ) ;
	if ( port <= 0 || port > 65535 )
	{
		return false ;
	}
	return true ;
}

//解析布尔值
function parseBoolean( val )
{
   var newVal = false ;
   if( val == 'true' )
   {
      newVal = true ;
   }
   else if( val == 'false' )
   {
      newVal = false ;
   }
   return newVal ;
}

//从路径去除 role svcname groupname hostname
function selectDBPath( dbpath, role, svcname, groupname, hostname )
{
	if( 'role' === groupname || 'role' === hostname || 'svcname' === groupname || 'svcname' === hostname || 'groupname' === groupname || 'groupname' === hostname || 'hostname' === groupname || 'hostname' === hostname )
	{
		return dbpath ;
	}
	var replaceTemp = '' ;
	function filterSlash( str )
	{
		var len = str.length ;
		var replaceTemp2 = replaceTemp ;
		replaceTemp2 = '/' + replaceTemp2 ;
		if( str.charAt( len - 1 ) === '/' )
		{
			replaceTemp2 = replaceTemp2 + '/' ;
		}
		return replaceTemp2 ;
	}
	if( role !== null && role !== '' )
	{
		var reg = new RegExp( '/' + role + '/|/' + role + '$', 'g' ) ;
		replaceTemp = '[role]' ;
		dbpath = dbpath.replace( reg, filterSlash ) ;
	}
	if( svcname !== null && svcname !== '' )
	{
		var reg = new RegExp( '/' + svcname + '/|/' + svcname + '$', 'g' ) ;
		replaceTemp = '[svcname]' ;
		dbpath = dbpath.replace( reg, filterSlash ) ;
	}
	if( groupname !== null && groupname !== '' )
	{
		var reg = new RegExp( '/' + groupname + '/|/' + groupname + '$', 'g' ) ;
		replaceTemp = '[groupname]' ;
		dbpath = dbpath.replace( reg, filterSlash ) ;
	}
	if( hostname !== null && hostname !== '' )
	{
		var reg = new RegExp( '/' + hostname + '/|/' + hostname + '$', 'g' ) ;
		replaceTemp = '[hostname]' ;
		dbpath = dbpath.replace( reg, filterSlash ) ;
	}
	return dbpath ;
}

/*
 * 数据路径转义
 * 参数1 路径字符串，例子 /opt/sequoiadb/[role]/[svcname]/[groupname]/[hostname] 可用的特殊命令就是 [role] [svcname] [groupname] [hostname]
 * 参数2 主机名
 * 参数3 端口
 * 参数4 角色
 * 参数5 分区组名
 */
function dbpathEscape( str, hostname, svcname, role, groupname )
{
	var newPath = '' ;
	while( true )
	{
		var leftNum = str.indexOf( '[' ) ;
		var rightNum = -1 ;
		if( leftNum >= 0 )
		{
			newPath += str.substring( 0, leftNum ) ;
			str = str.substring( leftNum ) ;
			rightNum = str.indexOf( ']' ) ;
			if( rightNum >= 0 )
			{
				var order = str.substring( 1, rightNum ) ;
				if( order == 'hostname' )
				{
					newPath += hostname + '' ;
				}
				else if( order == 'svcname' )
				{
					newPath += svcname + '' ;
				}
				else if( order == 'role' )
				{
					newPath += role + '' ;
				}
				else if( order == 'groupname' )
				{
					newPath += groupname + '' ;
				}
				else
				{
					newPath += str.substring( 0, rightNum + 1 ) ;
				}
				str = str.substring( rightNum + 1 ) ;
			}
			else
			{
				newPath += str ;
				break ;
			}
		}
		else
		{
			newPath += str ;
			break ;
		}
	}
	return newPath ;
}

/*
 * 端口转义
 * 参数1 端口字符串，例子 '11810[+10]'
 * 参数2 第几个节点 最小值 0
 */
function portEscape( str, num )
{
	var newPort = null ;
	str = str + '' ;
	if( str == '' )
	{
		return str ;
	}
	if( str.indexOf( '[' ) > 0 )
	{
		var portStr = str.substring( 0, str.indexOf( '[' ) ) ;
		var escapeStr = str.substring( str.indexOf( '[' ) ) ;
		var n = 1 ;
		if( escapeStr.charAt(0) == '[' && escapeStr.charAt(escapeStr.length - 1) == ']' )
		{
			if( escapeStr.charAt(1) == '+' )
			{
				n = 1 ;
			}
			else if( escapeStr.charAt(1) == '-' )
			{
				n = -1 ;
			}
			else
			{
				return null ;
			}
         if( isNaN( escapeStr.substring( 2, escapeStr.length - 1 ) ) == true )
         {
            return null ;
         }
         if( isNaN( portStr ) == true )
         {
            return null ;
         }
			var tempNum = parseInt( escapeStr.substring( 2, escapeStr.length - 1 ) ) * num * n ;
			newPort = '' + ( parseInt( portStr ) + tempNum ) ;
		}
		else
		{
			return null ;
		}
	}
	else
	{
		newPort = str ;
	}
	if( checkPort( newPort ) )
	{
		return newPort ;
	}
	else
	{
		return null ;
	}
}

//删除json对象里面指定字段
function deleteJson( json, keys )
{
   var newJson = {} ;
   $.each( json, function( key, value ){
      if( keys.indexOf( key ) == -1 )
      {
         newJson[key] = value ;
      }
   } ) ;
   return newJson ;
}

//设置json的值都为字符串
function convertJsonValueString( json )
{
   $.each( json, function( key, value ){
      if( typeof( value ) == "string" )
      {
         return true ;
      }
      else if( typeof( value ) == "number" )
      {
         json[key] = '' + value ;
      }
      else if( typeof( value ) == "boolean" )
      {
         json[key] = value ? 'true' : 'false' ;
      }
      else
      {
         json[key] = value ;
      }
   } ) ;
   return json ;
}

//JSON.parse JSON.stringify
{
   var _json_parse = null;
   var _json_string = null;
   if (typeof (JSON) != 'undefined') {
      _json_parse = JSON.parse;
      _json_string = JSON.stringify;
   }

   //过滤不可见字符
   function filterInviChart(str) {
      var i = 0, len = str.length;
      var newStr = '';
      var chars, code;
      while (i < len) {
         chars = str.charAt(i);
         code = chars.charCodeAt();
         if (code < 0x20 || code == 0x7F) {
            chars = '?';
         }
         newStr += chars;
         ++i;
      }
      return newStr;
   }

   //json字符串转js对象
   function json2Obj(str, func) {
      var json;
      try {
         json = _json_parse( str ) ;
      } catch (e) {
         try {
            var newStr = filterInviChart(str);
            json = _json_parse(newStr, func);
         } catch (e) {
            json = eval('(' + str + ')');
         }
      }
      return json;
   }

   JSON.parse = json2Obj;

   var rx_one = /^[\],:{}\s]*$/;
   var rx_two = /\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g;
   var rx_three = /"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g;
   var rx_four = /(?:^|:|,)(?:\s*\[)+/g;
   var rx_escapable = /[\\\"\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;
   var rx_dangerous = /[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;

   function f(n) {
      return n < 10
          ? "0" + n
          : n;
   }

   function this_value() {
      return this.valueOf();
   }

   if (typeof Date.prototype.toJSON !== "function") {

      Date.prototype.toJSON = function () {

         return isFinite(this.valueOf())
             ? this.getUTCFullYear() + "-" +
                     f(this.getUTCMonth() + 1) + "-" +
                     f(this.getUTCDate()) + "T" +
                     f(this.getUTCHours()) + ":" +
                     f(this.getUTCMinutes()) + ":" +
                     f(this.getUTCSeconds()) + "Z"
             : null;
      };

      Boolean.prototype.toJSON = this_value;
      Number.prototype.toJSON = this_value;
      String.prototype.toJSON = this_value;
   }

   var gap;
   var indent;
   var meta;
   var rep;


   function quote(str) {
      rx_escapable.lastIndex = 0;
      return rx_escapable.test(str)
          ? "\"" + str.replace(rx_escapable, function (a) {
             var c = meta[a];
             return typeof c === "string"
                 ? c
                 : "\\u" + ("0000" + a.charCodeAt(0).toString(16)).slice(-4);
          }) + "\""
          : "\"" + str + "\"";
   }


   function str(key, holder) {

      var i;
      var k;
      var v;
      var length;
      var mind = gap;
      var partial;
      var value = holder[key];

      if (value && typeof value === "object" &&
              typeof value.toJSON === "function") {
         value = value.toJSON(key);
      }

      if (typeof rep === "function") {
         value = rep.call(holder, key, value);
      }

      switch (typeof value) {
         case "string":
            return quote(value);

         case "number":
            if (value === Number.POSITIVE_INFINITY) {
               return 'Infinity';
            }
            else if (value === Number.NEGATIVE_INFINITY) {
               return '-Infinity';
            }
            else if (value === Number.NaN) {
               return '0';
            }
            else {
               return String(value);
            }

         case "boolean":
         case "null":
            return String(value);

         case "object":
            if (!value) {
               return "null";
            }
            gap += indent;
            partial = [];

            if (Object.prototype.toString.apply(value) === "[object Array]") {
               length = value.length;
               for (i = 0; i < length; i += 1) {
                  partial[i] = str(i, value) || "null";
               }

               v = partial.length === 0
                   ? "[]"
                   : gap
                       ? "[\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "]"
                       : "[" + partial.join(",") + "]";
               gap = mind;
               return v;
            }

            if (rep && typeof rep === "object") {
               length = rep.length;
               for (i = 0; i < length; i += 1) {
                  if (typeof rep[i] === "string") {
                     k = rep[i];
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  }
               }
            } else {

               for (k in value) {
                  if (Object.prototype.hasOwnProperty.call(value, k)) {
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  }
               }
            }

            v = partial.length === 0
                ? "{}"
                : gap
                    ? "{\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "}"
                    : "{" + partial.join(",") + "}";
            gap = mind;
            return v;
      }
   }

   //js对象转成json字符串
   meta = {    // table of character substitutions
      "\b": "\\b",
      "\t": "\\t",
      "\n": "\\n",
      "\f": "\\f",
      "\r": "\\r",
      "\"": "\\\"",
      "\\": "\\\\"
   };
   JSON.stringify = function (value, replacer, space) {

      var i;
      gap = "";
      indent = "";

      if (typeof space === "number") {
         for (i = 0; i < space; i += 1) {
            indent += " ";
         }

      } else if (typeof space === "string") {
         indent = space;
      }

      rep = replacer;
      if (replacer && typeof replacer !== "function" &&
              (typeof replacer !== "object" ||
              typeof replacer.length !== "number")) {
         throw new Error("JSON.stringify");
      }

      return str("", { "": value });
   };
}

//给函数对象增加一个getName的方法，用来返回函数名
Function.prototype.getName = function(){
    return this.name || this.toString().match(/function\s*([^(]*)\(/)[1] ;
}

//判断字符串是否含有中文
function hasChinese( str )
{
   var reg = new RegExp( "[\\u4E00-\\u9FFF]+" ,"g" ) ;
   return reg.test( str ) ;
}

/*
复制指定字段
src: 数据源(数组) [ { ... } ]
field: 指定复制的字段(数组，元素是字符串) [ ... ]
*/
function copyArrayField( src, field )
{
   var dst = [] ;
   var length = src.length ;
   var cpLen  = field.length ;
   for( var i = 0; i < length; ++i )
   {
      var row = {} ;
      for( var k = 0; k < cpLen; ++k )
      {
         row[ field[k] ] = src[i][ field[k] ] ;
      }
      dst.push( row ) ;
   }
   return dst ;
}

/*
   获取json的第一层字段
   json: object的json
*/
function getJsonFirstKeys( json )
{
   var keys = [] ;
   $.each( json, function( key ){
      keys.push( key ) ;
   } ) ;
   return keys ;
}

function parseOneJson( json_array, str, isParseJson, i, len, errType, errJson )
{
   var chars, isJson, isEsc, level, isString, start, json ;

   level = 0, isEsc = false, isString = false, start = i ;

   while( i < len )
	{
      isJson = true ;

		chars = str.charAt( i ) ;

		if( isEsc )
      {
         isEsc = false ;
      }
		else
		{
			if( ( chars === '{' || chars === '[' ) && isString === false )
         {
            ++level ;
         }
			else if( ( chars === '}' || chars === ']' ) && isString === false )
			{
				--level ;
				if( level === 0 )
				{
					++i ;
					subStr = str.substring( start, i ) ;

               if( isParseJson )
               {
                  try
                  {
                     json = JSON.parse( subStr ) ;
                  }
                  catch( e )
                  {
                     isJson = false ;
                     json = { " ": subStr } ;
                  }

                  if( errType == true )
                  {
                     errJson.push( !isJson ) ;
                  }
                  json_array.push( json ) ;
               }
               else
               {
                  json_array.push( subStr ) ;
               }
					break ;
				}
			}
			else if( chars === '"' )
         {
            isString = !isString ;
         }
			else if( chars === '\\' )
         {
            isEsc = true ;
         }
		}
		++i ;
	}

   return i ;
}

//解析响应的Json
function parseJson2( str, isParseJson, errJson )
{
   var jsonArr = [] ;
	var i = 0, len = str.length ;
	var chars, end, subStr, json, errType ;

   errType = isArray( errJson ) ;

	while( i < len )
	{
		while( i < len )
      {
         chars = str.charAt( i ) ;
         if( chars === '{' )
         {
            break ;
         }
         ++i ;
      }

		i = parseOneJson( jsonArr, str, isParseJson, i, len, errType, errJson ) ;
	}

	return jsonArr ;
}