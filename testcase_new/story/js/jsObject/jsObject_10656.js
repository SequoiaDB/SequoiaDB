/******************************************************************************
*@Description: seqDB-10656:System对象枚举已登录用户
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].listLoginUsers();
   }
}

SystemTest.prototype.listLoginUsers = function()
{
   this.init();

   var users = this.system.listLoginUsers( { detail: true } ).toArray();
   var info = this.cmd.run( "who | sed 's/  */ /g'" ).split( "\n" );
   for( var i = 0; i < users.length; i++ )
   {
      var userObj = JSON.parse( users[i] );
      var tmp = info[i].split( " " );
      var len = tmp.length;
      var username = tmp[0];             // 用户名
      var tty = tmp[1];                  // 登录终端
      var time = tmp[2];                 // 登录时间
      for( var j = 3; j < len; j++ )
      {
         if( tmp[j].indexOf( "(" ) !== -1 )
            break;
         time += " " + tmp[j];
      }
      var addr;                          // 登录的主机名或者ip
      if( tmp[j] === undefined )
      {
         addr = "";
      }
      else
      {
         addr = tmp[j].slice( 1, tmp[j].length - 1 );
      }

      if( username !== userObj.user || tty !== userObj.tty || time !== userObj.time || addr !== userObj.from )
      {
         throw new Error( "userObj: " + JSON.stringify( userObj ) );
      }
   }

   this.release();
}

main( test );
