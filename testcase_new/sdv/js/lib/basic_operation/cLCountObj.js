function CLCountObj ( count )
{
   this.hint =
      function( hint )
      {
         try
         {
            count = count.hint( hint );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return this;
      }

   this.valueOf =
      function()
      {
         try
         {
            var num = count.valueOf();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return num;
      }

   this.toString =
      function()
      {
         try
         {
            var str = count.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }
}
