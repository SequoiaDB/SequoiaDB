/******************************************************************************
@Description: Procedure common functions
@modify list:
   2014-3-14 Jianhui Xu  Init
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/******************************************************************************
@Description: clean procedure
@author: Jianhui Xu
@parameter:
   filter : procedure name filter
******************************************************************************/
function fmpCleanProcedures ( db, filter )
{
   if( filter == undefined ) { filter = ""; }

   var procedures = commGetProcedures( db, filter );
   for( var i = 0; i < procedures.length; ++i )
   {
      try
      {
         db.removeProcedure( procedures[i] );
      }
      catch( e )
      {
         if( e.message != SDB_FMP_FUNC_NOT_EXIST )
         {
            throw e;
         }
      }
   }
}

/******************************************************************************
@Description: clean procedure
@author: Jianhui Xu
@parameter:
   nameArray : name array
   ignoreNotExist : true/false, default is false
******************************************************************************/
function fmpRemoveProcedures ( nameArray, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = false; }

   for( var i = 0; i < nameArray.length; ++i )
   {
      try
      {
         db.removeProcedure( nameArray[i] );
      }
      catch( e )
      {
         if( !ignoreNotExist || e.message != SDB_FMP_FUNC_NOT_EXIST )
         {
            throw e;
         }
      }
   }
}

//add by TingYU
function checkResult ( rc, expRsts )
{
   //get actual records to array
   var actRsts = [];
   while( rc.next() )
   {
      actRsts.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRsts.length, expRsts.length );

   //check every records every fields
   for( var i in expRsts )
   {
      var actRec = actRsts[i];
      var expRec = expRsts[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}
