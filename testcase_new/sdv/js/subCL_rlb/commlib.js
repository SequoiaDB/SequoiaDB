/*******************************************************************************
*@Description : subCL testcase common functions and varialb
*@Modify list :
*              2015-10-10 huangxiaoni
*******************************************************************************/

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