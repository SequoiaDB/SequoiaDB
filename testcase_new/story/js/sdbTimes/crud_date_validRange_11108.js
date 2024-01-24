/************************************************************************
*@Description:  date, valid range
*testcases:        seqDB-11108、seqDB-11109、seqDB-11110
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11108_2";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //get local time or millisecond
   var t1 = cmdRun( "date -d '0001-01-01' +%s" ) + "000"; //to milliSeconds
   var t2 = cmdRun( "date -d '1900-01-01' +%s" ) + "000"; //to milliSeconds
   var t3 = cmdRun( "date -d '9999-12-31' +%s" ) + "000";
   var rawData = [{ a: 0, b: { $date: "1900-01-01" } },
   { a: 1, b: { $date: "1970-01-01" } },
   { a: 2, b: { $date: "9999-12-31" } },

   { a: 3, b: { $date: "0001-01-01T00:00:00.000Z" } },
   { a: 4, b: { $date: "1899-12-31T00:00:00.000Z" } },
   { a: 5, b: { $date: "1900-01-01T00:00:00.000Z" } },
   { a: 6, b: { $date: "9999-12-31T15:59:59.999Z" } }, //"9999-12-31-23:59:59.999999"

   { a: 7, b: { $date: "0001-01-01T00:00:00.000+0800" } },
   { a: 8, b: { $date: "1899-12-31T00:00:00.000+0800" } },
   { a: 9, b: { $date: "1900-01-01T00:00:00.000+0800" } },
   { a: 10, b: { $date: "9999-12-31T23:59:59.999+0800" } },

   { a: 11, b: { $date: { $numberLong: "-9223372036854775808" } } },
   { a: 12, b: { $date: { $numberLong: "-9223372036854775809" } } }, //out of range, turn to max boundary
   { a: 13, b: { $date: { $numberLong: "9223372036854775807" } } },
   { a: 14, b: { $date: { $numberLong: "9223372036854775808" } } }, //out of range, turn to min boundary
   { a: 15, b: { $date: { $numberLong: t1 } } },
   { a: 16, b: { $date: { $numberLong: t2 } } },
   { a: 17, b: { $date: { $numberLong: t3 } } },

   { a: 18, b: { $date: "-9223372036854775808" } },
   { a: 19, b: { $date: "-9223372036854775809" } },
   { a: 20, b: { $date: "9223372036854775807" } },
   { a: 21, b: { $date: "9223372036854775808" } },
   { a: 22, b: { $date: t1 } },
   { a: 23, b: { $date: t2 } },
   { a: 24, b: { $date: t3 } },

   { a: 25, b: { $date: -9223372036854775808 } },
   { a: 26, b: { $date: -9223372036854775809 } },
   { a: 27, b: { $date: 9223372036854775807 } },
   { a: 28, b: { $date: 9223372036854775808 } },
   { a: 29, b: { $date: Number( t1 ) } },
   { a: 30, b: { $date: Number( t2 ) } },
   { a: 31, b: { $date: Number( t3 ) } }];

   //crud
   cl.insert( rawData );

   var expRecs = [{ "a": 0, "b": { "$date": "1900-01-01" } },
   { "a": 1, "b": { "$date": "1970-01-01" } },
   { "a": 2, "b": { "$date": "9999-12-31" } },
   { "a": 3, "b": { "$date": "0001-01-01" } },
   { "a": 4, "b": { "$date": "1899-12-31" } },
   { "a": 5, "b": { "$date": "1900-01-01" } },
   { "a": 6, "b": { "$date": "9999-12-31" } },
   { "a": 7, "b": { "$date": "0001-01-01" } },
   { "a": 8, "b": { "$date": "1899-12-31" } },
   { "a": 9, "b": { "$date": "1900-01-01" } },
   { "a": 10, "b": { "$date": "9999-12-31" } },
   { "a": 11, "b": { "$date": -9223372036854776000 } },
   { "a": 12, "b": { "$date": 9223372036854776000 } },
   { "a": 13, "b": { "$date": 9223372036854776000 } },
   { "a": 14, "b": { "$date": -9223372036854776000 } },
   { "a": 15, "b": { "$date": "0001-01-01" } },
   { "a": 16, "b": { "$date": "1900-01-01" } },
   { "a": 17, "b": { "$date": "9999-12-31" } },
   { "a": 18, "b": { "$date": -9223372036854776000 } },
   { "a": 19, "b": { "$date": 9223372036854776000 } },
   { "a": 20, "b": { "$date": 9223372036854776000 } },
   { "a": 21, "b": { "$date": -9223372036854776000 } },
   { "a": 22, "b": { "$date": "0001-01-01" } },
   { "a": 23, "b": { "$date": "1900-01-01" } },
   { "a": 24, "b": { "$date": "9999-12-31" } },
   { "a": 25, "b": { "$date": -9223372036854776000 } },
   { "a": 26, "b": { "$date": -9223372036854776000 } },
   { "a": 27, "b": { "$date": -9223372036854776000 } },
   { "a": 28, "b": { "$date": -9223372036854776000 } },
   { "a": 29, "b": { "$date": "0001-01-01" } },
   { "a": 30, "b": { "$date": "1900-01-01" } },
   { "a": 31, "b": { "$date": "9999-12-31" } }];
   
   if ( commIsArmArchitecture() == false )
   {   
      var actRecs = cusorToArray( cl, rawData );
      checkRec( actRecs, expRecs );
   }
   
   updateDatas( cl, rawData );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   checkUpdateRec( rc );

   removeDatas( cl, rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}
