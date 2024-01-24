/************************************
*@Description: 使用date()函数更新记录 
*@author:      wangkexin
*@createdate:  2019.3.5
*@testlinkCase:seqDB-11062
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11062";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   cl.insert( { _id: 1, a: 1 } );

   var sql = ' update ' + csName + '.' + clName + ' set a=date("2016-06-01")';
   db.execUpdate( sql );
   var cursor = cl.find();
   var expRecs = '[{"_id":1,"a":{"$date":"2016-06-01"}}]';
   checkCLData( cursor, expRecs, 1 );

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