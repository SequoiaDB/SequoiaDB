/************************************************************************
*@Description:   seqDB-5404:指定目录导入数据，目录/文件不存在
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5404";
   var cl = readyCL( csName, clName );

   importData( csName, clName );

   cleanCL( csName, clName );
}

function importData ( csName, clName )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv'
      + ' --file ' + tmpFileDir + 'aa/bb/cc.csv';
   assert.tryThrow( 127, function()
   {
      cmd.run( imprtOption );
   } );
}