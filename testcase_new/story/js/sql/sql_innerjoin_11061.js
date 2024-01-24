/************************************
*@Description: 多层嵌套inner join查询
*@author:      wangkexin
*@createdate:  2019.3.4
*@testlinkCase:seqDB-11061
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName1 = "11061_bar1";
   var clName2 = "11061_bar2";
   var clName3 = "11061_bar3";

   var cl1 = commCreateCL( db, csName, clName1, {}, true, false, "create cl1 in the begin" );
   var cl2 = commCreateCL( db, csName, clName2, {}, true, false, "create cl2 in the begin" );
   var cl3 = commCreateCL( db, csName, clName3, {}, true, false, "create cl3 in the begin" );

   cl1.insert( { a: 0 } );
   cl1.insert( { a: 1 } );
   cl1.insert( { a: 2 } );
   cl1.insert( { a: 3 } );
   cl1.insert( { a: 4 } );
   commCreateIndex( cl1, "idx_bar1_a", { a: 1 } );

   cl2.insert( { a: 1, b: 1 } );
   cl2.insert( { a: 2, b: 1 } );
   cl2.insert( { a: 3, b: 2 } );
   cl2.insert( { a: 4, b: 2 } );
   cl2.insert( { a: 5, b: 2 } );
   commCreateIndex( cl2, "idx_bar2_b", { b: 1 } );

   cl3.insert( { c: 0 } );
   cl3.insert( { c: 1 } );
   cl3.insert( { c: 2 } );
   cl3.insert( { c: 3 } );
   cl3.insert( { c: 4 } );
   commCreateIndex( cl1, "idx_bar3_c", { c: 1 } );

   var sql = ' select t1.a, t2.b, t2.cnt from ' + csName + '.' + clName1 + ' as t1 inner join ( select t3.b, t3.cnt, t4.c from ( select count(a) as cnt, b from ' + csName + '.' + clName2 + ' group by b ) as t3 inner join ' + csName + '.' + clName3 + ' as t4 on t3.b = t4.c /*+use_hash() use_index(t4, idx_bar3_c)*/ ) as t2 on t1.a = t2.b /*+use_index(t1, idx_bar1_a) */';
   var cursor = db.exec( sql );
   var expRecs = '[{"a":1,"b":1,"cnt":2},{"a":2,"b":2,"cnt":3}]';
   checkCLData( cursor, expRecs, 2 );

   commDropCL( db, csName, clName1, true, true, "drop cl1 in the end" );
   commDropCL( db, csName, clName2, true, true, "drop cl2 in the end" );
   commDropCL( db, csName, clName3, true, true, "drop cl3 in the end" );
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