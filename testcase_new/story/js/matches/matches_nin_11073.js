/************************************
*@Description: 匹配符$in$nin，插入空数组和正则 
*@author:      wangkexin
*@createdate:  2019.3.5
*@testlinkCase:seqDB-11073
**************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11073";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   cl.insert( { _id: 1, a: [] } );
   cl.insert( { _id: 2, a: Regex( "W", "i" ) } );

   var cursor1 = cl.find( { a: { $in: [Regex( "W", "i" )] } } );
   var expRecs1 = '[{"_id":2,"a":{"$regex":"W","$options":"i"}}]';
   checkCLData( cursor1, expRecs1, 1 );

   var cursor2 = cl.find( { a: { $nin: [Regex( "W", "i" )] } } );
   var expRecs2 = '[{"_id":1,"a":[]}]';
   checkCLData( cursor2, expRecs2, 1 );

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