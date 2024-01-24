/************************************************************************
*@Description:   seqDB-8034:使用$elemMatch查询，目标字段为非对象（如string）  
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8034";
   var cl = readyCL( clName );

   insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( { a: [{ b: "test" }] } );
}

function findRecs ( cl )
{

   var rc = cl.find( { a: { $elemMatch: { "": "" } } } ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   //compare records number after find
   var expLen = 0;
   var actLen = findRecsArray.length;
   assert.equal( actLen, expLen );

}