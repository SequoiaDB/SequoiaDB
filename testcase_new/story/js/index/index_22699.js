/*******************************************************************************
@description:  seqDB-22699: listIndexes 带参数、不带参数查询
@author:  2020/08/28  lyy
*******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_22699";
main( test );

function test ( args )
{
   var cl = args.testCL;
   var indexName1 = "index_22699_1";
   var indexName2 = "index_22699_2";
   var indexName3 = "index_22699_3";

   var expValue1 = ["$id"];
   var actValue1 = getIndexName( cl );
   checkResult( expValue1, actValue1 );

   cl.createIndex( indexName1, { a: 1 }, false, false );
   cl.createIndex( indexName2, { b: 1 }, true, false );
   cl.createIndex( indexName3, { c: 1 }, true, true );

   var expValue2 = ["$id", indexName1, indexName2, indexName3];
   var actValue2 = getIndexName( cl );
   checkResult( expValue2, actValue2 );

   cl.dropIndex( indexName2 );

   var expValue3 = ["$id", indexName1, indexName3];
   var actValue3 = getIndexName( cl );
   checkResult( expValue3, actValue3 );
}
function getIndexName ( cl )
{
   var actValue = [];

   var cur = cl.listIndexes();
   var actValueArray = commCursor2Array( cur );
   for( var i = 0; i < actValueArray.length; i++ )
   {
      actValue.push( actValueArray[i].IndexDef.name );
   }

   return actValue;
}

function checkResult ( expValue, actValue )
{
   expValue.sort();
   actValue.sort();
   assert.equal( expValue, actValue );
}
