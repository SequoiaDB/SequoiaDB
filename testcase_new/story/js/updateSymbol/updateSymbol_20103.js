/************************************
*@Description: seqDB-20103:使用$inc更新符指定Value、Min、Max值更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/
testConf.clOpt = { StrictDataMode: true };
testConf.clName = COMMCLNAME + "_20103";
main( test );

function test ( testPara )
{
   commCreateIndex( testPara.testCL, "a_20103", { a: 1 } );
   //a字段为数值，Value为数值，Min为数值，Value<Min同时a+Value>=Min，更新成功，更新后a值为a+Value
   var doc = [{ id: 1, a: 20 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 30, Min: 50 } } } );
   var expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Value<Min同时a+Value<Min，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: 19 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 30, Min: 50 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Value>=Min同时a+Value>=Min，更新成功，更新后a值为a+Value
   doc = [{ id: 1, a: 20 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 100, Min: 50 } } } );
   expRecs = [{ id: 1, a: 120 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Value>=Min同时a+Value<Min，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: -31 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 80, Min: 50 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Value>Max同时a+Value<=Max，更新成功，更新后a值为a+Value
   var doc = [{ id: 1, a: -10 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 60, Max: 50 } } } );
   var expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Value>Max同时a+Value>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: -10 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 61, Max: 50 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Value<=Max同时a+Value<=Max，更新成功，更新后a值为a+Value
   doc = [{ id: 1, a: 10 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Max: 50 } } } );
   expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Max为数值，Value<=Max同时a+Value>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: 11 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 40, Max: 50 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Max为数值，Min<a+Value<Max，更新成功，更新后a值为a+Value
   doc = [{ id: 1, a: 10 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Max: 50, Min: 40 } } } );
   expRecs = [{ id: 1, a: 50 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段为数值，Value为数值，Min为数值，Max为数值，a+Value<Min或者a+Value>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, a: 11 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 40, Max: 50, Min: 40 } } }, -318 );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 28, Max: 50, Min: 40 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段为null或者其他非数值类型，更新后a字段值无变化
   doc = [{ id: 1, a: null }, { id: 2, a: "a" }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Max: 50, Min: 40 } } } );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Min为数值，Value>=Min，更新成功，更新后a值为Value
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Min: 40 } } } );
   expRecs = [{ id: 1, a: 40, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Min为数值，Value<Min，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 39, Min: 40 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Max为数值，Value<=Max，更新成功，更新后a值为Value
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Max: 40 } } } );
   expRecs = [{ id: 1, a: 40, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   //a字段不存在时，Value为数值，Max为数值，Value>Max，更新失败，报错信息返回具体失败的记录信息
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 41, Max: 40 } } }, -318 );
   checkResult( testPara.testCL, null, null, doc, { id: 1 } );
   testPara.testCL.remove();

   //Min、Max指定为int、double、numberLong、decimal类型时，更新成功，更新后值正确
   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Min: 39.39, Max: 40.4 } } } );
   expRecs = [{ id: 1, a: 40, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Min: { $numberLong: "-9223372036854775808" }, Max: { $numberLong: "9223372036854775807" } } } } );
   expRecs = [{ id: 1, a: 40, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();

   doc = [{ id: 1, b: 1 }];
   testPara.testCL.insert( doc );
   testPara.testCL.update( { $inc: { a: { Value: 40, Min: { $decimal: "-9223372036854775809" }, Max: { $decimal: "9223372036854775808" } } } } );
   expRecs = [{ id: 1, a: 40, b: 1 }]
   checkResult( testPara.testCL, null, null, expRecs, { id: 1 } );
   testPara.testCL.remove();
}

