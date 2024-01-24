/************************************************************************
*@Description:  SdbDate, valid range
*testcases:        seqDB-11111、seqDB-11112、seqDB-11113、seqDB-11114
*@Author:  2017/2/28  huangxiaoni init
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11111_1";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   //get local time or millisecond
   var t1 = cmdRun( "date -d '0001-01-01' +%s" ) + "000"; //to milliSeconds
   var t2 = cmdRun( "date -d '1900-01-01' +%s" ) + "000"; //to milliSeconds
   var t3 = cmdRun( "date -d '9999-12-31' +%s" ) + "000";
   var rawData = [{ a: 0, b: SdbDate() },
   { a: 1, b: SdbDate( "1901-01-01" ) },
   { a: 2, b: SdbDate( "9999-12-31" ) },

   { a: 3, b: SdbDate( "0001-01-01T00:00:00.000Z" ) },
   { a: 4, b: SdbDate( "1899-12-31T00:00:00.000Z" ) },
   { a: 5, b: SdbDate( "1900-01-01T00:00:00.000Z" ) },
   { a: 6, b: SdbDate( "9999-12-31T15:59:59.999Z" ) }, //"9999-12-31-23:59:59.999999"

   { a: 7, b: SdbDate( "0001-01-01T00:00:00.000+0800" ) },
   { a: 8, b: SdbDate( "1899-12-31T00:00:00.000+0800" ) },
   { a: 9, b: SdbDate( "1900-01-01T00:00:00.000+0800" ) },
   { a: 10, b: SdbDate( "9999-12-31T23:59:59.999+0800" ) },

   { a: 11, b: SdbDate( "-9223372036854775808" ) },
   { a: 12, b: SdbDate( "9223372036854775807" ) },
   { a: 13, b: SdbDate( t1 ) },
   { a: 14, b: SdbDate( t2 ) },
   { a: 15, b: SdbDate( t3 ) },

   { a: 16, b: SdbDate( -9223372036854775808 ) },
   { a: 17, b: SdbDate( -9223372036854775809 ) },
   { a: 18, b: SdbDate( 9223372036854775807 ) },
   { a: 19, b: SdbDate( 9223372036854775808 ) },
   { a: 20, b: SdbDate( Number( t1 ) ) },
   { a: 21, b: SdbDate( Number( t2 ) ) },
   { a: 22, b: SdbDate( Number( t3 ) ) }];

   //crud
   cl.insert( rawData );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var currentDate = cmdRun( 'date "+%Y-%m-%d"' );
   var expRecs = [eval( '( {a:0, b:{$date:"' + currentDate + '"}} )' ),
   { "a": 1, "b": { "$date": "1901-01-01" } },
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
   { "a": 12, "b": { "$date": -9223372036854776000 } },
   { "a": 13, "b": { "$date": "0001-01-01" } },
   { "a": 14, "b": { "$date": "1900-01-01" } },
   { "a": 15, "b": { "$date": "9999-12-31" } },
   { "a": 16, "b": { "$date": -9223372036854776000 } },
   { "a": 17, "b": { "$date": -9223372036854776000 } },
   { "a": 18, "b": { "$date": -9223372036854776000 } },
   { "a": 19, "b": { "$date": -9223372036854776000 } },
   { "a": 20, "b": { "$date": "0001-01-01" } },
   { "a": 21, "b": { "$date": "1900-01-01" } },
   { "a": 22, "b": { "$date": "9999-12-31" } }];

   //记录插入后rawData中的每条记录均转换成timestamp类型了，需重新定义函数格式的查询条件
   var findConf = [{ a: 0, b: SdbDate() },
   { a: 1, b: SdbDate( "1901-01-01" ) },
   { a: 2, b: SdbDate( "9999-12-31" ) },

   { a: 3, b: SdbDate( "0001-01-01T00:00:00.000Z" ) },
   { a: 4, b: SdbDate( "1899-12-31T00:00:00.000Z" ) },
   { a: 5, b: SdbDate( "1900-01-01T00:00:00.000Z" ) },
   { a: 6, b: SdbDate( "9999-12-31T15:59:59.999Z" ) }, //"9999-12-31-23:59:59.999999"

   { a: 7, b: SdbDate( "0001-01-01T00:00:00.000+0800" ) },
   { a: 8, b: SdbDate( "1899-12-31T00:00:00.000+0800" ) },
   { a: 9, b: SdbDate( "1900-01-01T00:00:00.000+0800" ) },
   { a: 10, b: SdbDate( "9999-12-31T23:59:59.999+0800" ) },

   { a: 11, b: SdbDate( "-9223372036854775808" ) },
   { a: 12, b: SdbDate( "9223372036854775807" ) },
   { a: 13, b: SdbDate( t1 ) },
   { a: 14, b: SdbDate( t2 ) },
   { a: 15, b: SdbDate( t3 ) },

   { a: 16, b: SdbDate( -9223372036854775808 ) },
   { a: 17, b: SdbDate( -9223372036854775809 ) },
   { a: 18, b: SdbDate( 9223372036854775807 ) },
   { a: 19, b: SdbDate( 9223372036854775808 ) },
   { a: 20, b: SdbDate( Number( t1 ) ) },
   { a: 21, b: SdbDate( Number( t2 ) ) },
   { a: 22, b: SdbDate( Number( t3 ) ) }];

   if ( commIsArmArchitecture() == false  )
   {
      var actRecs = cusorToArray( cl, findConf );
      checkRec( actRecs, expRecs );
      checkCount( cl, rawData.length, { b: { $type: 1, $et: 9 } } )
   }
   
   updateDatas( cl, findConf );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   checkUpdateRec( rc, rawData.length - 1, { up: { $type: 1, $et: 9 } } );

   removeDatas( cl, rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}
