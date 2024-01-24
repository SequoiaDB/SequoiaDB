/************************************************************************
*@Description:  seqDB-19175:科学计数法，底数只有整数，且全为0（如000E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'json';
   var tmpPrefix = "sdbimprt_19175";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   var expRecs = '[{"a":0,"b":0},{"a":1,"b":0}]';
   var findCond = { "b": { "$type": 2, "$et": "double" } };
   checkCLData( cl, recsNum, expRecs, findCond );

   // clean data
   cmd.run( "rm -rf " + importFile );
   cleanCL( csName, clName );
}

function initImportFile_testPoint ( importFile )
{
   var recordsNum = 2;
   var str = '{"a":0,"b":000E+308}' + '\n' + '{"a":1,"b":000E+400}';
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}