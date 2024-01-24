/******************************************************************************
*@Description : test js object System function: addGroup delGroup listGroups
*                                               isGroupExist
*               TestLink : 10652 System对象添加、删除用户组
*                          10653 System对象添加用户组，用户组id已存在,isUnique为true
*                          10654 System对象添加用户组，用户组id已存在,isUnique为false
*                          10658 System对象枚举用户组
*                          10661 System对象判断用户组是否存在
*@author      : Liang XueWang
******************************************************************************/

// 测试创建删除用户组
SystemTest.prototype.testAddDelGroup = function( isUnique )
{
   this.init();

   // 检查tmpGroup或testgroup是否存在
   deleteGroup( this.hostname, this.svcname, "tmpGroup", this.system );
   deleteGroup( this.hostname, this.svcname, "testGroup", this.system );

   // 检查用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"];
   if( user !== "root" )
   {
      this.release();
      return;
   }

   try
   {
      // 创建tmpGroup
      var gid = getIdleGID( this.cmd );
      var groupObj = {};
      groupObj["name"] = "tmpGroup";
      groupObj["id"] = gid;
      this.system.addGroup( groupObj );
      // 检查用户组
      checkGroup( this.cmd, groupObj );

      // 使用tmpGroup的gid创建testGroup
      groupObj["name"] = "testGroup";
      groupObj["id"] = gid;
      groupObj["isUnique"] = isUnique;
      groupObj["passwd"] = "testGroup";
      this.system.addGroup( groupObj );
      if( isUnique )
      {
         throw new Error( "create unique group with used gid should be fail,ed" );
      }
      // 检查用户组
      checkGroup( this.cmd, groupObj );

      // 删除用户组
      this.system.delGroup( "tmpGroup" );
      this.system.delGroup( "testGroup" );
   }
   catch( e )
   {
      if( ( e.message == 4 || e.message == 16 ) && isUnique )
         ;
      else
         throw e;
   }

   this.release();
}

// 测试枚举用户组
SystemTest.prototype.testListGroups = function()
{
   this.init();

   var groups = this.system.listGroups( { detail: true } ).toArray();
   var command = "cat /etc/group | awk -F : '{print $1,$3,$4}'";
   var info = this.cmd.run( command ).split( "\n" );
   for( var i = 0; i < groups.length; i++ )
   {
      var groupObj = JSON.parse( groups[i] );
      var tmp = info[i].split( " " );
      var name = tmp[0];        // 用户组名
      var gid = tmp[1];         // GID
      var members = tmp[2];     // 用户组成员
      if( name !== groupObj.name || gid !== groupObj.gid ||
         members !== groupObj.members )
      {
         throw new Error( "testListGroups test group info " + this + tmp + groups[i] );
      }
   }

   this.release();
}

// 测试判断用户组是否存在
SystemTest.prototype.testIsGroupExist = function()
{
   this.init();

   var result = this.system.isGroupExist( "root" );
   assert.equal( result, true );
   result = this.system.isGroupExist( "!@#$%" );
   assert.equal( result, false );

   this.release();
}

/******************************************************************************
*@Description : check group after create
*@author      : Liang XueWang            
******************************************************************************/
function checkGroup ( cmd, groupObj )
{
   var command = "cat /etc/group | grep " + groupObj.name;
   var info = cmd.run( command ).split( "\n" )[0];
   var tmp = info.split( ":" );
   var groupName = tmp[0];
   var gid = tmp[2];
   if( groupName !== groupObj.name || gid !== groupObj.id )
   {
      throw new Error( "checkGroup fail,check group info" + tmp + JSON.stringify( groupObj ) );
   }
}

/******************************************************************************
*@Description : get a idle gid from 1000-  0-999 is ocupied by linux system
*@author      : Liang XueWang            
******************************************************************************/
function getIdleGID ( cmd )
{
   var gids = cmd.run( "cat /etc/group | awk -F : '{print $3}'" ).split( "\n" );
   var gid = "1000";
   while( true )
   {
      if( gids.indexOf( gid ) == -1 )
      {
         break;
      }
      gid = gid * 1 + 1 + "";
   }
   return gid;
}


main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试创建删除用户组，gid唯一
      systems[i].testAddDelGroup( true );

      // 测试创建删除用户组，gid不唯一
      systems[i].testAddDelGroup( false );

      // 测试枚举用户组
      systems[i].testListGroups();

      // 测试判断用户组是否存在
      systems[i].testIsGroupExist();
   }
}


