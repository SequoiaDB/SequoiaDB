/******************************************************************************
 * @Description   : seqDB-23290 :: 多表并发导出后导入，cl为主子表 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.14
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
var maincsName = "maincs23290";
var mainclName = "maincl23290";
var subcsName = "cs23290"
var subclNames = ["cl23290A", "cl23290B", "cl23290C"];
var docs = new Array();
tmpFileDir += "/23290/";
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   // 准备主子表并插入记录
   var maincl = prepareCSCL();

   // 准备导出配置文件
   var exportConf = prepareExportConf();

   // 导出命令
   var command = installDir + "tools/sdbmigrate/bin/sdbexprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + exportConf + " --jobs 3 --debug";

   cmd.run( command );

   // 删除数据，准备导入
   maincl.remove();

   // 准备导入配置文件
   var importConf = prepareImportConf();
   // 导入命令
   var command = installDir + "tools/sdbmigrate/bin/sdbimprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + importConf + " --jobs 3 --debug";
   cmd.run( command );

   // 检查结果
   var cursor = maincl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 清理环境
   commDropCS( db, maincsName, true );
   commDropCS( db, subcsName, true );
   cmd.run( "rm -rf " + tmpFileDir );
}

function prepareCSCL ()
{
   commDropCS( db, maincsName, true );
   commDropCS( db, subcsName, true );
   var maincs = db.createCS( maincsName );
   var maincl = maincs.createCL( mainclName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 } );
   var subcs = db.createCS( subcsName );
   for( var i = 0; i < subclNames.length; i++ )
   {
      subcs.createCL( subclNames[i], { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0 } );
   }
   maincl.attachCL( subcsName + "." + subclNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( subcsName + "." + subclNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   maincl.attachCL( subcsName + "." + subclNames[2], { LowBound: { a: 200 }, UpBound: { a: 300 } } );
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i * 2, c: i * 3 } );
   }
   maincl.insert( docs );
   return maincl;
}

function prepareExportConf ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   var filename = tmpFileDir + "export23290.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=1\n" );
   file.write( "[collection1]\n" );
   file.write( "name=" + maincsName + "." + mainclName + "\n" );
   file.write( "type=json\n" );
   file.write( "dir=" + tmpFileDir + "\n" );
   return filename;
}

function prepareImportConf ()
{
   var filename = tmpFileDir + "import23290.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=1\n" );
   file.write( "[collection1]\n" );
   file.write( "name=" + maincsName + "." + mainclName + "\n" );
   file.write( "type=json\n" );
   file.write( "file=" + tmpFileDir + maincsName + "." + mainclName + "." + "0.json\n" );
   return filename;
}
