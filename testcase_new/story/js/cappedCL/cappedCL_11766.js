/************************************
*@Description: 创建多个固定集合空间多个固定集合
*@author:      luweikang
*@createdate:  2017.7.4
*@testlinkCase:seqDB-11766
**************************************/

main( test );
function test ()
{
   var csName1 = COMMCAPPEDCSNAME + "_11766_1";
   var csName2 = COMMCAPPEDCSNAME + "_11766_2";
   var clName = COMMCAPPEDCLNAME + "_11766";

   //clean environment before test
   commDropCS( db, csName1, true, "drop CS in the beginning" );
   commDropCS( db, csName2, true, "drop CS in the beginning" );

   //create cappedCS
   var options1 = { Capped: true };
   var options2 = { Capped: true, PageSize: 4096 };
   commCreateCS( db, csName1, false, "create cappedCS", options1 );
   commCreateCS( db, csName2, false, "create cappedCS", options2 );

   //create normal cappedCL
   for( var i = 0; i < 3; i++ )
   {
      var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
      commCreateCL( db, csName1, clName + i, optionObj, false, false, "create cappedCL" );
   }

   //create replSize cappedCL
   for( var i = 0; i < 9; i++ )
   {
      var replSize = i - 1;
      var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, ReplSize: replSize };
      commCreateCL( db, csName2, clName + i, optionObj, false, false, "create cappedCL" );
   }

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

   for( var i = 0; i < 3; i++ )
   {
      checkInsert( csName1, clName + i, doc );
   }
   for( var i = 0; i < 9; i++ )
   {
      checkInsert( csName2, clName + i, doc );
   }

   //pop data
   var expDoc = [{ No: 11, a: { $regex: "^z", $options: "i" } },
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

   for( var i = 0; i < 3; i++ )
   {
      checkPop( csName1, clName + i, expDoc );
   }
   for( var i = 0; i < 9; i++ )
   {
      checkPop( csName2, clName + i, expDoc );
   }

   //drop cappedCL
   for( var i = 0; i < 3; i++ )
   {
      commDropCL( db, csName1, clName + i, false, false, "drop cappedCL" );
   }
   for( var i = 0; i < 9; i++ )
   {
      commDropCL( db, csName2, clName + i, false, false, "drop cappedCL" );
   }

   //clean environment after test
   commDropCS( db, csName1, true, "drop CS in the end" );
   commDropCS( db, csName2, true, "drop CS in the end" );
}


function checkInsert ( csName, clName, doc )
{
   var dbcl = db.getCS( csName ).getCL( clName );
   dbcl.insert( doc );
   var rc = dbcl.find( null, { No: "", a: "" } );
   checkRec( rc, doc );
   rc.close();
}

function checkPop ( csName, clName, doc )
{
   var dbcl = db.getCS( csName ).getCL( clName );
   var rc = dbcl.find().sort( { "_id": 1 } ).skip( 9 ).limit( 1 );
   var id = rc.next().toObj()._id;
   var options = { 'LogicalID': id, 'Direction': 1 };
   dbcl.pop( options );
   var rc2 = dbcl.find( null, { No: "", a: "" } );
   checkRec( rc2, doc );
   rc.close();
   rc2.close();
}
