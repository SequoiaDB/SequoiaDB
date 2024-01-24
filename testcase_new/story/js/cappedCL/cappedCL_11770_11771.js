/************************************
*@Description: 创建固定集合，插入不同类型、长度的记录
*@author:      luweikang
*@createdate:  2017.7.6
*@testlinkCase:seqDB-11770,seqDB-11771
**************************************/

main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11770_11771";

   //create cappedCL
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insert data
   var doc = [{ No: 1, a: 10 }, { No: 2, a: 50 }, { No: 3, a: -1001 },
   { No: 4, a: { $decimal: "123.456" } }, { No: 5, a: 101.02 },
   { No: 6, a: { $numberLong: "9223372036854775807" } }, { No: 7, a: { $numberLong: "-9223372036854775808" } },
   { No: 8, a: { $date: "2017-05-01" } }, { No: 9, a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { No: 10, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 11, a: { $regex: "^z", $options: "i" } },
   { No: 12, a: null },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: "abc" },
   { No: 15, a: { MinKey: 1 } },
   { No: 16, a: { MaxKey: 1 } },
   { No: 17, a: true }, { No: 18, a: false },
   { No: 19, a: { name: "Jack" } },
   { No: 20, a: [1] },
   { No: 21, a: [3] },
   { No: 22, a: 22 }];
   var longstr = createBigStr();
   dbcl.insert( doc );
   dbcl.insert( { No: 23, a: 23 } );
   dbcl.insert( { No: 24, a: "" } );
   dbcl.insert( { No: 25, a: longstr } );

   //check data
   if( true !== commIsStandalone( db ) )
   {
      checkData( COMMCAPPEDCSNAME, clName );
   }
   else
   {
      standaloneCheckData( dbcl, 25 );
   }

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function standaloneCheckData ( dbcl, count )
{
   var rec = Number( dbcl.count() );
   assert.equal( rec, count );
}

function createBigStr ()
{
   var arr = new Array( 16 * 1024 * 1023 );
   var str = arr.toString();
   return str;
}