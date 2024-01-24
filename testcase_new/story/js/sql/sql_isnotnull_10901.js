/************************************
*@Description: 执行内置sql，查询条件使用isnot null
*@author:      wangkexin
*@createdate:  2019.3.4
*@testlinkCase:seqDB-10901
**************************************/

main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl10901";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   insertData( cl );

   //在cl使用内置SQL进行select，where子句中的条件为a is not null
   var sql = 'select * from ' + csName + "." + clName + ' where a is not null';
   var cursor = db.exec( sql );
   var expRecs1 = '[{"_id":1,"a":1,"b":1},{"_id":2,"a":1}]';
   checkCLData( cursor, expRecs1, 2 );

   //在cl使用内置SQL进行update，where子句中的条件为a is not null
   var sql = 'update ' + csName + "." + clName + ' set a=123 where a is not null';
   db.execUpdate( sql );
   var cursor = cl.find();
   var expRecs2 = '[{"_id":1,"a":123,"b":1},{"_id":2,"a":123},{"_id":3,"b":1},{"_id":4,"a":null}]';
   checkCLData( cursor, expRecs2, 4 );

   //在cl使用内置SQL进行delete，where子句中的条件为a is not null
   var sql = 'delete from ' + csName + "." + clName + ' where a is not null';
   db.execUpdate( sql );
   var cursor = cl.find();
   var expRecs3 = '[{"_id":3,"b":1},{"_id":4,"a":null}]';
   checkCLData( cursor, expRecs3, 2 );

   commDropCL( db, csName, clName, true, true, "drop CL in the end" );
}

function insertData ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );
   cl.insert( { _id: 2, a: 1 } );
   cl.insert( { _id: 3, b: 1 } );
   cl.insert( { _id: 4, a: null } );
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