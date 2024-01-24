/******************************************************************************
 * @Description   : seqDB-1539:大数据量+记录+lob+索引+压缩
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.07.12
 * @LastEditTime  : 2021.09.07
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_11539";
main( test );

function test ( args )
{
   var varCL = args.testCL;
   var filePath = WORKDIR + "/lob1539b/";
   var fileName = "filelob_1539b";
   var fileSize = 1024 * 1024;

   varCL.createIndex( "aIndex", { a: 1 } );
   var cur = varCL.listIndexes();
   var indexName = null;
   var obj = null;
   var obj2 = null;
   var obj3 = null;
   while( ( obj = cur.next() != null ) )
   {
      obj = cur.next().toObj();
      indexName = obj.IndexDef.name
   }
   cur.close();
   assert.equal( indexName, "aIndex" );

   var insertCount1 = 10000;
   var insertDocs = [];
   for( var i = 0; i < insertCount1; i++ )
   {
      insertDocs.push( { a: i, b: i + 1, c: i } );
   }
   varCL.insert( insertDocs );

   var rc = varCL.find();
   assert.equal( rc.count(), insertCount1 );

   varCL.update( { $set: { b: "x" } } );

   varCL.remove( { a: { $gte: 5 } } );
   var docs = [];
   for( i = 0; i < 5; i++ ) 
   {
      docs.push( { a: i, b: "x", c: i } );
   }
   var rc = varCL.find();
   commCompareResults( rc, docs );

   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = varCL.putLob( filePath + fileName );

   varCL.getLob( lobID, filePath + "checkputlob1539b", true );
   var actMD5 = File.md5( filePath + "checkputlob1539b" );
   assert.equal( fileMD5, actMD5 );

   var rc = varCL.listLobs();
   while( ( obj2 = cur.next() != null ) )
   {
      var obj2 = rc.current().toObj()
      var actOid = obj2.Oid.oid;
      assert.equal( actOid, lobID );
   }
   rc.close();

   varCL.deleteLob( lobID );

   var rcDelete = varCL.listLobs();
   while( ( obj3 = cur.next() != null ) )
   {
      var obj3 = rc.current().toObj()
      var actOid = obj3.Oid.oid;
      assert.equal( actOid, lobID );
   }
   rcDelete.close();

   deleteTmpFile( filePath );
}

function deleteTmpFile ( filePath )
{
   try
   {
      File.remove( filePath );
   } catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }
}

function makeTmpFile ( filePath, fileName, fileSize )
{
   if( fileSize == undefined ) { fileSize = 1024 * 100; }
   var fileFullPath = filePath + "/" + fileName;
   File.mkdir( filePath );

   var cmd = new Cmd();
   cmd.run( "dd if=/dev/zero of=" + fileFullPath + " bs=1c count=" + fileSize );
   var md5Arr = cmd.run( "md5sum " + fileFullPath ).split( " " );
   var md5 = md5Arr[0];
   return md5
}


