function ObjectIdObj ( data )
{
   try
   {
      if( data === undefined )
      {
         var obj = new ObjectId();
      }
      else
      {
         var obj = new ObjectId( data );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
   return obj;
}
