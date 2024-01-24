/*******************************************************************************
@Description : Create idIndex common functions
@Modify list :
               2016-8-10  wuyan  Init
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue )
{
   if( undefined == cl || undefined == indexName || undefined == indexKey || undefined == keyValue )
   {
      throw new Error( "ErrArg" );
   }
   var getIndex = new Boolean( true );
   try
   {
      getIndex = cl.getIndex( indexName );
   }
   catch( e )
   {
      getIndex = undefined;
   }

   var cnt = 0;
   while( cnt < 20 )
   {
      try
      {
         getIndex = cl.getIndex( indexName );
      }
      catch( e )
      {
         getIndex = undefined;
      }
      if( undefined != getIndex )
      {
         break;
      }
      ++cnt;
   }
   assert.notEqual( undefined, getIndex );

   var indexDef = getIndex.toString();
   indexDef = eval( '(' + indexDef + ')' );
   var index = indexDef["IndexDef"];

   assert.equal( keyValue, index["key"][indexKey] );
   assert.equal( keyValue, index["enforced"] );
   assert.equal( true, index["unique"] );
}

/****************************************************
@description: check the scanType of the explain
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkExplain ( CL, keyValue )
{
   listIndex = CL.find( keyValue ).explain()
   var scanType = listIndex.current().toObj()["ScanType"];
   var expectType = "ixscan";
   assert.equal( expectType, scanType );
}

/****************************************************
@description: check the result of query
@modify list:
              2016-8-10 yan WU init
****************************************************/
function checkCLData ( expRecs, rc )
{

   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   //var expRecs = '[{"a":1},{"a":2}]';
   var actRecs = JSON.stringify( recsArray );
   assert.equal( actRecs, expRecs );
}
