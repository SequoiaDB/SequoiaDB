/******************************************************************************
*@Description: seqDB-10651:System对象添加用户，已存在用户
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].addExistUser();
   }
}

SystemTest.prototype.addExistUser = function()
{
   this.init();

   // 检查用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"];
   if( user !== "root" )
   {
      this.release();
      return;
   }
   try
   {
      this.system.addUser( { name: "root" } );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != 9 )
      {
         throw e;
      }
   }

   this.release();
}

main( test );
