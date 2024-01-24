/*******************************************************************
* @Description : test export with --filelimit
*                seqDB-13524:指定文件大小=数据量大小
*                seqDB-13525:指定文件大小<数据量大小，导出数据到json
*                seqDB-13537:指定文件大小<数据量大小，导出数据到csv 
* @author      : chensiqin 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13524";

main( test );

function test ()
{
   commDropCL( db, csname, clname );
   var cl = commCreateCL( db, csname, clname );
   var exportDir = tmpFileDir + "13524/";
   var kb = 4;

   //指定文件大小=数据量大小 
   testFilelimit13524( cl, kb, "44K", "json", exportDir );
   cl.truncate();
   //指定文件大小<数据量大小json
   testFilelimit13525( cl, kb, "2K", "json", exportDir );
   cl.truncate();

   //指定文件大小<数据量大小csv
   testFilelimit13537( cl, kb, "2K", "csv", exportDir );

   commDropCL( db, csname, clname );

   // clean *.rec file
   var tmpRec = csname + "_" + clname + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

//指定文件大小=数据量大小
function testFilelimit13524 ( cl, kb, filelimit, fileType, exportDir )
{
   var expSize = 44 * 1024;
   insertDocs( cl, 4 );
   testExprtWithLimit( filelimit, fileType, exportDir )
   //校验导出后文件是否正确
   checkExprtfileSize( exportDir, fileType, expSize );
   testImportWithLimit( filelimit, fileType, exportDir );
   checkCl( cl, kb );
}

//指定文件大小<数据量大小 json
function testFilelimit13525 ( cl, kb, filelimit, fileType, exportDir )
{
   var expSize = 2 * 1024;
   insertDocs( cl, 4 );
   testExprtWithLimit( filelimit, fileType, exportDir )
   //校验导出后文件是否正确
   checkExprtfileSize( exportDir, fileType, expSize );
   testImportWithLimit( filelimit, fileType, exportDir );
   checkCl( cl, kb );
}

//指定文件大小<数据量大小 csv
function testFilelimit13537 ( cl, kb, filelimit, fileType, exportDir )
{
   var expSize = 2 * 1024;
   insertDocs( cl, 4 );
   testExprtWithLimit( filelimit, fileType, exportDir )
   //校验导出后文件是否正确
   checkExprtfileSize( exportDir, fileType, expSize );
   testImportWithLimit( filelimit, fileType, exportDir );
   checkCl( cl, kb );
}

function insertDocs ( cl, kb )
{
   var bytes = kb * 1024;//4*1024=4K  4096条记录  一条记录等于11个字节
   var doc = [];
   for( var i = 1; i < bytes; i++ )
   {
      doc.push( { a: 1 } )
   }
   cl.insert( doc );
}

function testExprtWithLimit ( filelimit, fileType, exportDir )
{
   cmd.run( "mkdir -p " + exportDir );
   var exportfile = exportDir + "sdbexprt13524." + fileType;
   cmd.run( "rm -rf " + exportfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + exportfile +
      " --filelimit " + filelimit +
      " --type " + fileType +
      " --fields a";

   testRunCommand( command );

}

function testImportWithLimit ( filelimit, fileType, exportDir )
{
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + exportDir +
      " --type " + fileType +
      " --fields='a int'";
   testRunCommand( command );

   cmd.run( "rm -rf " + exportDir );
}

function checkCl ( cl, kb )
{
   if( parseInt( cl.find( { a: 1 } ).count() ) !== 2 * ( kb * 1024 - 1 ) )
   {
      throw new Error( "testFileLimit fail check import cl count" + 2 * ( kb * 1024 - 1 ) + cl.find( { a: 1 } ).count() );
   }
}

function checkExprtfileSize ( exportDir, fileType, expSize )
{
   var file = exportDir + "sdbexprt13524." + fileType;
   var actSize = parseInt( File.stat( file ).toObj()["size"] );
   if( actSize > expSize )
   {
      throw new Error( "testExprtImprtJson fail,check file size" + expSize + actSize );
   }
}