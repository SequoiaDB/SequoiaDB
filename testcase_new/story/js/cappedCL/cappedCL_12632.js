/************************************
*@Description: pop the last record in direction 1 and -1
*@author:      liuxiaoxuan
*@createdate:  2017.8.30
*@testlinkCase: seqDB-12632
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_12632";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   var insertNums = 10;
   insertData( dbcl, insertNums );

   var isSuccess = true;
   var logicalID = getLastLogicalID( dbcl );

   //direction 1 first , and again direction -1 
   checkPopResult( dbcl, logicalID, 1, isSuccess );
   checkPopResult( dbcl, logicalID, -1 );

   //insert records again
   insertData( dbcl, insertNums );
   logicalID = getLastLogicalID( dbcl );

   //direction -1 first , and again direction 1 
   checkPopResult( dbcl, logicalID, -1, isSuccess );
   checkPopResult( dbcl, logicalID, 1 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}


function insertData ( dbcl, insertNums )
{
   var doc = [];
   for( var i = 1; i <= insertNums; i++ )
   {
      doc.push( { a: i } );
   }
   dbcl.insert( doc );
}

function getLastLogicalID ( dbcl )
{
   var logicalID;
   sortOpt = { '_id': 1 };
   var cursor = dbcl.find().sort( sortOpt );
   while( cursor.next() )
   {
      logicalID = cursor.current().toObj()._id;
   }
   return logicalID;
}

function checkPopResult ( dbcl, logicalID, direction, isSuccess )
{
   try
   {
      dbcl.pop( { LogicalID: logicalID, Direction: direction } );
      if( isSuccess == undefined ) throw new Error( "NEED_POP_ERROR" );
   }
   catch( e )
   {
      if( isSuccess == true )
      {
         throw new Error( e );
      }
      else if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}