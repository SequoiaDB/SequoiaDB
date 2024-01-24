/******************************************************************************
*@Description: seqDB-10657:System对象枚举所有用户
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].listAllUsers();
   }
}

SystemTest.prototype.listAllUsers = function()
{
   this.init();
   var users = this.system.listAllUsers( { detail: true } ).toArray();
   var info = this.cmd.run( "cat /etc/passwd" ).split( "\n" );
   for( var i = 0; i < users.length; i++ )
   {
      var userObj = JSON.parse( users[i] );
      var tmp = info[i].split( ":" );
      var username = tmp[0];    // 用户名
      var groupid = tmp[3];     // 用户组id
      var dir = tmp[5];         // 用户主目录
      if( username !== userObj.user || groupid !== userObj.gid || dir !== userObj.dir )
      {
         throw new Error( "userObj: " + JSON.stringify( userObj ) );
      }
   }

   this.release();
}

main( test );

