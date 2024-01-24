/************************************
*@Description: 混合记录外排功能 
               备节点执行排序查询  
*@author:      liuxiaoxuan
*@createdate:  2018.12.05
*@testlinkCase: seqDB-16724
                seqDB-16730
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_sort_16724_16730";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "M" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );
   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "S" } );
   var dbclSlave = db1.getCS( COMMCSNAME ).getCL( clName );

   // insert small record 
   var rd = new commDataGenerator();
   for( var i = 0; i < 50; i++ )
   {
      var objs = rd.getRecords( 10000, ["string", "string", "string", "string", "string"], ['a', 'b', 'c', 'd', 'e'] );
      dbcl.insert( objs );
   }

   // insert big str, larger than 256M
   objs = new Array();
   for( var i = 0; i < 20; i++ )
   {
      var str = createBigStr( 3 * 1023 );
      objs.push( { a: str, b: str, c: str, d: str, e: str } ); // sort({a:1,b:1,c:1,d:1,e:1}), sortObj > 15M
   }
   dbcl.insert( objs );

   // check result from Master node
   var cursor = dbclPrimary.find().sort( { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   checkSortResultForLargeData( cursor, { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   cursor.close();

   // check result from Slave node
   var cursor = dbclSlave.find().sort( { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   checkSortResultForLargeData( cursor, { a: 1, b: 1, c: 1, d: 1, e: 1 } );
   cursor.close();
   db1.close();

   commDropCL( db, COMMCSNAME, clName, true, true );
}

function createBigStr ( str )
{
   var length = 6 * 1023;
   var arr = "";
   for( var i = 0; i < length; i++ )
   {
      arr = arr + str;
   }
   return arr;
}