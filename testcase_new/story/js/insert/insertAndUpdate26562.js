/******************************************************************************
 * @Description   : seqDB-26562:普通表只指定FLG_INSERT_UPDATEONDUP/UpdateOnDup，单条/批量插入冲突记录
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.17
 * @LastEditTime  : 2022.05.17
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26562";

main( test );

function test ( testPara )
{
   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }] );

   //只指定FLG_INSERT_UPDATEONDUP，单条插入冲突记录
   cl.insert( { a: 1, b: 2 }, SDB_INSERT_UPDATEONDUP );
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 2 }, { a: 2, b: 2 }];
   commCompareResults( actRes, expRes );

   //只指定FLG_INSERT_UPDATEONDUP，批量插入冲突记录
   cl.insert( [{ a: 1, b: 3 }, { a: 2, b: 3 }], SDB_INSERT_UPDATEONDUP );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 3 }, { a: 2, b: 3 }];
   commCompareResults( actRes, expRes );

   //只指定UpdateOnDup，单条插入冲突记录，返回信息DuplicatedNum与ModifiedNum值不相等
   var actRes1 = cl.insert( [{ a: 1, b: 3 }], { UpdateOnDup: true } ).toObj();
   var expRes1 = { InsertedNum: 0, DuplicatedNum: 1, ModifiedNum: 0 };
   if( !commCompareObject( expRes1, actRes1 ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes1 ) + "\nactual:\n" + JSON.stringify( actRes1 ) );
   } 
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 3 }, { a: 2, b: 3 }];
   commCompareResults( actRes, expRes );

   //只指定UpdateOnDup，批量插入冲突记录
   cl.insert( [{ a: 1, b: 4 }, { a: 2, b: 4 }], { UpdateOnDup: true } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 4 }, { a: 2, b: 4 }];
   commCompareResults( actRes, expRes );
  
   //只指定UpdateOnDup为false，单条插入冲突记录
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 2 }, { UpdateOnDup: false } );
   } );

   //只指定UpdateOnDup，UpdateOnDup为""\null\0\on等无效值，单条插入冲突记录
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 1 }, { UpdateOnDup: "" } );
   } );

   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 1 }, { UpdateOnDup: null } );
   } );

   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 1 }, { UpdateOnDup: 0 } );
   } );

   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 1 }, { UpdateOnDup: "on" } );
   } );
  
   //检查表数据
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 4 }, { a: 2, b: 4 }];
   commCompareResults( actRes, expRes );   
}
