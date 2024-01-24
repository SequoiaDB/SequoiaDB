/************************************
*@Description:  seqDB-20104:使用$inc更新符指定Value、Default、Min、Max值更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/
testConf.clOpt = { StrictDataMode: true };
testConf.clName = COMMCLNAME + "_20104";
main( test );
function test ( testPara )
{
   commCreateIndex( testPara.testCL, "a_20104", { a: 1 } );

   //a字段为数值，Value为数值，Min为数值，Default为数值，a+Value>=Min，更新成功，更新后a值为a+Value
   var doc = [{ id: 1, a: 20 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Min: 50, Default: 100 } } } );
   var expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Default为数值，a+Value<Min，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: 19 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 30, Min: 50, Default: 100 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Default为数值，a+Value<=Max，更新成功，更新后a值为a+Value
   doc = [{ id: 1, a: 20 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Max: 50, Default: 100 } } } );
   expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Default为数值，a+Value>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: 20 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 31, Max: 50, Default: 100 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为null或者其他非数值类型，Value、Default、Min、Max的取值覆盖(a至d)更新成功，更新后a字段值无变化
   doc = [{ id: 1, a: null }, { id: 2, a: "a" }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Min: 50, Default: 100 } } } );
   testPara.testCL.update( { $inc: { a: { Value: 60, Min: 50, Default: 100 } } } );
   testPara.testCL.update( { $inc: { a: { Value: 100, Max: 50, Default: 100 } } } );
   testPara.testCL.update( { $inc: { a: { Value: 30, Max: 50, Default: 100 } } } );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Min为数值，Default为数值，Value+Default>=Min，更新成功，更新后a值为Value+Default
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Min: 50, Default: 20 } } } );
   expRecs = [{ id: 1, a: 50, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Min为数值，Default为数值，Value+Default<Min，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 29, Min: 50, Default: 20 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Max为数值，Default为数值，Value+Default<=Max，更新成功，更新后a值为Value
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Max: 50, Default: 20 } } } );
   expRecs = [{ id: 1, a: 50, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Max为数值，Default为数值，Value+Default>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 31, Max: 50, Default: 20 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Min为数值，Default为null，Value>=Min，更新成功，更新后a不存在
   //a字段不存在时，Value为数值，Min为数值，Default为null，Value<Min，更新成功，更新后a不存在
   //a字段不存在时，Value为数值，Max为数值，Default为null，Value<=Max，更新成功，更新后a不存在
   //a字段不存在时，Value为数值，Max为数值，Default为null，Value>Max，更新成功，更新后a不存在
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 50, Min: 50, Default: null } } } );
   testPara.testCL.update( { $inc: { a: { Value: 51, Min: 50, Default: null } } } );
   testPara.testCL.update( { $inc: { a: { Value: 50, Max: 50, Default: null } } } );
   testPara.testCL.update( { $inc: { a: { Value: 49, Max: 50, Default: null } } } );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();
}

