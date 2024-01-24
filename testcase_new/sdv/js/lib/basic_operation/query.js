/* *****************************************************************************
@discription: return object of Query
@author: yinzhen
@parameter:
   query: primitive SdbQuery object
***************************************************************************** */
function Query ( query )
{
   this.sort =
      function( sort )
      {
         try
         {
            query = query.sort( sort );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.next =
      function()
      {
         try
         {
            var obj = query.next();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.current =
      function()
      {
         try
         {
            var obj = query.current();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.count =
      function()
      {
         try
         {
            var count = query.count();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new CLCountObj( count );
      }

   this.explain =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            var cursor = query.explain( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.toArray =
      function()
      {
         try
         {
            var array = query.toArray();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return array;
      }

   this.close =
      function()
      {
         try
         {
            query.close();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.hint =
      function( hint )
      {
         try
         {
            query = query.hint( hint );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.limit =
      function( num )
      {
         try
         {
            query = query.limit( num );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.skip =
      function( num )
      {
         try
         {
            query = query.skip( num );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.update =
      function( rule, returnNew, options )
      {
         if( returnNew === undefined ) { returnNew = false; }
         if( options === undefined ) { options = {} }
         try
         {
            query = query.update( rule, returnNew, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.remove =
      function()
      {
         try
         {
            query = query.remove();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }
}
