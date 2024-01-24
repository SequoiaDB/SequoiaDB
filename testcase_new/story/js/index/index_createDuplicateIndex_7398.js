/******************************************************************************
@Description : 重复创建索引，验证索引名或索引键重复
@Modify list :
               2016-3-16  yan Wu  init
******************************************************************************/
var clName = CHANGEDPREFIX + "_duplicateIndex7398";
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection   
   var options = { ShardingKey: { a: 1, b: 1 }, ShardingType: "range", Compressed: true };
   var idxCL = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   // create index
   createIndex( idxCL, "testindex", { a: 1 }, false, false );

   // the index name duplicate
   createIndex( idxCL, "testindex", { a: 1 }, false, false, SDB_IXM_REDEF );

   // the index key duplicate
   createIndex( idxCL, "testkey", { a: 1 }, false, false, SDB_IXM_EXIST_COVERD_ONE );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "testindex", "a", 1, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   // drop collection in clean
   commDropCL( db, csName, clName, false, false );
}

