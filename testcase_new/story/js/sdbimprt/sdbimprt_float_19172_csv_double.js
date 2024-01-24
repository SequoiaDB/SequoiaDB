/************************************************************************
*@Description:  seqDB-19172:科学计数法，底数为小数点+小数位，小数位包含有效数字（.001E+310）
         import type: double
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/


main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_double_19172";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b double';

   // init import file and expect records
   var recsNum = initImportFile_testPoint1( importFile );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCount( cl, recsNum );
   var limitNum = 30; // expect data changes, other only ensure normal import
   var expRecs = initExpectData_testPoint1( limitNum );
   var findCond = { "b": { "$type": 2, "$et": "double" } };
   checkRecords( cl, limitNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );


   // init import file and expect records
   var recsNum = initImportFile_testPoint2( importFile );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCount( cl, recsNum );
   var limitNum = 40; // expect data changes, other only ensure normal import
   var expRecs = initExpectData_testPoint2( limitNum );
   var findCond = { "b": { "$type": 2, "$et": "double" } };
   checkRecords( cl, limitNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );

   // init import file and expect records
   var recsNum = initImportFile_testPoint3( importFile );
   var expRecs = initExpectData_testPoint3( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );

   cleanCL( csName, clName );
}

function initImportFile_testPoint1 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 307;
   // 0, b value e.g: ".01E" / ".001E"......
   var str = "";
   var bVal = ".0";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + "," + bVal + "1E\n";
      bVal += "0";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint2 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: ".000000000000001E++0" / ".000000000000001E+1"......".000000000000001E+400"
   var str = "";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + ",.000000000000001E+" + i + "\n";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint3 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: ".01E" / ".011E"......
   var str = "";
   var bVal = "1";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + ",.0" + bVal + "E\n";
      bVal += "1";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = '[{"a":0,"b":0.01},{"a":1,"b":0.001},{"a":2,"b":0.0001},{"a":3,"b":0.00001},{"a":4,"b":0.000001},{"a":5,"b":1e-7},{"a":6,"b":1e-8},{"a":7,"b":1e-9},{"a":8,"b":1e-10},{"a":9,"b":1e-11},{"a":10,"b":1e-12},{"a":11,"b":1e-13},{"a":12,"b":1e-14},{"a":13,"b":1e-15},{"a":14,"b":1e-16},{"a":15,"b":1e-17},{"a":16,"b":1e-18},{"a":17,"b":1e-19},{"a":18,"b":1e-20},{"a":19,"b":1e-21},{"a":20,"b":1e-22},{"a":21,"b":1e-23},{"a":22,"b":1e-24},{"a":23,"b":9.999999999999999e-26},{"a":24,"b":9.999999999999999e-27},{"a":25,"b":1e-27},{"a":26,"b":1e-28},{"a":27,"b":1e-29},{"a":28,"b":9.999999999999999e-31},{"a":29,"b":1e-31}]';
   return expRecs;;
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = '[{"a":0,"b":1e-15},{"a":1,"b":1e-14},{"a":2,"b":1e-13},{"a":3,"b":1e-12},{"a":4,"b":1e-11},{"a":5,"b":1e-10},{"a":6,"b":1e-9},{"a":7,"b":1e-8},{"a":8,"b":1e-7},{"a":9,"b":0.000001},{"a":10,"b":0.00001},{"a":11,"b":0.0001},{"a":12,"b":0.001},{"a":13,"b":0.01},{"a":14,"b":0.1},{"a":15,"b":1},{"a":16,"b":10},{"a":17,"b":100},{"a":18,"b":1000},{"a":19,"b":10000},{"a":20,"b":100000},{"a":21,"b":1000000},{"a":22,"b":10000000},{"a":23,"b":100000000},{"a":24,"b":1000000000},{"a":25,"b":10000000000},{"a":26,"b":100000000000},{"a":27,"b":1000000000000},{"a":28,"b":10000000000000},{"a":29,"b":100000000000000},{"a":30,"b":1000000000000000},{"a":31,"b":10000000000000000},{"a":32,"b":100000000000000000},{"a":33,"b":1000000000000000000},{"a":34,"b":10000000000000000000},{"a":35,"b":100000000000000000000},{"a":36,"b":1e+21},{"a":37,"b":1e+22},{"a":38,"b":1e+23},{"a":39,"b":1e+24}]';
   return expRecs;
}

function initExpectData_testPoint3 ( expRecsNum )
{
   var expRecs = [];
   var record;
   var bVal = "0.01";
   for( var i = 0; i < expRecsNum; i++ )
   {
      record = { "a": i, "b": Number( bVal ) };
      if( i < 15 )
      {
         bVal += "1";
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function checkCount ( cl, expRecsNum )
{
   var cnt = cl.count();
   assert.equal( cnt, expRecsNum );
}

function checkRecords ( cl, expRecsNum, expRecs, cond )
{

   var rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } ).limit( expRecsNum );

   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   // check count
   var actCnt = recsArray.length;
   assert.equal( actCnt, expRecsNum );

   // check records
   var actRecs = JSON.stringify( recsArray );
   assert.equal( actRecs, expRecs );
}