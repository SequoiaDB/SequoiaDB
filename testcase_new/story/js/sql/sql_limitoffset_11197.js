/************************************
*@Description: limit/offset下压至scan
*@author:      wangkexin
*@createdate:  2019.3.6
*@testlinkCase:seqDB-11197
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName1 = "11197_CL_T1";
   var clName2 = "11197_CL_T2";

   var cl_1 = commCreateCL( db, csName, clName1, {}, true, false, "create cl in the begin" );
   var cl_2 = commCreateCL( db, csName, clName2, {}, true, false, "create cl in the begin" );

   cl_1.insert( { _id: 1, a: 1, b: "test1", c: "c1" } );
   cl_1.insert( { _id: 2, a: 2, b: "test2", c: "c2" } );
   cl_1.insert( { _id: 3, a: 3, b: "test3", c: "c3" } );
   cl_1.insert( { _id: 4, a: 3, b: "test3", c: "c4" } );
   cl_1.insert( { _id: 5, a: 3, b: "test3", c: "c5" } );
   cl_1.insert( { _id: 6, a: 3, b: "test3", c: "c6" } );
   cl_1.insert( { _id: 7, a: 4, b: "test4", c: "c7" } );
   cl_1.insert( { _id: 8, a: 5, b: "test5", c: "c8" } );

   cl_2.insert( { _id: 1, b: 1 } );
   cl_2.insert( { _id: 2, b: 2 } );
   cl_2.insert( { _id: 3, b: 3 } );
   cl_2.insert( { _id: 4, a: 4 } );


   //1.limit下压至scan
   var sql = ' select * from ' + csName + '.' + clName1 + ' limit 1';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":1,"a":1,"b":"test1","c":"c1"}]';
   checkCLData( cursor, expRecs, 1 );

   //2.offset下压至scan
   var sql = ' select * from ' + csName + '.' + clName1 + ' offset 1';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":2,"a":2,"b":"test2","c":"c2"},{"_id":3,"a":3,"b":"test3","c":"c3"},{"_id":4,"a":3,"b":"test3","c":"c4"},{"_id":5,"a":3,"b":"test3","c":"c5"},{"_id":6,"a":3,"b":"test3","c":"c6"},{"_id":7,"a":4,"b":"test4","c":"c7"},{"_id":8,"a":5,"b":"test5","c":"c8"}]';
   checkCLData( cursor, expRecs, 7 );

   //3.limit和offset组合下压至scan
   var sql = ' select * from ' + csName + '.' + clName1 + ' limit 1 offset 5';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":6,"a":3,"b":"test3","c":"c6"}]';
   checkCLData( cursor, expRecs, 1 );

   var sql = ' select * from ' + csName + '.' + clName1 + ' limit 5 offset 1';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":2,"a":2,"b":"test2","c":"c2"},{"_id":3,"a":3,"b":"test3","c":"c3"},{"_id":4,"a":3,"b":"test3","c":"c4"},{"_id":5,"a":3,"b":"test3","c":"c5"},{"_id":6,"a":3,"b":"test3","c":"c6"}]';
   checkCLData( cursor, expRecs, 5 );

   //4.带where条件与limit、offset组合下压至scan
   var sql = ' select * from ' + csName + '.' + clName1 + ' where b="test3" limit 1 offset 2';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":5,"a":3,"b":"test3","c":"c5"}]';
   checkCLData( cursor, expRecs, 1 );

   //5.聚集group "b"y与limit、offset组合不下压至scan
   var sql = ' select count(a) as count from ' + csName + '.' + clName1 + ' group by a limit 1 offset 2';
   var cursor = db.exec( sql );
   var expRecs = '[{"count":4}]';
   checkCLData( cursor, expRecs, 1 );

   //6.排序order by与limit、offset组合下压至scan
   var sql = ' select * from ' + csName + '.' + clName1 + ' order by a desc limit 1 offset 2';
   var cursor = db.exec( sql );
   var expRecs = '[{"_id":3,"a":3,"b":"test3","c":"c3"}]';
   checkCLData( cursor, expRecs, 1 );

   //7.join与limit、offset组合不下压至scan
   var sql = ' select T1.a,T1.b,T1.c from ' + csName + '.' + clName1 + ' as T1 inner join ' + csName + '.' + clName2 + ' as T2 on T1.a=T2.b order by T1.a,T1.b,T1.c limit 2 offset 3';
   var cursor = db.exec( sql );
   var expRecs = '[{"a":3,"b":"test3","c":"c4"},{"a":3,"b":"test3","c":"c5"}]';
   checkCLData( cursor, expRecs, 2 );

   commDropCL( db, csName, clName1, true, true, "drop CL in the end" );
   commDropCL( db, csName, clName2, true, true, "drop CL in the end" );
}

function checkCLData ( rc, expRecs, expCnt )
{
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   rc.close();

   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}