/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2016-5-16 xiaoni huang
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function readyCL ( clName )
{

   commDropCL( db, COMMCSNAME, clName, true, true );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );
   return cl;
}

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

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

function idxAutoGenData ( cl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   var record = [];
   for( var i = 0; i < insertNum; ++i )
   {
      record.push( {
         "no": i, "no1": i * 2, "no2": i * 3,
         "obj_id": { "$oid": "123abcd00ef12358902300ef" },
         "subobj": { "obj": { "val": "sub" } },
         "string": "西边个喇嘛，东边个哑巴",
         "array": [i + "arr" + i, 5 * i, 2 * i + "ARR" + i, "arrayIndex"], "no3": 4 * i
      } );
   }
   cl.insert( record );
   cnt = 0;
   while( insertNum != cl.count() && cnt < 1000 )
   {
      ++cnt;
      sleep( 2 );
   }
   assert.equal( insertNum, cl.count() );
}

function idxQueryCheck ( cl, queryCond, verifyNum, idxName )
{
   var query = cl.find( queryCond ).explain( { Run: true } ).toArray();
   var queryObj = eval( "(" + query + ")" );
   /*
         if( "tbscan" == queryObj.ScanType )
         {
            throw "ErrorScanType" ;
         }
         if( idxName != queryObj.IndexName )
         {
            throw "ErrorIdxName" ;
         }
   */
   assert.equal( verifyNum, queryObj.ReturnNum );
}

