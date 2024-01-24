/* *****************************************************************************
@discription: return new File -> new FileObj
@author: yinzhen
@parameter
   filepath:String
   permission:int
   mode:int
@e.g.
   var file = new FileObj(filepath, permission, mode);
***************************************************************************** */
function FileObj ( filepath, permission, mode )
{
   if( permission === undefined ) { permission = 0700; }
   if( mode === undefined ) { mode = SDB_FILE_READWRITE | SDB_FILE_CREATE; }
   try
   {
      var file = new File( filepath, permission, mode );
   }
   catch( e )
   {
      throw new Error( e );
   }

   this.write =
      function( content )
      {
         try
         {
            file.write( content );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.close =
      function()
      {
         try
         {
            file.close();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }
}
