/******************************************************************************
 * @Description   : seqDB-23826:hash分区表，写入lob并切分到多个数据组，truncate后恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.12
 * @LastEditTime  : 2021.08.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var filePath = WORKDIR + "/lob23826/";
   var fileName = "filelob_23826";
   var fileSize = 1024 * 1024;
   var csName = "cs_23826";
   var clName = "cl_23826";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commCreateCS( db, csName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建分区表，指定自动切分
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { "ShardingKey": { "date": 1 }, AutoSplit: true } );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOids = insertLob( dbcl, filePath + fileName );

   // 执行truncate后恢复
   dbcl.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后进行校验
   for( var i = 0; i < lobOids.length; i++ )
   {
      dbcl.getLob( lobOids[i], filePath + "checkputlob23826" + "i", true );
      var actMD5 = File.md5( filePath + "checkputlob23826" + "i" );
      assert.equal( fileMD5, actMD5 );
   }
   checkRecycleItem( recycleName );

   deleteTmpFile( filePath );
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function insertLob ( dbcl, filePath )
{
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;
   var lobOids = [];

   for( var j = 0; j < 10; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + j ) + "-00.00.00.000000";
      var lobOid = dbcl.createLobID( timestamp );
      var lobOid = dbcl.putLob( filePath, lobOid );
      lobOids.push( lobOid );
   }
   return lobOids;
}