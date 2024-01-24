/************************************************************************
*@Description:   seqDB-5402:指定目录导入json/csv文件数据，目录为空
                 seqDB-6211:file指定导入，目录不存在
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5402";
   var cl = readyCL( csName, clName );

   importData( csName, clName );

   cleanCL( csName, clName );
}

function importData ( csName, clName )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a'
      + ' --file ' + tmpFileDir;
   assert.tryThrow( 127, function()
   {
      cmd.run( imprtOption );
   } );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a'
      + ' --file ' + tmpFileDir + "notExistsDir";

   assert.tryThrow( 127, function()
   {
      cmd.run( imprtOption );
   } );
}