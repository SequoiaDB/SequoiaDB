/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs                                       
*               TestLink: seqDB-17868:sdb和sdbcm的setIniConfigs支持ini格式，ini字段数据类型测试 
*@Author      : 2019-3-7  XiaoNi Huang
*@Info		  : sdbcm interface test
******************************************************************************/

main( test );

function test ()
{
   // init and prepare data
   var cmd = new Cmd();
   var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );

   var filePath = WORKDIR + "/" + "config17868_sdbcm.ini";
   initWorkDir( remote );
   cmd.run( "rm -f " + filePath );

   var iniData = {
      "Int.v1": -2147483648, "Int.v2": 2147483647, "Int.v3": -2147483649, "Int.v4": 2147483648,
      "Number.v1": -9007199254740992, "Number.v2": 9007199254740992, "Number.v3": -9007199254740993, "Number.v4": 9007199254740993,
      "Long.v1": -9223372036854775808, "Long.v2": 9223372036854775807, "Long.v3": -9223372036854776000, "Long.v4": 9223372036854776000,
      "Float.v1": -1.7E+308, "Float.v2": 1.7E+308, "Float.v3": -1.8E+308, "Float.v4": 1.8E+308,
      "String.v1": "ALL", "String.v2": "", "String.v3": " ",
      "Bool.v1": true, "Bool.v2": false,
      //"Null.v1": null,    ----not support
      "Nu.v1": undefined,
      "A": 1
   };
   var expFileData = '["A=1","","[Bool]","v1=true","v2=false","","[Float]","v1=-1.7e+308","v2=1.7e+308","v3=-inf","v4=inf","","[Int]","v1=-2147483648","v2=2147483647","v3=-2147483649","v4=2147483648","","[Long]","v1=-9.223372036854776e+18","v2=9.223372036854776e+18","v3=-9.223372036854776e+18","v4=9.223372036854776e+18","","[Number]","v1=-9007199254740992","v2=9007199254740992","v3=-9007199254740992","v4=9007199254740992","","[String]","v1=\\"ALL\\"","v2=\\"\\"","v3=\\" \\"",""]';
   var expGetData = '{"A":1,"Bool.v1":true,"Bool.v2":false,"Float.v1":-1.7e+308,"Float.v2":1.7e+308,"Float.v3":"-inf","Float.v4":"inf","Int.v1":-2147483648,"Int.v2":2147483647,"Int.v3":-2147483649,"Int.v4":2147483648,"Long.v1":-9223372036854773000,"Long.v2":9223372036854773000,"Long.v3":-9223372036854773000,"Long.v4":9223372036854773000,"Number.v1":{"$numberLong":"-9007199254740992"},"Number.v2":{"$numberLong":"9007199254740992"},"Number.v3":{"$numberLong":"-9007199254740992"},"Number.v4":{"$numberLong":"9007199254740992"},"String.v1":"ALL","String.v2":"","String.v3":" "}';

   // sdbcm test 
   sdbcmSetIniConf( oma, iniData, filePath );

   var actData = readRemoteFile( remote, filePath );
   checkResult( expFileData, actData );

   var actData = sdbcmGetIniConf( oma, filePath );
   checkResult( expGetData, actData );

   // clear remote data
   var file = remote.getFile();
   file.remove( filePath );

   oma.close();
}

function sdbcmSetIniConf ( oma, data, filePath )
{
   oma.setIniConfigs( data, filePath, { EnableType: true } );
}

function sdbcmGetIniConf ( oma, filePath )
{
   var rc = oma.getIniConfigs( filePath, { EnableType: true } );
   return JSON.stringify( rc.toObj() );
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

function initWorkDir ( remote ) 
{
   var file = remote.getFile();
   var dirExist = file.exist( WORKDIR );
   if( false === dirExist )
   {
      commMakeDir( COORDHOSTNAME, WORKDIR );
   }
}