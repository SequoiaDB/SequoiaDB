/*******************************************************************************
*@Description: find操作，匹配符$gte，日期/时间戳校验_ST.basicOperate.find.gte.002 
*@Author:  2019-5-31  wangkexin
*@testlinkCase: seqDB-5164
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5164";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   var recs = insertData( cl );
   var matchValue = { "$date": "0000-01-01" };
   var rc = cl.find( { key: { "$gte": matchValue } } );
   var expRecs = recs;
   checkRec( rc, expRecs );

   matchValue = { "$date": "9999-12-31" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[1]];
   checkRec( rc, expRecs );

   matchValue = { "$timestamp": "1902-01-01-00.00.00.000000" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[1], recs[2], recs[3]];
   checkRec( rc, expRecs );

   matchValue = { "$timestamp": "2037-12-31-23.59.59.999999" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[1], recs[3]];
   checkRec( rc, expRecs );

   matchValue = { "$timestamp": "2019-12-31-23.59.59.999999" };
   rc = cl.find( { key: { "$gte": matchValue } } );
   expRecs = [recs[1], recs[3]];
   checkRec( rc, expRecs );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}

function insertData ( cl )
{
   var dataArray = new Array();
   //插入数据包括边界值和非边界值
   dataArray.push( { "_id": 0, "key": { "$date": "0000-01-01" } } );
   dataArray.push( { "_id": 1, "key": { "$date": "9999-12-31" } } );
   dataArray.push( { "_id": 2, "key": { "$timestamp": "1902-01-01-00.00.00.000000" } } );
   dataArray.push( { "_id": 3, "key": { "$timestamp": "2037-12-31-23.59.59.999999" } } );
   cl.insert( dataArray );
   return dataArray;
}