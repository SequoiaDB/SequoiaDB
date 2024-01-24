/*******************************************************************************
*@Description : subCL testcase common functions and varialb
*@Modify list :
*              2015-10-10 huangxiaoni
*******************************************************************************/
function dropDM ( domainName, ignoreNotExist, message )
{
   if( ignoreNotExist == undefined ) 
   {
      ignoreNotExist = true;
   }
   if( message == undefined ) 
   {
      message = "";
   }

   try
   {
      var dm = db.getDomain( domainName );
      var rc = dm.listCollectionSpaces();
      while( rc.next() )                                                                                         
      {
         var csInDomain = rc.current().toObj().Name;
         db.dropCS( csInDomain );
      }

      db.dropDomain( domainName );
   }
   catch( e )
   {
      if( e != -214 || !ignoreNotExist )
      {
         throw buildException( "dropDM", e, "[dropDomain:" + domainName + "]",
            "e:-214", "[e:" + e + "]" );
      }
   }
}

/* ****************************************************
@description: get dataRG Info
@parameter:
   [nameStr] "GroupName","HostName","svcname"
@return: groupArray
**************************************************** */
function getDataGroupsName ()
{
   var tmpArray = commGetGroups( db );
   var groupNameArray = new Array;
   for( i = 0; i < tmpArray.length; i++ )
   {
      groupNameArray.push( tmpArray[i][0].GroupName );
   }
   return groupNameArray;
}