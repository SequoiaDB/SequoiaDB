/************************************
*@Description: 内排基本功能验证  
*@author:      liuxiaoxuan
*@createdate:  2018.12.04
*@testlinkCase: seqDB-16722
**************************************/
main( test );

function test ()
{
   //create CL
   var clName = COMMCLNAME + "_sort_16722";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // insert
   var rd = new commDataGenerator();
   for( var i = 0; i < 10; i++ )
   {
      var objs = rd.getRecords( 10000, ["string", "string", "string", "string", "string"], ['a', 'b', 'c', 'd', 'e'] );
      dbcl.insert( objs );
   }

   var objs = new Array();
   for( var i = 0; i < 150; i++ )
   {
      var objs = new Array();
      for( var j = 0; j < 10000; j++ )
      {
         objs.push( { a: "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa_16722_" + i, b: "testbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb_" + i, c: "testccccccccccccccccccccccccccccccccccccccccccc_" + i, d: "testdddddddddddddddddddddddddddddddddddddddddddddd" + i, e: "testeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" + i } );
      }
      dbcl.insert( objs );
   }

   // check result
   var cursor = dbcl.find().sort( { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   checkSortResultForLargeData( cursor, { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   cursor.close();

   commDropCL( db, COMMCSNAME, clName, true, true );
}