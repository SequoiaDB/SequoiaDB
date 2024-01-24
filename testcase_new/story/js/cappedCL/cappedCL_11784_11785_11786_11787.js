/************************************
*@Description: 创建固定集合，正向、逆向pop不同类型、长度的数据
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11784,seqDB-11785,seqDB-11786，seqDB-11787
**************************************/

main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11784_11785_11786_11787";

   //create cappedCL
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insertData
   insertData( COMMCAPPEDCSNAME, clName );

   //pop data Direction:1
   popData( COMMCAPPEDCSNAME, clName, 1, 1 );
   popData( COMMCAPPEDCSNAME, clName, 3, 1 );

   //pop data Direction:-1
   popData( COMMCAPPEDCSNAME, clName, 3, -1 );
   popData( COMMCAPPEDCSNAME, clName, 1, -1 );

   //check node data
   if( true !== commIsStandalone( db ) )
   {
      checkData( COMMCAPPEDCSNAME, clName );
   }
   else
   {
      standaloneCheckData( COMMCAPPEDCSNAME, clName, 0 );
   }

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function insertData ( csName, clName )
{
   var dbcl = db.getCS( csName ).getCL( clName );
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
   dbcl.insert( { No: 23, a: [23, 23.3, "23"] } );
   dbcl.insert( { No: 24, a: "" } );
   dbcl.insert( { No: 25, a: longstr } );

}

function createBigStr ()
{
   var arr = new Array( 16 * 1024 * 1023 );
   var str = arr.toString();
   return str;
}

function popData ( csName, clName, popNum, direction )
{

   var dbcl = db.getCS( csName ).getCL( clName );
   var rc = dbcl.find().sort( { "_id": 1 } ).skip( popNum - 1 ).limit( 1 );
   var id = rc.next().toObj()._id;
   var options = { LogicalID: id, Direction: direction };
   dbcl.pop( options );

}

function standaloneCheckData ( csName, clName, count )
{
   var dbcl = db.getCS( csName ).getCL( clName );
   var rec = Number( dbcl.count() );
   assert.equal( rec, count );
}