/***************************************************************************
@Description :seqDB-11993 :创建全文索引接口参数校验 
@Modify list :
              2018-10-25  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11993";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引类型非法的全文索引
   var indexName = "a_11993";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { content: "int" } );
   } );

   //创建非法的复合索引
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { content: "text", about: 1 } );
   } );

   //指定isUnique、enforced、sortBufferSize创建全文索引
   dbcl.createIndex( indexName, { content: "text" }, true, true, 128 );

   dbcl.insert( [{ content: "a" }, { content: "a" }] );
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, null, { content: "" } );
   var expResult = [{ content: "a" }, { content: "a" }];

   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
