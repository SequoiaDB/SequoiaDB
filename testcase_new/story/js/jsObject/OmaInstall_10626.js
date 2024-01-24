/******************************************************************************
*@Description : test js object oma function: getOmaInstallFile getOmaInstallInfo
*               TestLink: 10626 Oma获取Oma安装信息文件路径
*                         10627 Oma获取Oma安装信息                      
*@author      : Liang XueWang
******************************************************************************/

// 测试获取Oma安装文件和安装信息
OmaTest.prototype.testOmaInstall = function()
{
   this.testInit();
   var remote = new Remote( this.hostname, this.svcname );
   var cmd = remote.getCmd();

   // 测试getOmaInstallFile
   var file = this.oma.getOmaInstallFile();
   assert.equal( file, "/etc/default/sequoiadb" );

   // 测试getOmaInstallInfo
   try
   {
      var InstallInfo = this.oma.getOmaInstallInfo().toObj();
      var InstallFileContent = cmd.run( "cat /etc/default/sequoiadb" ).split( "\n" );
      checkOmaInstallInfo( InstallInfo, InstallFileContent );
   } catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }


   this.oma.close();
   remote.close();
}

/******************************************************************************
*@Description : check getOmaInstallInfo
*@author      : Liang XueWang              
******************************************************************************/
function checkOmaInstallInfo ( info, content )
{
   var keys = ["NAME", "SDBADMIN_USER", "INSTALL_DIR", "MD5"];
   for( var i = 0; i < keys.length; i++ )
   {
      var found = false;
      for( var j = 0; j < content.length; j++ )
      {
         content[j] = content[j].replace( / /g, "" );
         var ind = content[j].indexOf( keys[i] );
         if( ind === -1 )
            continue;
         found = true;
         var value1 = content[j].slice( ind + keys[i].length + 1 ).toLowerCase();
         var value2 = info[keys[i]].toString().toLowerCase();
         assert.equal( value1, value2 );
      }
      if( found === false && info[keys[i]] !== "" )
      {
         throw new Error( "checkOmaInstallInfo check key " + keys[i] + info.keys[i] );
      }
   }
}

main( test );

function test ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var staticOma = new OmaTest();

   var omas = [localOma, remoteOma];
   for( var i = 0; i < omas.length; i++ )
   {
      // 测试获取Oma安装文件和安装信息
      omas[i].testOmaInstall();
   }
}

