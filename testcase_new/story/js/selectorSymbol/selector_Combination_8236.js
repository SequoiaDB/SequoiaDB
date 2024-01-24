/************************************
*@Description: selector and mathes combination
*@author:      zhaoyu
*@createdate:  2016.7.19
*@testlinkCase:seqDB-8236
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{
      a: 0,
      int: -2147483648,
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
      tmp3: 0,
      v: -11, b: 11.56, c: -12.56, d: 100, e: 1, f: 1000, g: -20, h: 30,
      i: "HelloWorld", j: "HelloWorld", k: "HelloWorld", l: "HelloWorld", m: " Hello World ", n: " Hello World ", o: " Hello World ",
      p: "123",
      q: { name: "zhangsan", age: 18 },
      r: { name: "zhangsan", age: 18 },
      s: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }],
      t: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }],
      u: [1, 2, 3, 4]
   },
   {
      a: 1,
      int: 2147483647,
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
      tmp3: 3,
      v: -11, b: 11.56, c: -12.56, d: 100, e: 1, f: 1000, g: -20, h: 30,
      i: "HelloWorld", j: "HelloWorld", k: "HelloWorld", l: "HelloWorld", m: " Hello World ", n: " Hello World ", o: " Hello World ",
      p: "123",
      q: { name: "zhangsan", age: 18 },
      r: { name: "zhangsan", age: 18 },
      s: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }],
      t: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }],
      u: [1, 2, 3, 4]
   }];
   dbcl.insert( doc );

   //matches random
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
   var matchesCond = { $and: [tmpCond[rmNum1], tmpCond[rmNum2]] };

   //include all selector;
   var selectCondition1 = [{ v: { $abs: 1 } },
   { b: { $ceiling: 1 } },
   { c: { $floor: 1 } },
   { d: { $mod: 2 } },
   { e: { $add: 1 } },
   { f: { $subtract: -99 } },
   { g: { $multiply: -20 } },
   { h: { $divide: 3 } },
   { i: { $substr: 2 } },
   { j: { $strlen: 1 } },
   { k: { $lower: 1 } },
   { l: { $upper: 1 } },
   { m: { $ltrim: 1 } },
   { n: { $rtrim: 1 } },
   { o: { $trim: 1 } },
   { p: { $cast: "int32" } },
   { q: { $include: 1 } },
   { r: { $default: "default" } },
   { s: { $elemMatch: { age: 18 } } },
   { t: { $elemMatchOne: { age: 18 } } },
   { u: { $slice: 2 } }
   ];

   //get random selector combination
   var newCondition1 = getRdmDataFromArr( selectCondition1 );

   //check result
   var expRecs1 = [{
      v: 11, b: 12, c: -13, d: 0, e: 2, f: 1099, g: 400, h: 10,
      i: "He", j: 10, k: "helloworld", l: "HELLOWORLD", m: "Hello World ", n: " Hello World", o: "Hello World",
      p: 123,
      q: { name: "zhangsan", age: 18 },
      r: { name: "zhangsan", age: 18 },
      s: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }],
      t: [{ name: "zhangsan", age: 18 }],
      u: [1, 2]
   }];
   checkResult( dbcl, matchesCond, newCondition1, expRecs1, { _id: 1 } );
}

/************************************
*@Description: unset the order for arr elements.
*@author:      zhaoyu 
*@createDate:  2016/7/19
*@parameters:               
**************************************/
function getRdmDataFromArr ( arr )
{
   //unset the arr order
   var newArr = arr.sort( function() { return Math.random() - 0.5 } );

   //convert array to object
   var obj = {};
   for( var i in newArr )
   {
      for( var k in newArr[i] )
      {
         obj[k] = newArr[i][k];
      }
   }
   return obj;
}