/*******************************************************************************
*@Description:  find操作，匹配符$elemMatch，日期/时间戳校验_ST.basicOperate.find.element.002
*@Author:  2019-6-4  wangkexin
*@testlinkCase: seqDB-5242
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5242";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   var recs = insertData( cl );
   var matchValue = { "$date": "0000-01-01" };
   var rc = cl.find( { "key": { $elemMatch: { "content": { $elemMatch: { "cont1": { $elemMatch: { "cont2": matchValue } } } } } } } );
   var expRecs = [recs[0]];
   checkRec( rc, expRecs );

   matchValue = { "$date": "9999-12-31" };
   rc = cl.find( { "key": { $elemMatch: { "content": { $elemMatch: { "cont1": { $elemMatch: { "cont2": matchValue } } } } } } } );
   expRecs = [recs[1]];
   checkRec( rc, expRecs );

   matchValue = { "$timestamp": "1902-01-01-00.00.00.000000" };
   rc = cl.find( { "key": { $elemMatch: { "content": { $elemMatch: { "cont1": { $elemMatch: { "cont2": matchValue } } } } } } } );
   expRecs = [recs[2]];
   checkRec( rc, expRecs );

   matchValue = { "$timestamp": "2037-12-31-23.59.59.999999" };
   rc = cl.find( { "key": { $elemMatch: { "content": { $elemMatch: { "cont1": { $elemMatch: { "cont2": matchValue } } } } } } } );
   expRecs = [recs[3]];
   checkRec( rc, expRecs );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}

function insertData ( cl )
{
   var dataArray = new Array();
   dataArray.push( { "_id": 0, "key": { "content": { "cont1": { "cont2": { "$date": "0000-01-01" } } } } } );
   dataArray.push( { "_id": 1, "key": { "content": { "cont1": { "cont2": { "$date": "9999-12-31" } } } } } );
   dataArray.push( { "_id": 2, "key": { "content": { "cont1": { "cont2": { "$timestamp": "1902-01-01-00.00.00.000000" } } } } } );
   dataArray.push( { "_id": 3, "key": { "content": { "cont1": { "cont2": { "$timestamp": "2037-12-31-23.59.59.999999" } } } } } );
   cl.insert( dataArray );
   return dataArray;
}