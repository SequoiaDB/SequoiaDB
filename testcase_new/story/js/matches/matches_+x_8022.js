/************************************************************************
*@Description:   seqDB-8022:使用$+标识符查询，目标字段为非数组，不走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8022";
   var cl = readyCL( clName );

   insertRecs( cl );

   var rc = findRecs( cl );
   checkResult( rc );

   commDropCL( db, COMMCSNAME, clName, false, false );

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

function checkResult ( rc )
{

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }

   var expLen = 0;
   var actLen = findRecsArray.length;
   assert.equal( actLen, expLen );

}