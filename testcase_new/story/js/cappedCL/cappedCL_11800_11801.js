/************************************
*@Description: 创建固定集合空间集合，find查询数据 
*@author:      luweikang
*@createdate:  2017.7.12
*@testlinkCase:seqDB-11800,seqDB-11801
**************************************/

main( test );
function test ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11800_11801_";
   var options = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, options, false, false, "create capped cl" )

   //insert data 
   var doc = [{ a: 10, b: 1, c: "aaa" },
   { a: 100, b: 1, c: "aaa" },
   { a: 1001.02, b: 1 },
   { a: { $decimal: "20170519.09" }, b: 1, c: "aaa" },
   { a: { $numberLong: "9223372036854775807" }, b: 1, c: "aaa" },
   { a: { $date: "2017-05-19" }, b: 1 },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" }, b: 1, c: "aaa" },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: 1, c: "aaa" },
   { a: { $regex: "^z", $options: "i" }, b: 1 },
   { a: null, b: 1, c: "aaa" },
   { a: { $oid: "123abcd00ef12358902300ef" }, b: 1, c: "aaa" },
   { a: "abc", b: 2 },
   { a: { MinKey: 1 }, b: 2, c: "aaa" },
   { a: { MaxKey: 1 }, b: 2, c: "aaa" },
   { a: true, b: 2 },
   { a: false, b: 2, c: "aaa" },
   { a: { name: "Jack" }, b: 2, c: "aaa" },
   { a: [10, 11, 12], b: 2 },
   { a: [102.03, 103.4, 104.5], b: 2, c: "aaa" },
   { a: [1001], b: 2, c: "aaa" },
   { a: ["a", "b", "c"], b: 2 },
   { a: ["z"], b: 2, c: "aaa" },
   { b: 1 }];
   dbcl.insert( doc )
   //check count
   checkCountResult( dbcl, {}, 23 );

   //$gt
   var sortOption = { _id: 1 };
   var gtObj = { a: { $gt: 100 } };
   var results1 = { a: 1001.02, b: 1 };
   checkFindOneResult( dbcl, gtObj, sortOption, results1 );
   checkCountResult( dbcl, gtObj, 5 );

   //$in
   var inObj = { a: { $in: [10, "z"] } };
   var results2 = { a: 10, b: 1, c: "aaa" }
   checkFindOneResult( dbcl, inObj, sortOption, results2 );
   checkCountResult( dbcl, inObj, 3 );

   //$and
   var andObj = { $and: [{ a: 10 }, { b: 1 }] };
   var results3 = { a: 10, b: 1, c: "aaa" };
   checkFindOneResult( dbcl, andObj, sortOption, results3 );
   checkCountResult( dbcl, andObj, 1 );

   //$ne
   var neObj = { b: { $ne: 1 } };
   var results4 = { a: "abc", b: 2 };
   checkFindOneResult( dbcl, neObj, sortOption, results4 );
   checkCountResult( dbcl, neObj, 11 );

   //isNull
   var isnullObj = { c: { $isnull: 1 } };
   var results5 = { a: 1001.02, b: 1 };
   checkFindOneResult( dbcl, isnullObj, sortOption, results5 );
   checkCountResult( dbcl, isnullObj, 8 );

   //$exists
   var existsObj = { c: { $exists: 1 } };
   var results6 = { a: 10, b: 1, c: "aaa" };
   checkFindOneResult( dbcl, existsObj, sortOption, results6 );
   checkCountResult( dbcl, existsObj, 15 );

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function checkCountResult ( dbcl, options, results )
{
   var exc = dbcl.count( options )
   assert.equal( exc, results );
}

function checkFindOneResult ( dbcl, findOption, sortOption, results )
{
   var rc = dbcl.findOne( findOption ).sort( sortOption );

   var obj = rc.current().toObj();
   var id = obj._id;
   assert.notEqual( id, undefined );
   for( var i in obj )
   {
      if( i == "_id" )
      {
         continue;
      }
      assert.equal( obj[i], results[i] );
   }
}