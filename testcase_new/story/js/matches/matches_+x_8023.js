/************************************************************************
*@Description:   seqDB-8023:使用$+标识符查询，目标字段为非数组，走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8023";
   var indexName = CHANGEDPREFIX + "_index";
   var cl = readyCL( clName );
   createIndex( cl, indexName );

   insertRecs( cl );

   var rc = findRecs( cl );
   checkResult( rc, indexName );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, indexName )
{

   cl.createIndex( indexName, { a: 1 } );
}

function insertRecs ( cl )
{

   cl.insert( { a: "test" } )
}

function findRecs ( cl )
{

   var rc = cl.find( { "a.$1": "test" } ).sort( { a: 1 } );
   return rc;
}

function checkResult ( rc, indexName )
{
   //-------------------check index----------------------------

   var idx = rc.explain().current().toObj();
   if( idx["ScanType"] !== "ixscan" || idx["IndexName"] !== indexName )
   {
      throw new Error( "checkResult fail,[compare index]" +
         "[ScanType:ixscan,IndexName:" + indexName + "]" +
         "[ScanType:" + idx["ScanType"] + ",IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }

   var expLen = 0;
   var actLen = findRecsArray.length;
   assert.equal( actLen, expLen );

}