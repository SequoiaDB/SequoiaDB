/******************************************************************************
 * @Description   : seqDB-23313:conf配置文件覆盖原有sdbimprt工具所有参数
 * @Author        : liuli
 * @CreateTime    : 2021.10.09
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
// 记录分隔符/字段分隔符/字符串分隔符包含特殊字符
var csName = "cs_23313a";
var clNames = ["cl_23313a_1", "cl_23313a_2", "cl_23313a_3"];
var dataFileDir = tmpFileDir + "cl23313a/"

main( test );
function test ()
{
   commDropCS( db, csName );
   var docs = [];
   for( var i = 0; i < 100; i++ ) 
   {
      docs.push( { a: i + '', b: i * 3 + '', c: i * 6 + '' } );
   }

   for( var i = 0; i < clNames.length; i++ )
   {
      var dbcl = commCreateCL( db, csName, clNames[i], { ReplSize: 0 } );
      dbcl.insert( docs );
   }

   cmd.run( "rm -rf " + dataFileDir );
   cmd.run( "mkdir -p " + dataFileDir );
   // 生成 记录分隔符/字段分隔符/字符串分隔符
   var sep = randomString();

   // 准备导出配置文件
   var exportConf = prepareExportConf( sep );

   // 导出命令
   var command = installDir + "tools/sdbmigrate/bin/sdbexprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + exportConf + " --jobs 3 --debug";

   // 执行导出
   cmd.run( command );

   // 删除数据准备导入
   for( var i = 0; i < clNames.length; i++ )
   {
      var dbcl = db.getCS( csName ).getCL( clNames[i] );
      dbcl.remove();
   }

   // 准备导入配置文件
   var importConf = prepareImportConf( sep );

   // 导入命令
   var command = installDir + "tools/sdbmigrate/bin/sdbimprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + importConf + " --jobs 3 --debug";
   // 执行导入
   cmd.run( command );

   // 检查结果
   docs.sort( sortBy( "a" ) );
   for( var i = 0; i < clNames.length; i++ )
   {
      var cursor = db.getCS( csName ).getCL( clNames[i] ).find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }

   commDropCS( db, csName );
   cmd.run( "rm -rf " + dataFileDir );
}

// 准备导出配置文件
function prepareExportConf ( sep )
{
   var filename = dataFileDir + "export23313a.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csName + "." + clNames[i - 1] + "\n" );
      file.write( "type=csv\n" );
      file.write( "fields=a,b,c\n" )
      file.write( "dir=" + dataFileDir + "\n" );
      file.write( "delrecord=" + sep[0] + "\n" );
      file.write( "delfield=" + sep[1] + "\n" );
      file.write( "delchar=" + sep[2] + "\n" );
   }
   return filename;
}

// 准备导入配置文件
function prepareImportConf ( sep )
{
   var filename = dataFileDir + "import23313a.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csName + "." + clNames[i - 1] + "\n" );
      file.write( "file=" + dataFileDir + csName + "." + clNames[i - 1] + ".0.csv\n" );
      file.write( "fields=a,b,c\n" );
      file.write( "delrecord=" + sep[0] + "\n" );
      file.write( "delfield=" + sep[1] + "\n" );
      file.write( "delchar=" + sep[2] + "\n" );
      file.write( "headerline=true\n" )
   }
   return filename;
}

// return 记录分隔符/字段分隔符/字符串分隔符
function randomString ()
{
   var separator = [];
   for( var i = 0; i < 3; i++ )
   {
      var str = String.fromCharCode( ( Math.floor( Math.random() * 57 ) + 65 ) ) + '';
      separator.push( str );
   }
   println( "separator -- " + separator );
   // 生成的分隔符包含\`abc时重新生成
   var inc = ( ( separator.indexOf( "\\" ) != -1 ) ||
      ( separator.indexOf( "`" ) != -1 ) || ( separator.indexOf( "a" ) != -1 ) ||
      ( separator.indexOf( "b" ) != -1 ) || ( separator.indexOf( "c" ) != -1 ) );
   // 生成的3种分隔符重复时重新生成
   if( isRepeat( separator ) || inc )
   {
      var separator = randomString();
   }
   return separator;
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

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}