import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

//构造JSON字符串
function BuildObjStr ( obj )
{
   var objstr;
   if( '[object Array]' === toString.apply( obj ) )
   {
      objstr = "[";
      var first = true;
      for( item in obj )
      {
         if( !first )
         {
            objstr += ", ";
         }
         else
         {
            first = false;
         }

         if( "object" == typeof ( obj[item] ) )
         {
            objstr += BuildObjStr( obj[item] );
         }
         else if( "string" == typeof ( obj[item] ) )
         {
            objstr += ( "'" + obj[item] + "'" );
         }
         else
         {
            objstr += obj[item];
         }
      }
      objstr += "]";
   }
   else
   {
      objstr = "{";
      var first = true;
      for( item in obj )
      {
         if( !first )
         {
            objstr += ", ";
         }
         else
         {
            first = false;
         }
         if( "object" == typeof ( obj[item] ) )
         {
            objstr = objstr + item + ":";
            objstr += BuildObjStr( obj[item] );
         }
         else if( "string" == typeof ( obj[item] ) )
         {
            objstr += ( item + ":'" + obj[item] + "'" );
         }
         else
         {
            objstr += ( item + ":" + obj[item] );
         }
      }
      objstr += "}";
   }
   return objstr;
}

function compareObj ( lobj, robj, needObjectID )
{
   if( typeof ( lobj ) !== "object" ||
      typeof ( robj ) !== "object" )
   {
      return lobj === robj;
   }

   if( lobj === null || robj === null )
   {
      return lobj === robj;
   }

   if( undefined === needObjectID )
   {
      var needObjectID = false;
   }

   var lkeys = Object.getOwnPropertyNames( lobj );
   var rkeys = Object.getOwnPropertyNames( robj );
   if( ( needObjectID && lkeys.length !== rkeys.length ) ||
      ( !needObjectID && Math.abs( lkeys.length - rkeys.length ) !== 1 ) )
   {
      return false;
   }

   for( k in lobj )
   {
      if( needObjectID === false && k === "_id" )
      {
         continue;
      }

      if( robj[k] === undefined )
      {
         return false;
      }

      if( typeof ( lobj[k] ) === "object" &&
         typeof ( robj[k] ) === "object" )
      {
         if( !compareObj( lobj[k], robj[k], true ) )
         {
            return false;
         }
      }
      else if( typeof ( lobj[k] ) === "object" ||
         typeof ( robj[k] ) === "object" )
      {
         return false;
      }
      else if( lobj[k] !== robj[k] )
      {
         return false;
      }
   }
   return true;
}

function checkResult ( cl, cond, resultSet )
{
   if( "undefined" === typeof ( cl ) )
   {
      return true;
   }
   if( "undefined" === typeof ( cond ) )
   {
      var cond = {};
   }

   if( "undefined" === typeof ( resultSet ) )
   {
      var resultSet = [];
   }

   var tmp = resultSet.concat();
   var realRes = [];
   var docNum = 0;
   var check = true;
   var cursor = cl.find( cond );
   while( cursor.next() )
   {
      var doc = cursor.current().toObj();
      var i = 0;
      var find = false;
      if( check )
      {
         for( ; i < tmp.length; ++i )
         {
            if( compareObj( doc, tmp[i], false ) )
            {
               tmp.splice( i, 1 );
               find = true;
               break;
            }
         }
      }

      if( !find )
      {
         check = false;
      }
      realRes.push( doc );
   }

   if( !check || realRes.length != resultSet.length )
   {
      throw new Error( "expect:" + JSON.stringify( resultSet ) + "real:" + JSON.stringify( realRes ) );
   }

}

function upsertandmergerWithHint ( cl, rule, cond, hint, setoninsert, res, errres )
{
   if( undefined == rule )
   {
      return;
   }

   if( undefined == cond )
   {
      var cond = {};
   }

   try
   {
      cl.upsert( rule, cond, hint, setoninsert );
      docnum = cl.find( res ).count();
      if( 1 != docnum )
      {
         throw new Error( -1 );
      }

      if( undefined != errres )
      {
         docnum = cl.find( errres ).count();
         if( 0 != docnum )
         {
            throw new Error( -2 );
         }
      }
   }
   catch( e )
   {
      if( SDB_IO == e.message )
      {
         throw new Error( e );
      }
      else if( SDB_OOM == e.message && undefined != errres )
      {
         throw new Error( e );
      }
      else
      {
         throw e;
      }
   }
   finally
   {
      cl.remove();
   }
}

function upsertandmerger ( cl, rule, cond, res, errres )
{
   if( undefined == rule )
   {
      return;
   }

   if( undefined == cond )
   {
      var cond = {};
   }

   try
   {
      cl.upsert( rule, cond );
      docnum = cl.find( res ).count();
      if( 1 != docnum )
      {
         throw new Error( -1 );
      }

      if( undefined != errres )
      {
         docnum = cl.find( errres ).count();
         if( 0 != docnum )
         {
            throw new Error( -2 );
         }
      }
   }
   catch( e )
   {
      if( SDB_IO == e.message )
      {
         throw new Error( e );
      }
      else if( SDB_OOM == e.message && undefined != errres )
      {
         throw new Error( e );
      }
      else
      {
         throw e;
      }
   }
   finally
   {
      cl.remove();
   }
}
