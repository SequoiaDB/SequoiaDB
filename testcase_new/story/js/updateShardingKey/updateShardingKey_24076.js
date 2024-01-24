/******************************************************************************
 * @Description   : seqDB-24076:分区键更新 update 时，set 规则和 where 条件中主键一致允许更新
 * @Author        : chensiqin
 * @CreateTime    : 2021.04.8
 * @LastEditTime  : 2022.06.30
 * @LastEditors   : Yi Pan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_24076";
testConf.clOpt = { ShardingKey: { a: 1, b: -1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ()
{
   var cl = testPara.testCL;

   //insert data 	
   var doc = [{ a: 1, b: 1, c: 1 }, { a: 3, b: 0, c: 5 }];
   cl.insert( doc );

   //3.update指定$set和$replace不包含分区键，$matcher包含分区键
   var setCondition = { $set: { c: 60 } };
   var replaceCondition = { $replace: { c: 70 } };
   var findCondition = { a: 1 };
   var andCondition = { $and: [{ a: 1 }] };
   var expRecs = [{ a: 1, b: 1, c: 60 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, findCondition, false, expRecs );
   expRecs = [{ a: 1, b: 1, c: 70 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, replaceCondition, findCondition, false, expRecs );
   expRecs = [{ a: 1, b: 1, c: 60 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, andCondition, false, expRecs )

   //update指定$set包含分区键, $matcher包含分区键, 且二者值不一致, ModifiedNum为0
   setCondition = { $set: { a: 10 } };
   replaceCondition = { $replace: { a: 20 } };
   findCondition = { a: 1 };
   andCondition = { $and: [{ a: 1 }] };
   expRecs = [{ a: 1, b: 1, c: 60 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, findCondition, false, expRecs );
   updateAndCheckResult( cl, setCondition, andCondition, false, expRecs );
   //update指定$replace包含分区键, $matcher包含分区键,且二者值不一致，ModifiedNum为1
   expRecs = [{ a: 1, b: 1 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, replaceCondition, findCondition, false, expRecs );

   //update指定$set和$replace包含分区键, $matcher包含分区键, 且二者值一致, update成功
   setCondition = { $set: { a: 1, c: 2 } };
   replaceCondition = { $replace: { a: 1, c: 22 } };
   findCondition = { a: 1 };
   andCondition = { $and: [{ a: 1 }] };
   expRecs = [{ a: 1, b: 1, c: 2 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, findCondition, true, expRecs );
   expRecs = [{ a: 1, b: 1, c: 22 }, { a: 3, b: 0, c: 5 }];
   checkError( cl, replaceCondition, findCondition, true );
   expRecs = [{ a: 1, b: 1, c: 2 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, findCondition, true, expRecs );

   //update指定$set和$replace包含分区键, matcher中不包含分区键a, 失败
   setCondition = { $set: { a: 1, c: 3 } };
   replaceCondition = { $replace: { a: 1, c: 33 } };
   findCondition = { b: 1 };
   andCondition = { $and: [{ b: 1 }] };
   checkError( cl, setCondition, findCondition, true );
   checkError( cl, replaceCondition, findCondition, true );
   checkError( cl, setCondition, andCondition, true );

   //update指定$set和$replace包含分区键, matcher部分包含分区键, 失败
   setCondition = { $set: { a: 1, b: 1, c: 6 } };
   replaceCondition = { $replace: { a: 1, b: 1, c: 66 } };
   findCondition = { b: 1 };
   andCondition = { $and: [{ b: 1 }] };
   checkError( cl, setCondition, findCondition, true );
   checkError( cl, replaceCondition, findCondition, true );
   checkError( cl, setCondition, andCondition, true );

   //update指定$set和$replace包含分区键, matcher中包含分区键，等值匹配，更新成功
   setCondition = { $set: { a: 1, b: 1, c: 5 } };
   replaceCondition = { $replace: { a: 1, b: 1, c: 55 } };
   findCondition = { a: 1, b: 1 };
   andCondition = { $and: [{ a: 1 }, { b: 1 }] };
   expRecs = [{ a: 1, b: 1, c: 5 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, findCondition, true, expRecs );
   expRecs = [{ a: 1, b: 1, c: 55 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, replaceCondition, findCondition, true, expRecs );
   expRecs = [{ a: 1, b: 1, c: 5 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, andCondition, true, expRecs );

   //update指定$set和$replace包含分区键, matcher中包含分区键，值不等，更新失败
   setCondition = { $set: { a: 1, b: 1, c: 9 } };
   replaceCondition = { $replace: { a: 1, b: 1, c: 99 } };
   findCondition = { a: { $gt: 0 }, b: 1 };
   andCondition = { $and: [{ a: 2 }, { b: 1 }] };
   checkError( cl, setCondition, findCondition, true );
   checkError( cl, replaceCondition, findCondition, true );
   checkError( cl, setCondition, andCondition, true );

   //update指定$set和$replace包含分区键, macher对象的$and指定条件包含分区键不同值，UpdatedNum和ModifiedNum均为0
   setCondition = { $set: { a: 1, c: 3 } };
   andCondition = { $and: [{ a: 1 }, { b: 1 }, { a: 3 }] };
   expRecs = [{ a: 1, b: 1, c: 5 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, andCondition, true, expRecs );
   setCondition = { $set: { a: 1, c: 3 } };
   andCondition = { $and: [{ a: 3 }, { b: 1 }, { a: 1 }] };
   expRecs = [{ a: 1, b: 1, c: 5 }, { a: 3, b: 0, c: 5 }];
   updateAndCheckResult( cl, setCondition, andCondition, true, expRecs );
}

function updateAndCheckResult ( cl, updateCondition, findCondition, ShardingKeyFlag, expRecs )
{
   var hint = {};

   updateData( cl, updateCondition, findCondition, hint, ShardingKeyFlag );
   var cursor = cl.find().sort( { "a": 1 } );
   commCompareResults( cursor, expRecs );
}

function checkError ( cl, updateCondition, findCondition, ShardingKeyFlag )
{
   var hint = {};

   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      updateData( cl, updateCondition, findCondition, hint, ShardingKeyFlag );
   } );
}
