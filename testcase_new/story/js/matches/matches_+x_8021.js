/************************************************************************
*@Description:   seqDB-8021:使用$+标识符查询，目标字段为数组且数组元素为嵌套对象，走索引查询 
*@Author:  2016/5/23  xiaoni huang
*@bug: -------------jira:1745
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8021";
   var indexName = CHANGEDPREFIX + "_index";
   var cl = readyCL( clName );
   createIndex( cl, indexName );

   var rawData = [{ a: 0, b: [1, 2, { c: [3, 4, 5] }] },
   { a: 1, b: [1, 3, { c: [4, 5] }] },
   { a: 2, b: [4, 2, { c: [2, 1] }] }];
   insertRecs( cl, rawData );

   var cond = { "b.2.c.$1": 5 };
   var findRecsArray = findRecs( cl, cond );
   //checkResult( cl, findRecsArray, rawData, indexName );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, indexName )
{

   cl.createIndex( indexName, { "b.2.c": 1 } );
}

function insertRecs ( cl, rawData )
{
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
}

function findRecs ( cl, cond )
{

   var rc = cl.find( cond ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( cl, findRecsArray, rawData, indexName )
{
   //-------------------check index----------------------------

   //compare scanType
   var idx = cl.find( { "b.2.c.$1": 5 } ).explain().current().toObj();
   if( idx["ScanType"] !== "ixscan" || idx["IndexName"] !== indexName )
   {
      throw new Error( "checkResult fail,[compare index]" +
         "[ScanType:ixscan,IndexName:" + indexName + "]" +
         "[ScanType:" + idx["ScanType"] + ",IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------

   for( i = 0; i < rawData.length - 1; i++ )
   {
      //compare records number after find
      var expLen = 2;
      var actLen = findRecsArray.length;
      assert.equal( actLen, expLen )

      //compare records
      var actB = findRecsArray[i]["b"].toString();
      var expB = rawData[i]["b"].toString();
      assert.equal( actB, expB );
   }
}