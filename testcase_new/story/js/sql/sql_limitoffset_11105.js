/************************************
*@Description: limit和offset关键字的位置在sql语句中无序 
*@author:      wangkexin
*@createdate:  2019.3.4
*@testlinkCase:seqDB-11105
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11105";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   cl.insert( { _id: 1, a: 0 } );
   cl.insert( { _id: 2, a: 1 } );
   cl.insert( { _id: 3, a: 2 } );
   cl.insert( { _id: 4, a: 3 } );
   cl.insert( { _id: 5, a: 4 } );

   var sql1 = ' select * from ' + csName + '.' + clName + ' offset 1 limit 2';
   var sql2 = ' select * from ' + csName + '.' + clName + ' limit 2 offset 1';
   var cursor1 = db.exec( sql1 );
   var cursor2 = db.exec( sql2 );
   var expRecs = '[{"_id":2,"a":1},{"_id":3,"a":2}]';
   checkCLData( cursor1, expRecs, 2 );
   checkCLData( cursor2, expRecs, 2 );

   commDropCL( db, csName, clName, true, true, "drop CL in the end" );
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