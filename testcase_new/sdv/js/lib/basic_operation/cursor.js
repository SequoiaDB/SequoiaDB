/* *****************************************************************************
@discription: return object of Cursor
@author: yinzhen
@parameter:
   cursor: primitive SdbCursor object
***************************************************************************** */
function Cursor ( cursor )
{
   this.current =
      function()
      {
         try
         {
            var obj = cursor.current();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.close =
      function()
      {
         try
         {
            cursor.close();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.next =
      function()
      {
         try
         {
            var obj = cursor.next();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.arrayAccess =
      function( index )
      {
         try
         {
            var obj = cursor.arrayAccess( index );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.toArray =
      function()
      {
         try
         {
            var array = cursor.toArray();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return array;
      }

   this.valueOf =
      function()
      {
         try
         {
            var value = cursor.valueOf();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return value;
      }

   this.toString =
      function()
      {
         try
         {
            var str = cursor.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }
}
