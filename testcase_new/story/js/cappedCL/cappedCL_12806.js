/************************************
*@Description: 指定中间记录执行先逆向pop后正向pop
*@author:      liuxiaoxuan
*@createdate:  2017.9.30
*@testlinkCase: seqDB-12806
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_12806";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   //insert records 
   var insertNums = 32768;
   insertData( dbcl, insertNums );

   //get one of record in the middle
   var mid_position = 20000;
   var orderby = 1
   var logicalID = getMiddleOneId( dbcl, mid_position, orderby );

   //pop with direction -1 
   var direction = -1;
   popLogicalID( dbcl, logicalID, direction );

   //check count
   var expectCount = 20000;
   checkCount( dbcl, expectCount );

   //insert records again
   insertData( dbcl, insertNums );
   //check count
   expectCount = expectCount + insertNums;
   checkCount( dbcl, expectCount );

   //get one of record in the middle
   mid_position = 25000;
   orderby = -1;
   logicalID = getMiddleOneId( dbcl, mid_position, orderby );

   //pop with direction 1 
   direction = 1;
   popLogicalID( dbcl, logicalID, direction );

   //check count 
   expectCount = 25000;
   checkCount( dbcl, expectCount );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}


function insertData ( dbcl, insertNums )
{
   var str = "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
   var doc = [];
   for( var i = 0; i < 32768; i++ )
   {
      doc.push( { a: i, b: str } );
   }
   dbcl.insert( doc );
}

function getMiddleOneId ( dbcl, position, order )
{
   var logicalID = -1;
   var rec = dbcl.find().sort( { _id: order } ).skip( position ).limit( 1 );
   logicalID = rec.current().toObj()._id;
   return logicalID;
}

function popLogicalID ( dbcl, logicalID, direction )
{
   dbcl.pop( { LogicalID: logicalID, Direction: direction } );
}

function checkCount ( dbcl, expectCount )
{
   var actCount = dbcl.count();
   assert.equal( expectCount, actCount );
}