/* *****************************************************************************
@discription: return object of CollectionSpace
@author: yinzhen
@parameter:
   cs: primitive SdbCS object
***************************************************************************** */
function CollectionSpace ( cs )
{
   this.renameCL =
      function( oldname, newname )
      {
         try
         {
            cs.renameCL( oldname, newname );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.dropCL =
      function( name )
      {
         try
         {
            cs.dropCL( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.createCL =
      function( name, options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            var cl = cs.createCL( name, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Collection( cl );
      }

   this.getCL =
      function( name )
      {
         try
         {
            var cl = cs.getCL( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Collection( cl );
      }

   this.setDomain =
      function( options )
      {
         try
         {
            cs.setDomain( options );
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
            cs.alter( options );
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
            cs.setAttributes( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.removeDomain =
      function()
      {
         try
         {
            cs.removeDomain();
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
            var str = cs.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }
}
