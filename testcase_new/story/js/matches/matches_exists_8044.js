/************************************************************************
*@Description:   seqDB-8044:使用$exists:0查询，不走索引查询 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8044";
   var cl = readyCL( clName );

   insertRecs( cl );
   var rc = findRecs( cl );
   checkResult( rc );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0 },
   { a: 1, b: null },
   { a: 2, b: "" }] );
}

function findRecs ( cl )
{

   var rc = cl.find( { b: { $exists: 0 } } ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc )
{

   var findRtn = new Array();
   while( tmpRecs = rc.next() ) 
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"] !== undefined )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + undefined + "]" +
         "[b:" + findRtn[0]["b"] + "]" );
   }
}