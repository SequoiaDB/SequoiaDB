/************************************************************************
*@Description:   seqDB-8107:使用$not查询，指定多个不同数据类型的值
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8107";
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
   {
      a: 1, int: 2147483647,
      double: 7,
      null: 1.7E+308,
      string: "hello",
      bool: false,
      subObj: { b: { c: "hello" } },
      array: [2, 3, { c: "hello" }],
      long: { "$numberLong": "9223372036854775807" },
      decimal: { "$decimal": "222.002" },
      oid: { "$regex": "^rg", "$options": "i" },
      regex: { "$oid": "123abcd00ef1235890230099" },
      binary: { "$binary": "aGVsbG8gd27ybGQ=", "$type": "2" },
      date: { "$date": "1999-01-18" },
      timestamp: { "$timestamp": "1999-01-18-23.59.59.999999" },
      tmp1: [3, 4, 99],
      tmp2: [99, { a: 3 }, { b: 4 }],
      str: "test",
      tmp3: 3
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{

   var tmpCond = [{ a: { $ne: 1 } },
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
   { tmp3: { $field: "a" } }];
   var rmNum1 = parseInt( Math.random() * tmpCond.length );
   var rmNum2 = parseInt( Math.random() * tmpCond.length );
   var cond = { $not: [tmpCond[rmNum1], tmpCond[rmNum2]] };
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
   var extRecs = JSON.stringify( rawData[1] );
   assert.equal( actRecs, extRecs );
}