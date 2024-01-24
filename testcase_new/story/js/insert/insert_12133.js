/******************************************************************************
 * @Description   : seqDB-12133:插入sequoadb支持所有类型数据
 * @Author        : Wu Yan
 * @CreateTime    : 2017.07.11
 * @LastEditTime  : 2021.07.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12133";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { a: 0, b: "zero" } );
   docs.push( { a: -1, b: "plus one" } );
   docs.push( { a: 2147483647, b: "max_int32" } );
   docs.push( { a: 9223372036854775807, b: "max_int64" } );
   docs.push( { a: 123e+50, b: "float" } );
   docs.push( { a: "value:?*", b: "string" } );
   docs.push( { a: { "$date": "2015-01-29" }, b: "date" } );
   docs.push( { a: { "$timestamp": "2015-01-29-14.30.40.124233" }, b: "timestamp" } );
   docs.push( { a: { "$oid": "123abcd00ef12358902300ef" }, b: "OID" } );
   docs.push( { a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, b: "binary" } );
   docs.push( { a: { "$regex": "HelloWorld", "$options": "i" }, b: "regex" } );
   docs.push( { a: true, b: "booleantrue" } );
   docs.push( { a: false, b: "booleanfalse" } );
   docs.push( { a: { a: 1, b: "one" }, b: "object" } );
   docs.push( { a: ["abc", 0, "def"], b: "array1" } );
   docs.push( { a: [{ a1: "array", b1: [1, 2, 3] }, "type array", 123], b: "array2" } );

   varCL.insert( docs );
   var cursor = varCL.find();
   commCompareResults( cursor, docs );
}
