/******************************************************************************
*@Description: seqDB-10659:System对象获取当前用户信息
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].getCurrentUser();
   }
}

SystemTest.prototype.getCurrentUser = function()
{
   this.init();

   var userObj = this.system.getCurrentUser().toObj();
   var name = this.cmd.run( "whoami 2>/dev/null" ).split( "\n" )[0];
   var gid = this.cmd.run( "id -g " + name + " 2>/dev/null" ).split( "\n" )[0];
   var tmp = this.cmd.run( "echo ~" + name ).split( "\n" );
   var dir = tmp[tmp.length - 2];
   if( name !== userObj.user || gid !== userObj.gid || dir !== userObj.dir )
   {
      throw new Error( "userObj: " + JSON.stringify( userObj ) );
   }

   this.release();
}

main( test );
