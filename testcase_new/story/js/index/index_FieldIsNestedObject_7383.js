/*************************************************************************
@Description : Creating index by use nest object as the key .
@Modify list :
               2014-5-18  xiaojun Hu  Modify
               2016-2-27  yan wu Modify:删除函数inspecIndex( indexName , indexKey , keyValue , idxUnique , idxEnforced )，
                                        更新为调用公共函数inspecIndex；
                                        增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）
*************************************************************************/

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, csName, clName );

   varCL.createIndex( "testindex", { "use.id": 1 }, true );
   inspecIndex( varCL, "testindex", "use.id", 1, true, false );

   varCL.insert( { use: { id: 1, name: "chen" } } );
   varCL.insert( { use: { id: 2, name: "chen" } } );

   checkExplain( varCL, { "use.id": 1 }, "ixscan", "testindex" );

   var rc = varCL.find( { "use.id": 1 } );
   var expRecs = [];
   expRecs.push( { use: { id: 1, name: "chen" } } );
   checkRec( rc, expRecs );

   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      varCL.insert( { use: { id: 1, name: "chensdf" } } );
   } );
   commDropCL( db, csName, clName, false, false );
}

