function _parseOneJson( json_array, str, isParseJson, i, len, errType, errJson )
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

function _isArray( object ) {
   if( typeof( object ) == 'undefined' )
      return false ;
   if( object === null )
      return false ;
   //判断length属性是否是可枚举的 对于数组 将得到false
   return object && typeof( object ) === 'object' && typeof( object.length ) === 'number' &&
            typeof( object.splice ) === 'function' && !( object.propertyIsEnumerable( 'length' ) ) ;
}

function _object2postArg( object )
{
   var res = '' ;

   if( typeof( object ) == 'object' )
   {
      var i = 0 ;
      for( var key in object )
      {
         if ( i > 0 )
         {
            res += '&' ;
         }

         res += encodeURIComponent( key ) + '=' ;

         if ( typeof( object[key] ) == 'object' )
         {
            res += encodeURIComponent( JSON.stringify( object[key] ) ) ;
         }
         else
         {
            res += encodeURIComponent( object[key] ) ;
         }
         ++i ;
      }
   }

   return res ;
}

//解析响应的Json
function _parseJson2( str, isParseJson, errJson )
{
   var jsonArr = [] ;
   var i = 0, len = str.length ;
   var chars, end, subStr, json, errType ;

   errType = _isArray( errJson ) ;

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

      i = _parseOneJson( jsonArr, str, isParseJson, i, len, errType, errJson ) ;
   }

   return jsonArr ;
}