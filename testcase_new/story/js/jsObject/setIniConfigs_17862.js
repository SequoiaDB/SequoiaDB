/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs                                       
*               TestLink: seqDB-17862:sdb和sdbcm的setIniConfigs支持ini格式，基本功能测试 
*@Author      : 2019-1-18  XiaoNi Huang
*@Info        ：用例执行机可能没有装sequoiadb，本地测试时new Oma()会失败，只能用Cmd；
                而Cmd不能远程执行，所以远程测试时只能使用Remote
******************************************************************************/

main( test );

function test ()
{
   // init and prepare data
   var cmd = new Cmd();
   var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );

   var filePath = WORKDIR + "/" + "config17862.ini";
   initWorkDir( cmd, remote );
   cmd.run( "rm -f " + filePath );

   var iniData = { "A.a1": 1, "A.a2": 2, "B.b1": 3, "B.b2": 4, "C": 5 };
   var expGetData = '{"A.a1":"1","A.a2":"2","B.b1":"3","B.b2":"4","C":"5"}';
   var expFileData = '["C=\\"5\\"","","[A]","a1=\\"1\\"","a2=\\"2\\"","","[B]","b1=\\"3\\"","b2=\\"4\\"",""]';

   // sdb test
   sdbSetIniConf( iniData, filePath );

   var actData = readLocalFile( cmd, filePath );
   checkResult( expFileData, actData );

   var actData = sdbGetIniConf( filePath );
   checkResult( expGetData, actData );

   // clear local data
   cmd.run( "rm -f " + filePath );


   // sdbcm test 
   sdbcmSetIniConf( oma, iniData, filePath );

   var actData = readRemoteFile( remote, filePath );
   checkResult( expFileData, actData );

   var actData = sdbcmGetIniConf( oma, filePath );
   checkResult( expGetData, actData );

   // clear remote data
   var file = remote.getFile();
   file.remove( filePath );
}

function sdbSetIniConf ( data, filePath )
{
   Oma.setIniConfigs( data, filePath );
}

function sdbGetIniConf ( filePath )
{
   var rc = Oma.getIniConfigs( filePath );
   return JSON.stringify( rc.toObj() );
}

function sdbcmSetIniConf ( oma, data, filePath )
{
   oma.setIniConfigs( data, filePath );
}

function sdbcmGetIniConf ( oma, filePath )
{
   var rc = oma.getIniConfigs( filePath );
   return JSON.stringify( rc.toObj() );
}

function readLocalFile ( cmd, filePath )
{
   var rc = cmd.run( 'cat ' + filePath ).split( "\n" );
   return JSON.stringify( rc );
}

function readRemoteFile ( remote, filePath )
{
   var file = remote.getFile( filePath );
   var rc = file.read().split( "\n" );
   return JSON.stringify( rc );
}

function checkResult ( expData, actData )
{
   assert.equal( expData, actData );
}

function initWorkDir ( cmd, remote ) 
{
   // localhost
   try
   {
      cmd.run( "ls " + WORKDIR );
   }
   catch( e ) 
   {
      if( 2 == e.message )   // 2: No such file or directory
      {
         cmd.run( "mkdir -p " + WORKDIR );
      }
      else
      {
         throw e;
      }
   }

   // remote host
   var file = remote.getFile();
   var dirExist = file.exist( WORKDIR );
   if( false === dirExist )
   {
      commMakeDir( COORDHOSTNAME, WORKDIR );
   }
}