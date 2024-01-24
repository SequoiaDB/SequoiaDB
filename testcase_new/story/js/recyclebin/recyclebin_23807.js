/******************************************************************************
 * @Description   : seqDB-23807:hash分区表，写入lob并切分到多个数据组，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.12
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23807";
testConf.clOpt = { ShardingKey: { "date": 1 }, AutoSplit: true };

main( test );
function test ( args )
{
   var filePath = WORKDIR + "/lob23807/";
   var fileName = "filelob_23807";
   var fileSize = 1024 * 1024;
   var clName = testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;
   var dbcs = db.getCS( COMMCSNAME );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOids = insertLob( dbcl, filePath + fileName );

   // 删除cl后恢复
   dbcs.dropCL( clName );
   var recycleName = getOneRecycleName( db, COMMCSNAME + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后进行校验
   for( var i = 0; i < lobOids.length; i++ )
   {
      dbcl.getLob( lobOids[i], filePath + "checkputlob23807" + "i", true );
      var actMD5 = File.md5( filePath + "checkputlob23807" + "i" );
      assert.equal( fileMD5, actMD5 );
   }
   checkRecycleItem( recycleName );

   deleteTmpFile( filePath );
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