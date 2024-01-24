/* *****************************************************************************
@discription: return object of Collection
@author: yinzhen
@parameter:
   cl: primitive SdbCollection object
***************************************************************************** */
function Collection ( cl )
{
   this.count =
      function( cond )
      {
         if( cond === undefined ) { cond = {}; }
         try
         {
            var count = cl.count( cond );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return count;
      }

   this.insert =
      function( docs, options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            var obj = cl.insert( docs, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.find =
      function( cond, sel )
      {
         if( cond === undefined ) { cond = {}; }
         if( sel === undefined ) { sel = {}; }
         try
         {
            var query = cl.find( cond, sel );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Query( query );
      }

   this.createAutoIncrement =
      function( options )
      {
         try
         {
            cl.createAutoIncrement( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.update =
      function( rule, cond, hint, options )
      {
         if( cond === undefined ) { cond = {}; }
         if( hint === undefined ) { hint = {}; }
         if( options === undefined ) { options = {}; }
         try
         {
            var obj = cl.update( rule, cond, hint, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.remove =
      function( cond, hint )
      {
         if( cond === undefined ) { cond = {}; }
         if( hint === undefined ) { hint = {}; }
         try
         {
            var obj = cl.remove( cond, hint );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.putLob =
      function( filePath, oid )
      {
         try
         {
            if( oid === undefined )
            {
               var obj = cl.putLob( filePath );
            }
            else
            {
               var obj = cl.putLob( filePath, oid );
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.createLobID =
      function( time )
      {
         try
         {
            if( time === undefined )
            {
               var obj = cl.createLobID();
            }
            else
            {
               var obj = cl.createLobID( time );
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.listLobs =
      function( sdbQueryOption )
      {
         try
         {
            if( sdbQueryOption === undefined )
            {
               var cursor = cl.listLobs();
            }
            else
            {
               var cursor = cl.listLobs( sdbQueryOption );
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.getLob =
      function( oid, filePath, forced )
      {
         if( forced === undefined ) { forced = false; }
         try
         {
            cl.getLob( oid, filePath, forced );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.createIndex =
      function( name, indexDef, isUnique, enforced, sortBufferSize )
      {
         if( isUnique === undefined ) { isUnique = false; }
         if( enforced === undefined ) { enforced = false; }
         if( sortBufferSize === undefined ) { sortBufferSize = 64; }
         try
         {
            cl.createIndex( name, indexDef, isUnique, enforced, sortBufferSize );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.dropIndex =
      function( name )
      {
         try
         {
            cl.dropIndex( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getIndex =
      function( name )
      {
         try
         {
            var obj = cl.getIndex( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.split =
      function( sourceGroup, targetGroup, condition, endcondition )
      {
         if( endcondition === undefined ) { endcondition = {}; }
         try
         {
            cl.split( sourceGroup, targetGroup, condition, endcondition );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.attachCL =
      function( subCLFullName, options )
      {
         try
         {
            cl.attachCL( subCLFullName, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.splitAsync =
      function( sourceGroup, targetGroup, condition, endcondition, options )
      {
         if( endcondition === undefined ) { endcondition = {}; }
         if( options === undefined ) { options = {}; }
         try
         {
            var taskID = cl.splitAsync( sourceGroup, targetGroup, condition, endcondition, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return taskID;
      }

   this.truncateLob =
      function( oid, length )
      {
         try
         {
            cl.truncateLob( oid, length );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.truncate =
      function()
      {
         try
         {
            cl.truncate();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.listIndexes =
      function()
      {
         try
         {
            var cursor = cl.listIndexes();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.deleteLob =
      function( oid )
      {
         try
         {
            cl.deleteLob( oid );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.setAttributes =
      function( options )
      {
         try
         {
            cl.setAttributes( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.enableCompression =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            cl.enableCompression( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.alter =
      function( options )
      {
         try
         {
            cl.alter( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.detachCL =
      function( subCLFullName )
      {
         try
         {
            cl.detachCL( subCLFullName );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.disableSharding =
      function()
      {
         try
         {
            cl.disableSharding();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.enableSharding =
      function( options )
      {
         try
         {
            cl.enableSharding( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.upsert =
      function( rule, cond, hint, setOnInsert, options )
      {
         if( cond === undefined ) { cond = {}; }
         if( hint === undefined ) { hint = {}; }
         if( setOnInsert === undefined ) { setOnInsert = {}; }
         if( options === undefined ) { options = {}; }
         try
         {
            var obj = cl.upsert( rule, cond, hint, setOnInsert, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.createIdIndex =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            cl.createIdIndex( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.findOne =
      function( cond, sel )
      {
         if( cond === undefined ) { cond = null; }
         if( sel === undefined ) { sel = null; }
         try
         {
            var query = cl.findOne( cond, sel );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Query( query );
      }

   this.aggregate =
      function( subOp )
      {
         try
         {
            var query = cl.aggregate( subOp );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Query( query );
      }

   this.dropIdIndex =
      function()
      {
         try
         {
            cl.dropIdIndex();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }
   
   this.toString =
      function()
      {
         try
         {
            var str = cl.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }
}
