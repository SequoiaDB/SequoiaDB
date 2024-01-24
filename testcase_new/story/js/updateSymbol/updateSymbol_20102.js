/************************************
*@Description: seqDB-20102:使用$inc更新符指定Value、Default值更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/

testConf.clOpt = { StrictDataMode: true };
testConf.clName = COMMCLNAME + "_20102";
main( test );

function test ( testPara )
{
   commCreateIndex( testPara.testCL, "a_20102", { a: 1 } );

   //a字段为数值/null/非数值类型/不存在，Value为数值，Default为数值
   var doc = [{ id: 1, a: 1 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4 }];
   var defaultValue = 10;
   var expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 11 }];
   updateAndCheckResult( testPara.testCL, doc, defaultValue, expRecs );

   //a字段为数值/null/非数值类型/不存在，Value为数值，Default为null
   defaultValue = null;
   expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4 }];
   updateAndCheckResult( testPara.testCL, doc, defaultValue, expRecs );

   //default为double
   doc = [{ id: 1 }];
   defaultValue = 100.12;
   expRecs = [{ id: 1, a: 101.12 }];
   updateAndCheckResult( testPara.testCL, doc, defaultValue, expRecs );

   //default为numberLong
   doc = [{ id: 1 }];
   defaultValue = { $numberLong: "-9223372036854775808" };
   expRecs = [{ id: 1, a: { $numberLong: "-9223372036854775807" } }];
   updateAndCheckResult( testPara.testCL, doc, defaultValue, expRecs );

   //default为decimal
   doc = [{ id: 1 }];
   defaultValue = { $decimal: "9223372036854775808" };
   expRecs = [{ id: 1, a: { $decimal: "9223372036854775809" } }];
   updateAndCheckResult( testPara.testCL, doc, defaultValue, expRecs );

   //default为其他类型
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: 1, Default: "a" } } }, -6 );
}

function updateAndCheckResult ( cl, doc, defaultValue, expRecs )
{
   cl.insert( doc );
   cl.update( { $inc: { a: { Value: 1, Default: defaultValue } } } );
   checkResult( cl, null, null, expRecs, { id: 1 } );
   cl.remove();
}