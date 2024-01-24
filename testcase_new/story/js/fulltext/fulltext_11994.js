/***************************************************************************
@Description :seqDB-11994 :删除不存在的全文索引 
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

   var clName = COMMCLNAME + "_ES_11994";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //删除存在的全文索引，删除成功
   var indexName = "a_11994";
   commCreateIndex( dbcl, indexName, { content: "text" } );
   commCheckIndexConsistency( dbcl, indexName, true );
   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropIndex( dbcl, indexName );

   //删除不存在的全文索引，删除失败
   commCheckIndexConsistency( dbcl, indexName, false );
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      dbcl.dropIndex( indexName );
   } );
   checkIndexNotExistInES( esIndexNames );

   dropCL( db, COMMCSNAME, clName, true, true );
}
