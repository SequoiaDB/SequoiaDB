/************************************************************************
*@Description:   seqDB-5414:记录/字段/字符分隔符使用重复
                      （如记录与字段分隔符重复，记录与字符分隔符重复，记录/字段/字符分隔符全部重复）
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5414";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5414.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   cleanCL( csName, clName );
}

function readyData ( imprtFile )
{
   var file = fileInit( imprtFile );
   file.write( "atest,btest\n1,1\n2,2" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a -r "," -e "," -a ","'
      + ' --file ' + imprtFile;
   assert.tryThrow( 127, function()
   {
      var rc = cmd.run( imprtOption );
   } );
}