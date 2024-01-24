/************************************************************************
*@Description:   seqDB-8096:使用$size查询，目标字段为非数组 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8097";
   var cl = readyCL( clName );

   var rawData = [{ a: "string", b: "test" },
   { a: "subobj", b: { c: { d: "test" } } }];
   insertRecs( cl, rawData );

   var sizeNum = [0, 1];
   var findRecsArray = findRecs( cl, sizeNum );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
}

function findRecs ( cl, sizeNum )
{

   var findRecsArray = [];
   for( i = 0; i < sizeNum.length; i++ )
   {

      var rc = cl.find( { b: { $size: 1, $et: sizeNum[i] } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   var expLen1 = 0;
   var actLen1 = findRecsArray[0].length;
   if( actLen1 !== expLen1 )
   {
      throw new Error( "checkResult fail, [compare number]" +
         "[recsNum:" + expLen1 + "]" +
         "[recsNum:" + actLen1 + "]" );
   }

   var expLen2 = 1;
   var actLen2 = findRecsArray[1].length;
   var expA2 = JSON.stringify( findRecsArray[1] );
   var actA2 = '[{"a":"subobj","b":{"c":{"d":"test"}}}]';
   if( actLen2 !== expLen2 || expA2 !== actA2 )
   {
      throw new Error( "checkResult fail, [compare number]" +
         "[recsNum:" + expLen2 + ", a: " + expA2 + "]" +
         "[recsNum:" + actLen2 + ", a: " + actA2 + "]" );
   }
}