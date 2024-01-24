import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function getRandomString ( strLen ) //string length value locate in [minLen, maxLen)
{
   var str = "";
   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

function checkSortResultForLargeData ( cursor, sortCond )
{
   while( cursor.next() )
   {
      var expectResult = cursor.current().toObj(); // the pre one
      if( cursor.next() )
      {
         var actResult = cursor.current().toObj(); // the next one
         for( var sortKey in sortCond )
         {
            if( expectResult[sortKey] > actResult[sortKey] )  // sort result not expected
            {
               throw new Error( "checkSortResultForLargeData check result" +
                  JSON.stringify( expectResult ) + JSON.stringify( actResult ) );
            }
            else if( expectResult[sortKey] < actResult[sortKey] ) // compare next record
            {
               break;
            }
         }
      }
   }
}

function getSortType ( dbcl, findConf, selectorCond, sortCond, hintCond )
{
   if( typeof ( selectorCond ) == "undefined" ) { selectorCond = null; }
   if( typeof ( findCond ) == "undefined" ) { findCond = null; }
   if( typeof ( sortCond ) == "undefined" ) { sortCond = null; }
   if( typeof ( hintCond ) == "undefined" ) { hintCond = null; }

   var sortType = "";
   var explains = dbcl.find( findConf, selectorCond ).sort( sortCond ).hint( hintCond ).explain( { Expand: true, Run: true } );
   var childOperators = explains.next().toObj().PlanPath.ChildOperators;
   for( var i in childOperators )
   {
      sortType = childOperators[i]["PlanPath"]["Estimate"]["SortType"];
   }
   return sortType;
}

function checkSortType ( expectResult, actResult )
{
   assert.equal( expectResult, actResult );
}
