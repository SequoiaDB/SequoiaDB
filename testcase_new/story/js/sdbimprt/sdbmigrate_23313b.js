/******************************************************************************
 * @Description   : seqDB-23313:conf配置文件覆盖原有sdbimprt工具所有参数
 * @Author        : liuli
 * @CreateTime    : 2021.10.11
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
// 指定参数包含datefmt和timestampfmt，导入字段名包含特殊字符
var csName = "cs_23313b";
var clNames = ["cl_23313b_1", "cl_23313b_2", "cl_23313b_3", "cl_23313b_4", "cl_23313b_5", "cl_23313b_6"];
tmpFileDir += "/23313b/";
var dataFileDir = tmpFileDir + "cl23313b/"

main( test );
function test ()
{
   commDropCS( db, csName );
   for( var i = 0; i < clNames.length; i++ )
   {
      commCreateCL( db, csName, clNames[i] );
   }

   var sdbimprtOption = ["datefmt=DD-MM-YYYY",
      "datefmt=MM:DD:YYYY",
      "datefmt=DD*YYYY*MM",
      "timestampfmt=YYYY-MM-DD-HH.mm.ss.ffffff",
      "timestampfmt=YYYY/MM/DD HH:mm:ss:ffffff",
      "timestampfmt=HH.mm.ss.ffffff YYYY-MM-DD"];

   cmd.run( "rm -rf " + dataFileDir );
   cmd.run( "mkdir -p " + dataFileDir );
   // 生成导入的字段名
   var fields = randomString();

   // 准备导入文件
   readyData();

   // 准备导入配置文件
   var importConf = prepareImportConf( fields, sdbimprtOption );

   // 导入命令
   var command = installDir + "tools/sdbmigrate/bin/sdbimprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + importConf + " --jobs 3 --debug";
   // 执行导入
   cmd.run( command );

   // 检查结果
   var expRecs = [];
   var bson = {};
   bson[fields[0]] = 1;
   bson[fields[1]] = { "$date": "1900-01-01" };
   expRecs.push( bson );
   for( var i = 0; i < 3; i++ )
   {
      var cursor = db.getCS( csName ).getCL( clNames[i] ).find().sort( { a: 1 } );
      commCompareResults( cursor, expRecs );
   }

   var expRecs = [];
   var bson = {};
   bson[fields[0]] = 1;
   bson[fields[1]] = { "$timestamp": "2037-12-31-23.59.59.999999" };
   expRecs.push( bson );
   for( var i = 3; i < 6; i++ )
   {
      var cursor = db.getCS( csName ).getCL( clNames[i] ).find().sort( { a: 1 } );
      commCompareResults( cursor, expRecs );
   }

   commDropCS( db, csName );
   cmd.run( "rm -rf " + tmpFileDir );
}

// 准备导入配置文件
function prepareImportConf ( fields, sdbimprtOption )
{
   var filename = tmpFileDir + "import23313b.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=6\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csName + "." + clNames[i - 1] + "\n" );
      file.write( "file=" + dataFileDir + clNames[i - 1] + ".csv\n" );
      if( i <= 3 )
      {
         file.write( "fields=" + fields[0] + " int," + fields[1] + " date\n" );
      }
      else
      {
         file.write( "fields=" + fields[0] + " int," + fields[1] + " timestamp\n" );
      }
      file.write( sdbimprtOption[i - 1] + "\n" );
   }
   return filename;
}

// 准备导入文件
function readyData ()
{
   var file = fileInit( dataFileDir + clNames[0] + ".csv" );
   file.write( '1,01-01-1900' );
   file.close();
   var file = fileInit( dataFileDir + clNames[1] + ".csv" );
   file.write( '1,01:01:1900' );
   file.close();
   var file = fileInit( dataFileDir + clNames[2] + ".csv" );
   file.write( '1,01*1900*01' );
   file.close();
   var file = fileInit( dataFileDir + clNames[3] + ".csv" );
   file.write( '1,2037-12-31-23.59.59.999999' );
   file.close();
   var file = fileInit( dataFileDir + clNames[4] + ".csv" );
   file.write( '1,2037/12/31 23:59:59:999999' );
   file.close();
   var file = fileInit( dataFileDir + clNames[5] + ".csv" );
   file.write( '1,23.59.59.999999 2037-12-31' );
   file.close();
}

// 随机生成字段名
function randomString ()
{
   var fields = [];
   for( var i = 0; i < 2; i++ )
   {
      var str = String.fromCharCode( ( Math.floor( Math.random() * 57 ) + 65 ) ) + '';
      fields.push( str );
   }
   println( "fields -- " + fields );
   // 字段名\`时重新生成
   var inc = ( ( fields.indexOf( "\\" ) != -1 ) || ( fields.indexOf( "`" ) != -1 ) );
   // 字段名重复时重新生成
   if( isRepeat( fields ) || inc )
   {
      var fields = randomString();
   }
   return fields;
}

// 判断是否有重复元素
function isRepeat ( arr )
{
   var hash = {};
   for( var i in arr )
   {
      if( hash[arr[i]] )
      {
         return true;
      }
      hash[arr[i]] = true;
   }
   return false;
}