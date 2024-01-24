/******************************************************************************
 * @Description   : seqDB-26461:普通表指定更新规则，单条插入不冲突/冲突记录
 *                  seqDB-26463:普通表指定更新规则，批量插入冲突记录
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.05.17
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26461_26463";

main( test );

function test ( testPara )
{
   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   //指定UpdateOnDup和更新规则，单条插入不冲突记录，查看表数据
   cl.insert( { a: 2, b: 2 }, { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }, { a: 2, b: 2 }];
   commCompareResults( actRes, expRes );

   //指定UpdateOnDup和更新规则，单条插入冲突记录，查看表数据
   cl.insert( { a: 1, b: 3 }, { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [ { a: 2, b: 2 }, { a: 3, b: 1 }];
   commCompareResults( actRes, expRes );

   //指定UpdateOnDup和更新规则，批量插入冲突记录，检查ModifiedNum字段值和表数据
   var actRes1 = cl.insert( [ { a: 2, b: 2 }, { a: 3, b: 1 } ], { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } ).toObj();
   var expRes1 = { "InsertedNum": 0, "DuplicatedNum": 2, "ModifiedNum": 2 };
   if( !commCompareObject( expRes1, actRes1 ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes1 ) + "\nactual:\n" + JSON.stringify( actRes1 ) );
   }
   actRes = cl.find().sort( { a: 1 } );
   expRes = [ { a: 4, b: 2 }, { a: 5, b: 1 } ];
   commCompareResults( actRes, expRes );
}
