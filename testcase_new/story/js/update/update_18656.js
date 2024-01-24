/******************************************************************************
 * @Description   : seqDB-18656:$set更新字段值相同，类型不同
 * @Author        : Wang Kexin
 * @CreateTime    : 2019.07.08
 * @LastEditTime  : 2021.07.19
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_18656";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   insertRecords( varCL );
   updateRecords( varCL );
   var expRecs = [{ "integer_key": { "$numberLong": "10000000000000000" } }, { "numberLong_key": 20000000000000000 }, { "float_key": { "$decimal": "123.456" } }, { "decimal_key": "789.456" }, { "string_key": null }, { "oid_key": "123abcd00ef12358902300ef" }, { "bool_key": "true" }, { "date_key": { "$timestamp": "2012-01-01-00.00.00.000000" } }, { "timestamp_key": { "$date": "2019-12-31" } }, { "binary_key": { "obj1": "aGVsbG8gd29ybGQ=", "obj2": "1" } }, { "regex_key": { "subobj1": "^张", "subobj2": "i" } }, { "object_key": "value" }, { "array_key": { "subobj1": "abc", "subobj2": 0, "subobj3": "def" } }, { "null_key": "null" }, { "min_key": { "$maxKey": 1 } }, { "max_key": { "$minKey": 1 } }];
   var rc = varCL.find();
   commCompareResults( rc, expRecs );
}

// 插入数据
function insertRecords ( cl )
{
   var records = [{ "integer_key": 10000000000000000 }, { "numberLong_key": { "$numberLong": "20000000000000000" } }, { "float_key": "123.456" }, { "decimal_key": { "$decimal": "789.456" } }, { "string_key": "null" }, { "oid_key": { "$oid": "123abcd00ef12358902300ef" } }, { "bool_key": true }, { "date_key": { "$date": "2012-01-01" } }, { "timestamp_key": { "$timestamp": "2019-12-31-00.00.00.000000" } }, { "binary_key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { "regex_key": { "$regex": "^张", "$options": "i" } }, { "object_key": { "subobj": "value" } }, { "array_key": ["abc", 0, "def"] }, { "null_key": null }, { "min_key": { "$minKey": 1 } }, { "max_key": { "$maxKey": 1 } }];

   cl.insert( records );
}

// 更新数据
function updateRecords ( cl )
{
   cl.update( { "$set": { "integer_key": { "$numberLong": "10000000000000000" } } }, { "integer_key": { $exists: 1 } } );
   cl.update( { "$set": { "numberLong_key": 20000000000000000 } }, { "numberLong_key": { $exists: 1 } } );
   cl.update( { "$set": { "float_key": { "$decimal": "123.456" } } }, { "float_key": { $exists: 1 } } );
   cl.update( { "$set": { "decimal_key": "789.456" } }, { "decimal_key": { $exists: 1 } } );
   cl.update( { "$set": { "string_key": null } }, { "string_key": { $exists: 1 } } );
   cl.update( { "$set": { "oid_key": "123abcd00ef12358902300ef" } }, { "oid_key": { $exists: 1 } } );
   cl.update( { "$set": { "bool_key": "true" } }, { "bool_key": { $exists: 1 } } );
   cl.update( { "$set": { "date_key": { "$timestamp": "2012-01-01-00.00.00.000000" } } }, { "date_key": { $exists: 1 } } );
   cl.update( { "$set": { "timestamp_key": { "$date": "2019-12-31" } } }, { "timestamp_key": { $exists: 1 } } );
   cl.update( { "$set": { "binary_key": { "obj1": "aGVsbG8gd29ybGQ=", "obj2": "1" } } }, { "binary_key": { $exists: 1 } } );
   cl.update( { "$set": { "regex_key": { "subobj1": "^张", "subobj2": "i" } } }, { "regex_key": { $exists: 1 } } );
   cl.update( { "$set": { "object_key": "value" } }, { "object_key": { $exists: 1 } } );
   cl.update( { "$set": { "array_key": { "subobj1": "abc", "subobj2": 0, "subobj3": "def" } } }, { "array_key": { $exists: 1 } } );
   cl.update( { "$set": { "null_key": "null" } }, { "null_key": { $exists: 1 } } );
   cl.update( { "$set": { "min_key": { "$maxKey": 1 } } }, { "min_key": { $exists: 1 } } );
   cl.update( { "$set": { "max_key": { "$minKey": 1 } } }, { "max_key": { $exists: 1 } } );
}