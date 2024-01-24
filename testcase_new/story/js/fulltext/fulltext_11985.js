/***************************************************************************
@Description :seqDB-11985 :创建重复的全文索引 
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

   var clName = COMMCLNAME + "_ES_11985";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引名已存在的全文索引
   var indexName = "a_11985";
   dbcl.createIndex( indexName, { about: 1 } );
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      dbcl.createIndex( indexName, { content: "text" } );
   } );

   //在已存在全文索引定义的集合中，再次创建全文索引
   dbcl.createIndex( "b_11985", { content: "text" } );
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      dbcl.createIndex( "c_11985", { content: "text" } );
   } );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "b_11985" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
