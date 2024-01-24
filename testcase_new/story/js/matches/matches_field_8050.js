/************************************************************************
*@Description:   seqDB-8050: 使用$field查询，字段不存在 
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8050";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( { a: 0, b: 0 } );
}

function findRecs ( cl )
{

   var rc = cl.find( { "test": { $field: "world" } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray )
{

   var expLen = 0;
   assert.equal( findRecsArray.length, expLen );
}