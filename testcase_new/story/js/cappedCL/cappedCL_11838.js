/************************************
*@Description: 创建普通集合，并在该集合上使用pop操作
*@author:      luweikang
*@createdate:  2017.7.10
*@testlinkCase:seqDB-11838
**************************************/

main( test );
function test ()
{
   //create normal CL
   var clName = COMMCLNAME + "_11838";
   dbcl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create normal CL" );

   //insert data
   normalCLinsertData( dbcl );

   //normalCL pop data
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.pop( { LogicalID: 0, Direction: -1 } );
   } );

   //clean environment after test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}

function normalCLinsertData ( dbcl )
{
   var doc = [{ No: 1, a: 10 }, { No: 2, a: 50 }, { No: 3, a: -1001 },
   { No: 4, a: { $decimal: "123.456" } }, { No: 5, a: 101.02 },
   { No: 6, a: { $numberLong: "9223372036854775807" } }, { No: 7, a: { $numberLong: "-9223372036854775808" } },
   { No: 8, a: { $date: "2017-05-01" } }, { No: 9, a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { No: 10, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 11, a: { $regex: "^z", $options: "i" } },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: "abc" },
   { No: 15, a: { MinKey: 1 } },
   { No: 16, a: { MaxKey: 1 } },
   { No: 17, a: true }, { No: 18, a: false },
   { No: 19, a: { name: "Jack" } },
   { No: 20, a: [1] },
   { No: 21, a: [3] },
   { No: 22, a: 22 }];
   dbcl.insert( doc );
}