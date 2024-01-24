/************************************************************************
*@Description:   seqDB-8092:使用$type查询，走索引查询
      base type:    16--int32; 1--double;    10--null; 2--string; 8--bool;   3--subObj; 4--array;
      special type: 18--int64; 100--decimal; 7--oid;   11--regex; 5--binary; 9--date;  17--timestamp;
                    cover all data type
*@Author:  2016/5/21  xiaoni huang
 @Modifier: 2020/08/06 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "_8092";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_8092";

   var indexObj = { "key": 1 };
   //typeNum: 16
   var dataType = ["int32", "int64", "double", "decimal", "string", "oid", "bool", "date",
      "timestamp", "bindata", "regex", "object", "array", "null", "minkey", "maxkey"];
   var rawData = [{ "key": -2147483648 },
   { "key": { "$numberLong": "-9223372036854775808" } },
   { "key": -1.7E+308 },
   { "key": { "$decimal": "111.001" } },
   { "key": "test_mathes_type_8092.js" },
   { "key": { "$oid": "123abcd00ef12358902300ef" } },
   { "key": true },
   { "key": { "$date": "9999-12-31" } },
   { "key": { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { "key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { "key": { "$regex": "^rg", "$options": "i" } },
   { "key": { "subObj": "value" } },
   { "key": ["a", "b", "c"] },
   { "key": null },
   { "key": { "$minKey": 1 } },
   { "key": { "$maxKey": 1 } }];

   cl.insert( rawData );
   cl.createIndex( indexName, indexObj );

   for( var i in rawData )
   {
      var expectation = [rawData[i]];
      var findCondition = { "key": { $type: 2, $et: dataType[i] } };
      var rc = cl.find( findCondition ).sort( indexObj ).hint( { "": indexName } );
      checkRec( rc, expectation );
   }

   var newData = [{ "key": { "$date": "2012-12-21" }, "b": 5 },
   { "key": { "$date": "1997-01-20" }, "b": 7 },
   { "key": { "$date": "1997-01-20" }, "b": 6 },
   { "key": { "$date": "2020-03-31" }, "b": 3 },
   { "key": { "$date": "2008-08-08" }, "b": 9 }];

   var expectation = [{ "key": { "$date": "1997-01-20" }, "b": 6 },
   { "key": { "$date": "1997-01-20" }, "b": 7 },
   { "key": { "$date": "2008-08-08" }, "b": 9 },
   { "key": { "$date": "2012-12-21" }, "b": 5 },
   { "key": { "$date": "2020-03-31" }, "b": 3 },
   { "key": { "$date": "9999-12-31" } }];

   var indexName = "keyIndex";
   var indexObj = { "key": 1, "b": 1 };
   var findCondition = { "key": { "$type": 2, "$et": "date" } };

   cl.insert( newData );
   cl.createIndex( indexName, indexObj );
   var rc = cl.find( findCondition ).sort( indexObj ).hint( { "": indexName } );
   checkRec( rc, expectation );
}
