/******************************************************************************
 * @Description   : seqDB-24251 :: 带--autodelchar并指定--trim right
 * @Author        : luweikang
 * @CreateTime    : 2021.05.26
 * @LastEditTime  : 2021.02.03
 ******************************************************************************/

testConf.clName = COMMCLNAME + "cl_24251B"; 
main( test )

function test ( testPara )
{
   var cl = testPara.testCL;
   // 准备导入配置文件
   var importFile = prepareImportFile();
   var docs = expRecords();
   // 导入命令
   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME + " -l " + testConf.clName + " --file " + importFile + " --fields 'str1,str2,str3' --job 1 --parsers 1 --insertnum 1 --autodelchar true --trim right";
   // 执行导入
   cmd.run( command );

   // 检查结果
   var cursor = cl.find().sort( { "_id": 1 } );
   commCompareResults( cursor, docs );

   File.remove( importFile );
}

function prepareImportFile ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   var filename = tmpFileDir + "test24251B.csv";
   var file = new File( filename );
   file.write( "abc,1   ,ccc\n" );
   file.write( "   abc   ,   2,ccc\n" );
   file.write( "   abc\"   ,3   ,ccc\n" );
   file.write( "   \"abc   ,  4  ,ccc\n" );
   file.write( "\"   abc   \",  5  ,ccc\n" );
   file.write( "\"   abc   ,  6,ccc\n" );
   file.write( "   abc   \", 7  ,ccc\n" );
   file.write( "   \"abc\"   ,  8  ,ccc\n" );

   file.write( "9,   abc   ,ccc\n" );
   file.write( "10  ,   abc\"   ,ccc\n" );
   file.write( "11,   \"abc   ,ccc\n" );
   file.write( "   12,\"   abc   \",ccc\n" );
   file.write( "13,\"   abc   ,ccc\n" );
   file.write( "  14  ,   abc   \",ccc\n" );
   file.write( "  15,   \"abc\"   ,ccc\n" );

   file.write( "16,ccc,   abc   \n" );
   file.write( "  17  ,ccc,   abc\"   \n" );
   file.write( "18  ,ccc,   \"abc   \n" );
   file.write( " 19,ccc,\"   abc   \"\n" );
   file.write( " 20,ccc,\"   abc   \n" );
   file.write( "21  ,ccc,   abc   \"\n" );
   file.write( "22,ccc,   \"abc\"   \n" );
   file.close();
   
   return filename;
}

function expRecords ()
{
   var tmpArrs = new Array();
   tmpArrs.push( { str1: "abc", str2: 1, str3: "ccc" } );
   tmpArrs.push( { str1: "   abc", str2: 2, str3: "ccc" } );
   tmpArrs.push( { str1: "abc", str2: 3, str3: "ccc" } );
   tmpArrs.push( { str1: "abc   ,  4  ,ccc" } );
   tmpArrs.push( { str1: "   abc", str2: 5, str3: "ccc" } );
   tmpArrs.push( { str1: "   abc   ,  6,ccc" } );
   tmpArrs.push( { str1: "abc", str2: 7, str3: "ccc" } );
   tmpArrs.push( { str1: "abc", str2: 8, str3: "ccc" } );

   tmpArrs.push( { str1: 9, str2: "   abc", str3: "ccc" } );
   tmpArrs.push( { str1: 10, str2: "abc", str3: "ccc" } );
   tmpArrs.push( { str1: 11, str2: "abc   ,ccc" } );
   tmpArrs.push( { str1: 12, str2: "   abc", str3: "ccc" } );
   tmpArrs.push( { str1: 13, str2: "   abc   ,ccc" } );
   tmpArrs.push( { str1: 14, str2: "abc", str3: "ccc" } );
   tmpArrs.push( { str1: 15, str2: "abc", str3: "ccc" } );

   tmpArrs.push( { str1: 16, str2: "ccc", str3: "   abc" } );
   tmpArrs.push( { str1: 17, str2: "ccc", str3: "abc" } );
   tmpArrs.push( { str1: 18, str2: "ccc", str3: "abc" } );
   tmpArrs.push( { str1: 19, str2: "ccc", str3: "   abc" } );
   tmpArrs.push( { str1: 20, str2: "ccc", str3: "   abc" } );
   tmpArrs.push( { str1: 21, str2: "ccc", str3: "abc" } );
   tmpArrs.push( { str1: 22, str2: "ccc", str3: "abc" } );

   return tmpArrs
}