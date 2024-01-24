/************************************************************************
*@Description:   seqDB-8087:使用$isnull:1查询，目标字段存在且为null，走索引查询
                 seqDB-8089:使用$isnull:1查询，目标字段存在且不为null，走索引查询
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8087";
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

   cl.createIndex( indexName, { b: 1 } );
}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0 },
   { a: 1, b: null },
   { a: 2, b: "" }] );
}

function findRecs ( cl )
{

   var rc = cl.find( { b: { $isnull: 1 } } ).sort( { a: 1 } );

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

   var findRtn = new Array();
   while( tmpRecs = rc.next() ) 
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"] !== undefined || findRtn[1]["b"] !== null )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + undefined + ", b:" + null + "]" +
         "[b:" + findRtn[0]["b"] + ", b:" + findRtn[1]["b"] + "]" );
   }
}