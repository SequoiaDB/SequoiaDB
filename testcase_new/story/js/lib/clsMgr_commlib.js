/******************************************************************************
*@Description : Public function for testing attachNode.
*@Modify list :
*               2018-12-12  Wangkexin  Init
******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );
// Get group from Sdb
function getGroup ( db )
{
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
}
