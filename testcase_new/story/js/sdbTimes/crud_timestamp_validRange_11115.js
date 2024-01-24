/************************************************************************
*@Description:  timestamp, valid range
*testcases:        seqDB-11115、seqDB-11116、seqDB-11117
*@Author:  2017/2/28  huangxiaoni init
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11115_1";

   var rawData = [{ a: 0, b: { $timestamp: "1902-01-01-00.00.00.000000" } },
   { a: 1, b: { $timestamp: "1970-01-01-00.00.00.000000" } },
   { a: 2, b: { $timestamp: "2037-12-31-23.59.59.999999" } },

   { a: 3, b: { $timestamp: "1901-12-31T15:54:03.000Z" } }, //"1902-01-01T00:00:00.000Z"
   { a: 4, b: { $timestamp: "2037-12-31T15:59:59.999Z" } }, //"2037-12-31-23:59:59.999999"

   { a: 5, b: { $timestamp: "1902-01-01T00:00:00.000+0800" } },
   { a: 6, b: { $timestamp: "2037-12-31T23:59:59.999+0800" } }]

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //crud
   cl.insert( rawData );

   var localtime1 = turnLocaltime( '1901-12-31T15:54:03.000Z', '%Y-%m-%d-%H.%M.%S.000000' );
   var expRecs = [{ "a": 0, "b": { "$timestamp": "1902-01-01-00.00.00.000000" } },
   { "a": 1, "b": { "$timestamp": "1970-01-01-00.00.00.000000" } },
   { "a": 2, "b": { "$timestamp": "2037-12-31-23.59.59.999999" } },
   eval( '( {a:3, b:{$timestamp:"' + localtime1 + '"}} )' ),
   { "a": 4, "b": { "$timestamp": "2037-12-31-23.59.59.999000" } },
   { "a": 6, "b": { "$timestamp": "2037-12-31-23.59.59.999000" } }];
   var actRecs = cusorToArray( cl, rawData );
   checkRec( actRecs, expRecs );
   checkCount( cl, rawData.length, { b: { $type: 1, $et: 17 } } );

   updateDatas( cl, rawData );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   checkUpdateRec( rc, rawData.length, { up: { $type: 1, $et: 17 } } );

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
      //{ a: 5, b: { $timestamp: "1902-01-01T00:00:00.000+0800" } }由于带时区信息，不同的时区展示不同，不对该记录进行比较
      var cursor = cl.find( { $and: [array[i], { a: { $ne: 5 } }] }, { _id: { $include: 0 } } ).sort( { a: 1 } );
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   return rcData
}
