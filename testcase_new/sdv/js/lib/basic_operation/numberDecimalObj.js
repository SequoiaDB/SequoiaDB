function NumberDecimalObj ( date, precisionScale )
{
   try
   {
      if( precisionScale === undefined )
      {
         var decimalObj = new NumberDecimal( date );
      }
      else
      {
         var decimalObj = new NumberDecimal( date, precisionScale );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
   return decimalObj;
}
