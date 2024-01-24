/************************************************************************
*@Description:  timestamp, separator is colon
*testcases:        seqDB-11308
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11308";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   var rawData = [{ a: 0, b: { $timestamp: "1902-01-01-00:00:00.000000" } },
   { a: 1, b: { $timestamp: "1970-01-01-00:00:00.000000" } },
   { a: 2, b: { $timestamp: "2037-12-31-23:59:59.999999" } },

   { a: 3, b: Timestamp( "1902-01-01-00:00:00.000000" ) },
   { a: 4, b: Timestamp( "2037-12-31-23:59:59.999999" ) }]

   cl.insert( rawData );

   var expRecs = [{ "a": 0, "b": { "$timestamp": "1902-01-01-00.00.00.000000" } },
   { "a": 1, "b": { "$timestamp": "1970-01-01-00.00.00.000000" } },
   { "a": 2, "b": { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { "a": 3, "b": { "$timestamp": "1902-01-01-00.00.00.000000" } },
   { "a": 4, "b": { "$timestamp": "2037-12-31-23.59.59.999999" } }];
   var actRecs = cusorToArray( cl, rawData );
   checkRec( actRecs, expRecs );
   checkCount( cl, rawData.length, { b: { $type: 1, $et: 17 } } );

   commDropCL( db, COMMCSNAME, clName );
}
