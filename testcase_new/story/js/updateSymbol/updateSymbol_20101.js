/************************************
*@Description: seqDB-20101:使用$inc更新符指定Value更新字段值，字段值使用对象类型
*@author:      zhaoyu
*@createdate:  2019.10.29
**************************************/

testConf.clName = COMMCLNAME + "_20101";
main( test );

function test ( testPara )
{
   commCreateIndex( testPara.testCL, "a_20101", { a: 1 } );
   var doc = [{ id: 1, a: 1 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, b: 1 }];

   //value值使用int
   var value = 1;
   var expRecs = [{ id: 1, a: 2 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 1, b: 1 }];
   updateAndCheckResult( testPara.testCL, doc, value, expRecs );

   //value值使用double
   value = 100.12;
   expRecs = [{ id: 1, a: 101.12 }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: 100.12, b: 1 }];
   updateAndCheckResult( testPara.testCL, doc, value, expRecs );

   //value值使用numberLong   
   value = { $numberLong: "-9223372036854775808" };
   expRecs = [{ id: 1, a: { $numberLong: "-9223372036854775807" } }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: { $numberLong: "-9223372036854775808" }, b: 1 }]
   updateAndCheckResult( testPara.testCL, doc, value, expRecs );

   //value值使用decimal,SEQUOIADBMAINSTREAM-5130
   value = { $decimal: "9223372036854775808" };
   expRecs = [{ id: 1, a: { $decimal: "9223372036854775809" } }, { id: 2, a: null }, { id: 3, a: "a" }, { id: 4, a: { $decimal: "9223372036854775808" }, b: 1 }];
   updateAndCheckResult( testPara.testCL, doc, value, expRecs );

   //value为null
   invalidDataUpdateCheckResult( testPara.testCL, { $inc: { a: { Value: null } } }, -6 );
}

function updateAndCheckResult ( cl, doc, value, expRecs )
{
   cl.insert( doc );
   cl.update( { $inc: { a: { Value: value } } } );
   checkResult( cl, null, null, expRecs, { id: 1 } );
   cl.remove();
}
