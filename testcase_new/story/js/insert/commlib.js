/* *****************************************************************************
@discretion: Insert common functions
@modify list:
             2014-3-1 Jianhui Xu  Init
***************************************************************************** */
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

function readyCL ( clName )
{
   commDropCL( db, COMMCSNAME, clName, true, true, "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "Failed to create CL." );
   return cl;
}

function checkRecords ( cl, recs, exceptId )
{
   if( exceptId === undefined ) { exceptId = true; }
   var sel = {};
   if( exceptId === true ) { sel = { _id: { $include: 0 } }; }
   var rc = cl.find( {}, sel ).sort( { a: 1 } );
   commCompareResults( rc, recs, exceptId );
}

function keyConflict ( cl, recsArray )
{
   // key conflict, not set flag
   for( var i = 0; i < recsArray.length; i++ )
   {
      try
      {
         cl.insert( recsArray[i] );
         throw new Error( "need throw error" );
      }
      catch( e )
      {
         if( SDB_IXM_DUP_KEY != e.message )
         {
            throw e;
         }
      }
   }
}

function checkReturnOid ( rc, expOid )
{
   var actOid = rc.toObj()["_id"];
   for( var i = 0; i < actOid.length; i++ )
   {
      if( expOid[i] !== actOid[i] )
      {
         throw new Error( "expOid[i]: " + expOid[i] + "\nactOid[i]: " + actOid[i] );
      }
   }
}

