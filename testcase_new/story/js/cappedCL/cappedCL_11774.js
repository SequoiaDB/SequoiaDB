/************************************
*@Description:capped cl, insert with _id and pop datas
*@author:      liuxiaoxuan
*@createdate:  2017.10.09
*@testlinkCase: seqDB-11774
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11774";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   //insert datas , _id locate in the ending
   var data = { a: 1, _id: 0 };
   for( var i = 0; i < 3; i++ )
   {
      dbcl.insert( data );
   }

    //get size of each record
    var recordSize = recordHeader;
    if ( recordSize % 4 != 0 ) 
    {   
        recordSize = recordSize + ( 4 - recordSize % 4 );
    }

   //check find and count
   var expectResult = [{ "_id": 0, "a": 1 },
   { "_id": recordSize, "a": 1 },
   { "_id": recordSize * 2, "a": 1 }];
   var sortConf = { _id: 1 };
   checkRecords( dbcl, null, null, sortConf, null, null, expectResult );

   var expectCount = 3;
   checkCount( dbcl, null, expectCount );

   //pop datas
   var logicalID = 0;
   var direction = -1;
   dbcl.pop( { LogicalID: logicalID, Direction: direction } );

   //insert datas,_id locate in the front/ending
   var doc = [];
   var insertNums = 100;
   for( var i = 0; i < insertNums; i++ ) 
   {
      doc.push( { _id: i, a: i } );
      doc.push( { a: i, _id: i } );
      doc.push( { a: i } );
   }
   dbcl.insert( doc );

   //check count
   expectCount = 300;
   checkCount( dbcl, null, expectCount );
   //check find
   var expectIDs = [];
   for( var i = 0; i < expectCount; i++ )
   {
      expectIDs.push( i * recordSize );
   }
   checkLogicalID( dbcl, null, null, sortConf, null, null, expectIDs );

   //drop cl and createCL again
   commDropCL( db, COMMCAPPEDCSNAME, clName );
   dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   //insert again  
   dbcl.insert( doc );

   //check count and find
   checkCount( dbcl, null, expectCount );
   checkLogicalID( dbcl, null, null, sortConf, null, null, expectIDs );

   //truncate datas 
   dbcl.truncate();

   //insert again
   dbcl.insert( doc );

   //check count and find
   checkCount( dbcl, null, expectCount );
   checkLogicalID( dbcl, null, null, sortConf, null, null, expectIDs );

   //pop datas  
   dbcl.pop( { LogicalID: logicalID, Direction: direction } );

   //check after pop
   expectCount = 0;
   checkCount( dbcl, null, expectCount );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}