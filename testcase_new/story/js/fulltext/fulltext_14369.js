/***************************************************************************
@Description :seqDB-14369 :创建全文索引，索引名字段长度验证 
@Modify list :
              2018-10-24  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_14369";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //索引名长度为1时，全文索引创建成功
   var indexName = "a_14369";
   dbcl.createIndex( indexName, { content: "text" } );
   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropIndex( dbcl, indexName, true );
   checkIndexNotExistInES( esIndexNames );

   //固定集合名长度小于127时，全文索引创建成功
   var indexName = "";
   for( var i = 0; i < 20; i++ )
   {
      indexName = indexName + "a";
   }
   dbcl.createIndex( indexName, { content: "text" } );
   dropIndex( dbcl, indexName, true );
   checkIndexNotExistInES( esIndexNames );

   //固定集合名长度等于127时，全文索引创建成功
   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var cursor = cursor.next().toObj();
   var cappedCLLength = String( cursor["UniqueID"] ).length + 5;

   var indexName = "";
   for( var i = 0; i < 127 - cappedCLLength; i++ )
   {
      indexName = indexName + "a";
   }
   dbcl.createIndex( indexName, { content: "text" } );

   var dbOperater = new DBOperator();
   var cappedCLName = dbOperater.getCappedCLName( dbcl, indexName );

   commCheckIndexConsistency( dbcl, indexName, true );
   dropIndex( dbcl, indexName, true );
   checkIndexNotExistInES( esIndexNames );

   //SEQUOIADBMAINSTREAM-3896
   //固定集合名长度等于128时，全文索引创建失败
   var indexName = "";
   for( var i = 0; i < 127 - cappedCLLength + 1; i++ )
   {
      indexName = indexName + "a";
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { content: "text" } );
   } );
   checkIndexNotExistInES( esIndexNames );

   //固定集合名长度大于127时，全文索引创建失败
   var indexName = "";
   for( var i = 0; i < 127 - cappedCLLength + 100; i++ )
   {
      indexName = indexName + "a";
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { content: "text" } );
   } );
   checkIndexNotExistInES( esIndexNames );

   dropCL( db, COMMCSNAME, clName, true, true );
}
