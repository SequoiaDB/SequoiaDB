/*******************************************************************************
*@Description: find操作，匹配符$gte，整型/长整型/浮点型/数组型边界值校验_ST.basicOperate.find.gte.001
*@Author:  2019-5-31  wangkexin
*@testlinkCase: seqDB-5163
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5163";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   var recs = insertData( cl );
   var matchValue = -2147483648;
   var rc = cl.find( { key: { "$gte": matchValue } } );
   var expRecs = [recs[0], recs[1], recs[2], recs[4], recs[5], recs[7], recs[8], recs[10]];
   checkRec( rc, expRecs );

   matchValue = 2147483647;
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[1], recs[4], recs[5], recs[7]];
   checkRec( rc, expRecs );

   matchValue = { "$numberLong": "-9223372036854775808" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[0], recs[1], recs[2], recs[3], recs[4], recs[5], recs[7], recs[8], recs[10]];
   checkRec( rc, expRecs );

   matchValue = { "$numberLong": "9223372036854775807" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[4], recs[7]];
   checkRec( rc, expRecs );

   matchValue = -1.7e+308;
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[0], recs[1], recs[2], recs[3], recs[4], recs[5], recs[6], recs[7], recs[8], recs[10]];
   checkRec( rc, expRecs );

   matchValue = 1.7e+308;
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[7]];
   checkRec( rc, expRecs );

   matchValue = [];
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[9], recs[10]];
   checkRec( rc, expRecs );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}

function insertData ( cl )
{
   var dataArray = new Array();
   //插入数据包括边界值和非边界值
   dataArray.push( { "_id": 0, "key": -2147483648 } );
   dataArray.push( { "_id": 1, "key": 2147483647 } );
   dataArray.push( { "_id": 2, "key": 123 } );
   dataArray.push( { "_id": 3, "key": { "$numberLong": "-9223372036854775808" } } );
   dataArray.push( { "_id": 4, "key": { "$numberLong": "9223372036854775807" } } );
   dataArray.push( { "_id": 5, "key": { "$numberLong": "30000000000000000" } } );
   dataArray.push( { "_id": 6, "key": -1.7e+308 } );
   dataArray.push( { "_id": 7, "key": 1.7e+308 } );
   dataArray.push( { "_id": 8, "key": 123.456 } );
   dataArray.push( { "_id": 9, "key": [] } );
   dataArray.push( { "_id": 10, "key": ["abc", 0, "def"] } );
   cl.insert( dataArray );
   return dataArray;
}