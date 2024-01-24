/******************************************************************************
*@Description: seqDB-10660:System对象判断用户是否存在
@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].isUserExist();
   }
}

SystemTest.prototype.isUserExist = function()
{
   this.init();

   var result = this.system.isUserExist( "root" );
   if( !result )
   {
      throw new Error( "root is not exist!" );
   }

   result = this.system.isUserExist( "!@#$%" );
   if( result )
   {
      throw new Error( "!@#$% is exist!" );
   }

   this.release();
}

main( test )
