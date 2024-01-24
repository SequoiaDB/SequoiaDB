/*******************************************************************************
*@Description:   seqDB-13640:查询使用排序，指定selector为$include和flags=FLG_QUERY_STRINGOUT
*@Author:        2019-2-25  wangkexin
*@Modify list :
*                2020-08-13 wuyan  modify
********************************************************************************/
testConf.clName = COMMCLNAME + "_cl_13640";
main( test );

function test ( testPara )
{
   readyData( testPara.testCL );
   //特殊数据类型如：最小值$minKey、最大值$maxKey、空值null、数组、对象、二进制数据、正则表达式会被忽略，stringout会返回"":""
   //使用$include选择符，排序字段：查询字段  排序顺序：顺序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc1 = testPara.testCL.find( {}, { Key: { $include: 1 } } ).sort( { Key: 1 } ).flags( 1 );
   var expRecs1 = [{ "": "" }, { "": "" }, { "": "" }, { "": "123" }, { "": "123.456" }, { "": "{ \"$decimal\": \"123.456\" }" }, { "": "3000000000" }, { "": "value" }, { "": "" }, { "": "" }, { "": "123abcd00ef12358902300ef" }, { "": "true" }, { "": "2012-01-01" }, { "": "2012-01-01-13.14.26.124233" }, { "": "" }, { "": "" }];
   checkRec( rc1, expRecs1 );

   //使用$include选择符，排序字段：查询字段  排序顺序：逆序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc2 = testPara.testCL.find( {}, { Key: { $include: 1 } } ).sort( { Key: -1 } ).flags( 1 );
   var expRecs2 = [{ "": "" }, { "": "" }, { "": "2012-01-01-13.14.26.124233" }, { "": "2012-01-01" }, { "": "true" }, { "": "123abcd00ef12358902300ef" }, { "": "" }, { "": "" }, { "": "value" }, { "": "" }, { "": "3000000000" }, { "": "123.456" }, { "": "{ \"$decimal\": \"123.456\" }" }, { "": "123" }, { "": "" }, { "": "" }];
   checkRec( rc2, expRecs2 );

   //使用$include选择符，排序字段：非查询字段  排序顺序：顺序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc3 = testPara.testCL.find( {}, { Key: { $include: 1 } } ).sort( { _id: 1 } ).flags( 1 );
   var expRecs3 = [{ "": "123" }, { "": "3000000000" }, { "": "123.456" }, { "": "{ \"$decimal\": \"123.456\" }" }, { "": "value" }, { "": "123abcd00ef12358902300ef" }, { "": "true" }, { "": "2012-01-01" }, { "": "2012-01-01-13.14.26.124233" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }];
   checkRec( rc3, expRecs3 );

   //使用$include选择符，排序字段：非查询字段  排序顺序：逆序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc4 = testPara.testCL.find( {}, { Key: { $include: 1 } } ).sort( { _id: -1 } ).flags( 1 );
   var expRecs4 = [{ "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "" }, { "": "2012-01-01-13.14.26.124233" }, { "": "2012-01-01" }, { "": "true" }, { "": "123abcd00ef12358902300ef" }, { "": "value" }, { "": "{ \"$decimal\": \"123.456\" }" }, { "": "123.456" }, { "": "3000000000" }, { "": "123" }];
   checkRec( rc4, expRecs4 );
}

