/************************************
*@Description: 创建固定集合空间集合，参数Direction,LogicalID校验
*@author:      luweikang
*@createdate:  2017.12.22
*@testlinkCase:seqDB-11826,seqDB-11827
**************************************/

main( test );
function test ()
{
   //check cl
   var clName = COMMCAPPEDCLNAME + "_11827";
   var options = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var cl = db.getCS( COMMCAPPEDCSNAME ).createCL( clName, options );

   //test record
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

   //get No=10 record ID
   var middleId = getMiddleID( cl, doc, 9 );

   //check Direction is error
   var options = { Directi: -1, LogicalID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 12, "check Direction is error" );

   //check LogicalID is error
   var options = { Direction: -1, LogiID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 22, "check LogicalID is error" );

   //check Direction=-100
   var options = { Direction: -100, LogicalID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 9, "check Direction=-100" );

   //check Direction=100
   var options = { Direction: 100, LogicalID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 12, "check Direction=100" );

   //check Direction="abc"
   var options = { Direction: "abc", LogicalID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 22, "check Direction=abc" );

   //check no Direction,Direction will equals 1
   var options = { LogicalID: middleId };
   checkOption( cl, doc, options );
   checkPopResult( cl, 12, "check no Direction" );

   //check LogicalID=LogicalID+1
   var options = { Direction: -1, LogiID: middleId + 1 };
   checkOption( cl, doc, options );
   checkPopResult( cl, 22, "check LogicalID=LogicalID+1" );

   //check no LogicalID
   var options = { Direction: -1 };
   checkOption( cl, doc, options );
   checkPopResult( cl, 22, "check no LogicalID" );

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function checkOption ( dbcl, doc, options )
{
   try
   {
      dbcl.insert( doc );
      dbcl.pop( options );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

}

function getMiddleID ( dbcl, doc, num )
{
   dbcl.insert( doc );
   var cursor = dbcl.find().sort( { No: 1 } ).skip( num ).limit( 1 );
   var id = cursor.current().toObj()._id;
   dbcl.truncate();
   return id;
}

function checkPopResult ( dbcl, expRecordNum, msg )
{
   var act = dbcl.count();
   assert.equal( act, expRecordNum );
   dbcl.truncate();
}