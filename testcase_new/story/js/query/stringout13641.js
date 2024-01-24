/*******************************************************************************
*@Description:   seqDB-13641: 查询不使用selector，指定flags=FLG_QUERY_STRINGOUT 
*@Author:        2019-2-25  wangkexin
*@Modify list :
*                2020-08-13 wuyan  modify
********************************************************************************/
testConf.clName = COMMCLNAME + "_cl_13641";
main( test );

function test ( testPara )
{
   readyData( testPara.testCL );

   //不使用选择符，排序字段：查询字段  排序顺序：顺序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc1 = testPara.testCL.find().sort( { Key: 1 } ).flags( 1 );
   var expRecs1 = [{ "_id": 15, "Key": { "$minKey": 1 } }, { "_id": 14, "Key": null }, { "_id": 13, "Key": ["abc", 0, "def"] }, { "_id": 1, "Key": 123 }, { "_id": 3, "Key": 123.456 }, { "_id": 4, "Key": { "$decimal": "123.456" } }, { "_id": 2, "Key": 3000000000 }, { "_id": 5, "Key": "value" }, { "_id": 12, "Key": { "subobj": "value" } }, { "_id": 10, "Key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { "_id": 6, "Key": { "$oid": "123abcd00ef12358902300ef" } }, { "_id": 7, "Key": true }, { "_id": 8, "Key": { "$date": "2012-01-01" } }, { "_id": 9, "Key": { "$timestamp": "2012-01-01-13.14.26.124233" } }, { "_id": 11, "Key": { "$regex": "^张", "$options": "i" } }, { "_id": 16, "Key": { "$maxKey": 1 } }];
   checkRec( rc1, expRecs1 );

   //不使用选择符，排序字段：非查询字段  排序顺序：逆序排序  指定flags=FLG_QUERY_STRINGOUT
   var rc2 = testPara.testCL.find().sort( { _id: -1 } ).flags( 1 );
   var expRecs2 = [{ "_id": 16, "Key": { "$maxKey": 1 } }, { "_id": 15, "Key": { "$minKey": 1 } }, { "_id": 14, "Key": null }, { "_id": 13, "Key": ["abc", 0, "def"] }, { "_id": 12, "Key": { "subobj": "value" } }, { "_id": 11, "Key": { "$regex": "^张", "$options": "i" } }, { "_id": 10, "Key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { "_id": 9, "Key": { "$timestamp": "2012-01-01-13.14.26.124233" } }, { "_id": 8, "Key": { "$date": "2012-01-01" } }, { "_id": 7, "Key": true }, { "_id": 6, "Key": { "$oid": "123abcd00ef12358902300ef" } }, { "_id": 5, "Key": "value" }, { "_id": 4, "Key": { "$decimal": "123.456" } }, { "_id": 3, "Key": 123.456 }, { "_id": 2, "Key": 3000000000 }, { "_id": 1, "Key": 123 }];
   checkRec( rc2, expRecs2 );

   //不使用选择符，不使用排序 指定flags=FLG_QUERY_STRINGOUT
   var rc3 = testPara.testCL.find().flags( 1 );
   var expRecs3 = [{ "_id": 1, "Key": 123 }, { "_id": 2, "Key": 3000000000 }, { "_id": 3, "Key": 123.456 }, { "_id": 4, "Key": { "$decimal": "123.456" } }, { "_id": 5, "Key": "value" }, { "_id": 6, "Key": { "$oid": "123abcd00ef12358902300ef" } }, { "_id": 7, "Key": true }, { "_id": 8, "Key": { "$date": "2012-01-01" } }, { "_id": 9, "Key": { "$timestamp": "2012-01-01-13.14.26.124233" } }, { "_id": 10, "Key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { "_id": 11, "Key": { "$regex": "^张", "$options": "i" } }, { "_id": 12, "Key": { "subobj": "value" } }, { "_id": 13, "Key": ["abc", 0, "def"] }, { "_id": 14, "Key": null }, { "_id": 15, "Key": { "$minKey": 1 } }, { "_id": 16, "Key": { "$maxKey": 1 } }];
   checkRec( rc3, expRecs3 );
}