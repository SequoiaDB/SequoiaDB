/*******************************************************************************
*@Description: find操作，匹配符$mod，整型/长整型/浮点型边界值校验_ST.basicOperate.find.mod.001  
*@Author:  2019-6-4  wangkexin
*@testlinkCase: seqDB-5239
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5239";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   var recs = insertData( cl );
   var modValue = -2147483648;
   var rc = cl.find( { key: { "$mod": [modValue, 2] } } );
   var expRecs = [recs[0]];
   checkRec( rc, expRecs );

   var modValue2 = 2147483647;
   rc = cl.find( { key: { "$mod": [modValue2, 3] } } );
   var expRecs2 = [recs[1]];
   checkRec( rc, expRecs2 );

   var modValue3 = { "$numberLong": "-9223372036854775808" };
   rc = cl.find( { key: { "$mod": [modValue3, 0] } } );
   var expRecs3 = [recs[2], recs[4], recs[5]];
   checkRec( rc, expRecs3 );

   var modValue4 = { "$numberLong": "9223372036854775807" };
   rc = cl.find( { key: { "$mod": [modValue4, 0] } } );
   var expRecs4 = [recs[3], recs[4], recs[5]];
   checkRec( rc, expRecs4 );

   var modValue5 = -1.7e+308;
   rc = cl.find( { key: { "$mod": [modValue5, 0] } } );
   var expRecs5 = [recs[4], recs[5]];
   checkRec( rc, expRecs5 );

   var modValue6 = 1.7e+308;
   rc = cl.find( { key: { "$mod": [modValue6, 0] } } );
   var expRecs6 = [recs[4], recs[5]];
   checkRec( rc, expRecs6 );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}

function insertData ( cl )
{
   var dataArray = new Array();
   dataArray.push( { "_id": 0, "key": 4294967298 } );
   dataArray.push( { "_id": 1, "key": 4294967297 } );
   dataArray.push( { "_id": 2, "key": { "$numberLong": "-9223372036854775808" } } );
   dataArray.push( { "_id": 3, "key": { "$numberLong": "9223372036854775807" } } );
   dataArray.push( { "_id": 4, "key": -1.7e+308 } );
   dataArray.push( { "_id": 5, "key": 1.7e+308 } );
   cl.insert( dataArray );
   return dataArray;
}