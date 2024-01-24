/************************************
*@Description: selector combination
*@author:      zhaoyu
*@createdate:  2016.7.19
*@testlinkCase:seqDB-8237/seqDB-5841/seqDB-8232/seqDB-8233/seqDB-8234
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ a: -11, b: 11.56 },
   { a: 11, b: 11.23 },
   { a: -11.23, b: -11.56, c: -12.56 },
   { a: 11.23, b: -11.23, c: -12.26, d: 100 },
   { c: 12.56, d: -101, e: -1 },
   { c: 12.26, e: 1, f: 1000 },
   { f: -1000, g: 20 },
   { g: -20, h: 30 },
   { h: -30, i: "HelloWorld" },
   { i: "HelloWorld", j: "HelloWorld" },
   { j: "HelloWorld", k: "HelloWorld" },
   { k: "HelloWorld", l: "HelloWorld" },
   { l: "HelloWorld", m: " Hello World " },
   { m: " Hello World ", n: " Hello World " },
   { n: " Hello World ", o: " Hello World " },
   { o: " Hello World ", p: "123" },
   { p: "123", q: { name: "zhangsan", age: 18 } },
   { q: { name: "zhangsan", age: 18 }, r: { name: "zhangsan" } },
   { r: { name: "zhangsan", age: 18 }, s: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }] },
   { t: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }] },
   { u: [1, 2, 3, 4] }];
   dbcl.insert( doc );

   //include all selector;
   var selectCondition1 = [{ a: { $abs: 1 } },
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

   //get new selector combination
   var newCondition1 = getRdmDataFromArr( selectCondition1 );

   //check result
   var expRecs1 = [{ a: 11, b: 12, r: "default" },
   { a: 11, b: 12, r: "default" },
   { a: 11.23, b: -11, c: -13, r: "default" },
   { a: 11.23, b: -11, c: -13, d: 0, r: "default" },
   { c: 12, d: -1, e: 0, r: "default" },
   { c: 12, e: 2, f: 1099, r: "default" },
   { f: -901, g: -400, r: "default" },
   { g: 400, h: 10, r: "default" },
   { h: -10, i: "He", r: "default" },
   { i: "He", j: 10, r: "default" },
   { j: 10, k: "helloworld", r: "default" },
   { k: "helloworld", l: "HELLOWORLD", r: "default" },
   { l: "HELLOWORLD", m: "Hello World ", r: "default" },
   { m: "Hello World ", n: " Hello World", r: "default" },
   { n: " Hello World", o: "Hello World", r: "default" },
   { o: "Hello World", p: 123, r: "default" },
   { p: 123, q: { name: "zhangsan", age: 18 }, r: "default" },
   { q: { name: "zhangsan", age: 18 }, r: { name: "zhangsan" } },
   { r: { name: "zhangsan", age: 18 }, s: [{ name: "zhangsan", age: 18 }, { name: "lisi", age: 18 }] },
   { r: "default", t: [{ name: "zhangsan", age: 18 }] },
   { r: "default", u: [1, 2] }];
   checkResult( dbcl, null, newCondition1, expRecs1, { _id: 1 } );
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