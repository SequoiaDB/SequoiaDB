import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition )
{
   var rc = dbcl.find( findCondition, findCondition2 ).sort( sortCondition );;
   checkRec( rc, expRecs );
}

/************************************
*@Description: check result when the expect result of find data is failed.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:               
**************************************/
function InvalidArgCheck ( dbcl, condition, condition2, expRecs )
{
   assert.tryThrow( expRecs, function()
   {
      dbcl.find( condition, condition2 ).toArray();
   } );
}
