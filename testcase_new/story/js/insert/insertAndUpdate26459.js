/******************************************************************************
 * @Description   : seqDB-26459:insert时指定多个flag参数
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.05.13
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26459";

main( test );

function test ( testPara )
{
   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   //flag指定FLG_INSERT_UPDATEONDUP | FLG_INSERT_CONTONDUP，插入冲突记录
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      cl.insert( { a: 1, b: 2 }, SDB_INSERT_UPDATEONDUP | SDB_INSERT_CONTONDUP );
   } );

   //flag指定FLG_INSERT_UPDATEONDUP | FLG_INSERT_REPLACEONDUP，插入冲突记录
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      cl.insert( { a: 1, b: 2 }, SDB_INSERT_UPDATEONDUP | SDB_INSERT_REPLACEONDUP );
   } );

   //flag指定FLG_INSERT_CONTONDUP | FLG_INSERT_REPLACEONDUP，插入冲突记录
   assert.tryThrow( SDB_INVALIDARG, function ()
   {
      cl.insert( { a: 1, b: 2 }, SDB_INSERT_CONTONDUP | SDB_INSERT_REPLACEONDUP );
   } );

   //检查表数据
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }];
   commCompareResults( actRes, expRes );

   //flag指定FLG_INSERT_UPDATEONDUP | FLG_INSERT_RETURN_ID，单条插入冲突记录，检查_id、ModifiedNum字段值
   cl.insert( { a: 1 }, { UpdateOnDup: true, Update: { "$set": { "_id": "123" } } } );
   actRes = cl.insert( { a: 1, b: 3 }, SDB_INSERT_UPDATEONDUP | SDB_INSERT_RETURN_ID ).toObj();
   expRes = { "_id": "123", "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 };
   //SEQUOIADBMAINSTREAM-7322
   /*if( !commCompareObject( expRes, actRes ) )
   {
      throw new Error( "\nactRes: " + JSON.stringify( actRes ) + "\nexpRes: " + JSON.stringify( expRes ) );
   }*/
 
   //检查表数据
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 3 }];
   commCompareResults( actRes, expRes );
}
