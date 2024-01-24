/*******************************************************************************
*@Description : common functions
*@Modify list :
*              2016/7/11 huangxiaoni
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


function createCL ( csName, clName, autoCreateCS, ignoreExisted, message )
{

   if( autoCreateCS == undefined ) { autoCreateCS = true; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }

   //createCS
   if( autoCreateCS )
   {
      commCreateCS( db, csName, true, "Failed to createCS." );
   }

   //createCL
   try
   {
      db.execUpdate( "create collection " + csName + "." + clName );
   }
   catch( e )
   {
      if( e.message != SDB_DMS_EXIST || !ignoreExisted )
      {
         throw e;
      }
   }

   //getCL
   return eval( 'db.' + csName + '.getCL("' + clName + '")' );
}

function dropCL ( csName, clName, ignoreNotExist, message )
{

   if( message == undefined ) { message = ""; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }

   try
   {
      db.execUpdate( "drop collection " + csName + "." + clName );
   }
   catch( e )
   {
      if( ( e.message != SDB_DMS_CS_NOTEXIST && ignoreNotExist ) || ( e.message == SDB_DMS_NOTEXIST && ignoreNotExist ) )
      {
         //continue
      }
      else
      {
         throw e;
      }
   }
}