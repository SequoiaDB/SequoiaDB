/************************************************************************
*@Description:  Timestamp, valid range
*testcases:        seqDB-11118、seqDB-11119、seqDB-11120、seqDB-11121
*@Author:  2017/2/28  huangxiaoni init
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11118_1";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //get local time or millisecond
   var t1 = cmdRun( "date -d '1902-01-01' +%s" );
   var t2 = cmdRun( "date -d '2037-12-31' +%s" );
   var rawData = [{ a: 0, b: Timestamp() },
   { a: 1, b: Timestamp( "1902-01-01-00.00.00.000000" ) },
   { a: 2, b: Timestamp( "2037-12-31-23.59.59.999999" ) },

   { a: 3, b: Timestamp( "1901-12-31T15:54:03.000Z" ) },
   { a: 4, b: Timestamp( "2037-12-31T15:59:59.999Z" ) },

   { a: 5, b: Timestamp( "1902-01-01T00:00:00.000+0800" ) },
   { a: 6, b: Timestamp( "2037-12-31T23:59:59.999+0800" ) },

   { a: 7, b: Timestamp( Number( t1 ), 0 ) },
   { a: 8, b: Timestamp( Number( t2 ) + 86399, 0 ) }, //86399999 = 23:59:59

   { a: 9, b: Timestamp( -2147483648, 0 ) },
   { a: 10, b: Timestamp( 2147483647, 0 ) }];

   //crud
   cl.insert( rawData );

   var localtime1 = turnLocaltime( '1901-12-31T15:54:03.000Z', '%Y-%m-%d-%H.%M.%S.000000' );
   var localtime2 = cmdRun( 'date -d@"' + t1 + '" "+%Y-%m-%d-%H.%M.%S.000000"' );
   var localtime3 = cmdRun( 'date -d@"-2147483648" "+%Y-%m-%d-%H.%M.%S.000000"' );
   var expRecs = [{ "a": 1, "b": { "$timestamp": "1902-01-01-00.00.00.000000" } },
   { "a": 2, "b": { "$timestamp": "2037-12-31-23.59.59.999999" } },
   eval( '( {a:3, b:{$timestamp:"' + localtime1 + '"}} )' ),
   { "a": 4, "b": { "$timestamp": "2037-12-31-23.59.59.999000" } },
   { "a": 6, "b": { "$timestamp": "2037-12-31-23.59.59.999000" } },
   eval( '( {a:7, b:{$timestamp:"' + localtime2 + '"}} )' ),
   { "a": 8, "b": { "$timestamp": "2037-12-31-23.59.59.000000" } },
   eval( '( {a:9, b:{$timestamp:"' + localtime3 + '"}} )' ),
   { "a": 10, "b": { "$timestamp": "2038-01-19-11.14.07.000000" } }];

   //记录插入后rawData中的每条记录均转换成timestamp类型了，需重新定义函数格式的查询条件
   var findConf = [{ a: 0, b: Timestamp() },
   { a: 1, b: Timestamp( "1902-01-01-00.00.00.000000" ) },
   { a: 2, b: Timestamp( "2037-12-31-23.59.59.999999" ) },

   { a: 3, b: Timestamp( "1901-12-31T15:54:03.000Z" ) },
   { a: 4, b: Timestamp( "2037-12-31T15:59:59.999Z" ) },

   { a: 5, b: Timestamp( "1902-01-01T00:00:00.000+0800" ) },
   { a: 6, b: Timestamp( "2037-12-31T23:59:59.999+0800" ) },

   { a: 7, b: Timestamp( Number( t1 ), 0 ) },
   { a: 8, b: Timestamp( Number( t2 ) + 86399, 0 ) }, //86399999 = 23:59:59

   { a: 9, b: Timestamp( -2147483648, 0 ) },
   { a: 10, b: Timestamp( 2147483647, 0 ) }];

   //按照函数格式进行记录匹配，由于{ a: 0, b: Timestamp()}的条件是匹配当前时间，匹配不到记录，因此通过b字段类型匹配记录为timestamp类型
   var actRecs = cusorToArray( cl, findConf );
   checkRec( actRecs, expRecs );
   checkCount( cl, rawData.length, { b: { $type: 1, $et: 17 } } );

   //按照函数格式进行记录匹配，由于{ a: 0, b: Timestamp()}的条件是匹配当前时间，匹配不到记录进行更新，因此不存在up字段
   updateDatas( cl, findConf );
   var rc = cl.find( { a: { $ne: 0 } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
   checkUpdateRec( rc, rawData.length - 1, { up: { $type: 1, $et: 17 } } );

   removeDatas( cl, rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}

/*******************************************************************************
@Description : 从数组中获取元素作为查询条件，将匹配的记录存入array中返回
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function cusorToArray ( cl, array )
{
   var i = 0;
   //get actual records to array
   var rcData = [];
   for( i = 0; i < array.length; i++ )
   {
      //{ a: 5, b: Timestamp( "1902-01-01T00:00:00.000+0800" )}由于带时区信息，不同的时区展示不同，不对该记录进行比较
      var cursor = cl.find( { $and: [array[i], { a: { $ne: 5 } }] }, { _id: { $include: 0 } } ).sort( { a: 1 } );
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   return rcData
}
