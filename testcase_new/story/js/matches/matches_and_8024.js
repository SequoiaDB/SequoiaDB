/******************************************************************************
 * @Description   : seqDB-8024:使用$and查询，不走索引查询
                    seqDB-8102:不同字段字段覆盖所有匹配符组合测试
 * @Author        : xiaoni huang
 * @CreateTime    : 2016.05.24
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : Lai Xuan
 ******************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8024";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   var rawData = [{
      a: 0, int: -2147483648,
      double: -1.7E+308,
      null: null,
      string: "test",
      bool: true,
      subObj: { "0": { c: "test" } },
      array: [2, { c: "test" }],
      long: { "$numberLong": "-9223372036854775808" },
      decimal: { "$decimal": "111.001" },
      oid: { "$oid": "123abcd00ef12358902300ef" },
      regex: { "$regex": "^rg", "$options": "i" },
      binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      date: { "$date": "2038-01-18" },
      timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      tmp1: [1, 2],
      tmp2: [{ a: 1 }, { b: 2 }],
      str: "dhafj",
      tmp3: 0
   },
   { tmp: "hello" },
   { a: 1 },
   { a: 2, int: 2147483647 },
   { a: 3, int: -2147483648, null: 999 },
   { a: 4, int: -2147483648, null: null, bool: false },
   { a: 5, int: -2147483648, null: null, bool: true, string: "hello" },
   { a: 6, int: -2147483648, null: null, bool: true, string: "test", double: 49.06 },
   { a: 7, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, 3] },
   {
      a: 8, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123111111111111111111111" }
   },
   {
      a: 9, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "1999-01-18" }, timestamp: { "$timestamp": "1999-01-18-23.59.59.999999" }
   },
   {
      a: 10, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "2038-01-18" }, timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      long: { "$numberLong": "9223372036854775807" }, decimal: { "$decimal": "222.002" }
   },
   {
      a: 11, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "2038-01-18" }, timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      long: { "$numberLong": "-9223372036854775808" }, decimal: { "$decimal": "111.001" },
      regex: 999
   },
   {
      a: 12, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "2038-01-18" }, timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      long: { "$numberLong": "-9223372036854775808" }, decimal: { "$decimal": "111.001" },
      regex: { "$regex": "^rg", "$options": "i" }, subObj: { "0": { c: "hello" } }
   },
   {
      a: 13, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "2038-01-18" }, timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      long: { "$numberLong": "-9223372036854775808" }, decimal: { "$decimal": "111.001" },
      regex: { "$regex": "^rg", "$options": "i" }, subObj: { "0": { c: "test" } }, str: "test"
   },
   {
      a: 14, int: -2147483648, null: null, bool: true, string: "test", double: 48.00, array: [2, { c: "test" }],
      oid: { "$oid": "123abcd00ef12358902300ef" },
      date: { "$date": "2038-01-18" }, timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" },
      long: { "$numberLong": "-9223372036854775808" }, decimal: { "$decimal": "111.001" },
      regex: { "$regex": "^rg", "$options": "i" }, subObj: { "0": { c: "test" } }, str: "dhafj", tmp3: 1
   }
   ];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{

   var cond = {
      $and: [{ a: { $ne: 1 } },
      { int: { $et: -2147483648 } },
      { null: { $isnull: 1 } },
      { bool: { $in: [true, ""] } },
      { string: { $nin: ["hello", 999] } },
      { double: { $mod: [2, 0] } },
      { array: { $all: [2, { c: "test" }] } },
      { $and: [{ a: { $exists: 1 } }, { oid: { "$oid": "123abcd00ef12358902300ef" } }] },
      { $or: [{ date: { "$date": "2038-01-18" } }, { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }] },
      { $not: [{ long: { "$numberLong": "9223372036854775807" } }, { decimal: { "$decimal": "222.002" } }] },
      { regex: { $type: 1, $et: 11 } },
      { subObj: { $elemMatch: { "0": { c: "test" } } } },
      { str: { $regex: 'dh.*fj', $options: 'i' } },
      { "tmp1.$1": 2 },
      { tmp2: { $size: 1, $et: 2 } },
      { tmp3: { $field: "a" } }]
   };
   var rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   var expLen = 1;
   assert.equal( findRecsArray.length, expLen );
   var actRecs = JSON.stringify( findRecsArray[0] );
   var extRecs = JSON.stringify( rawData[0] );
   assert.equal( actRecs, extRecs );
}