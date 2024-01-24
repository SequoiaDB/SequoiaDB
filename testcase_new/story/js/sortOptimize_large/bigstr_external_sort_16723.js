/************************************
*@Description: 大记录外排功能验证   
*@author:      liuxiaoxuan
*@createdate:  2018.12.04
*@testlinkCase: seqDB-16723
**************************************/
main( test );

function test ()
{
   //create CL
   var clName = COMMCLNAME + "_sort_16723";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // insert big str, larger than 256M
   var objs = new Array();
   for( var i = 0; i < 10; i++ )
   {
      var str1 = createBigStr( 6 * 1023 );
      var str2 = createBigStr( 10 * 1023 );
      objs.push( { a: str1, b: str1, c: "testc_" + i } ); // sort({a:1,b:1}), sortObj > 10M
      objs.push( { a: getRandomString( 1024 ), b: getRandomString( 1024 ), c: str2 } ); // sortObj < 10M
   }
   dbcl.insert( objs );

   // check result
   var cursor = dbcl.find().sort( { a: 1, b: 1, c: 1 } );
   checkSortResultForLargeData( cursor, { a: 1, b: 1, c: 1 } );
   cursor.close();

   commDropCL( db, COMMCSNAME, clName, true, true );
}

function createBigStr ( length )
{
   var arr = "";
   var str = getRandomString( 1024 );
   for( var i = 0; i < length; i++ )
   {
      arr = arr + str;
   }
   return arr;
}
