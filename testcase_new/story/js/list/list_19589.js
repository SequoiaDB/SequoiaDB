/******************************************************************************
*@Description : seqDB-19589:检查list( SDB_LIST_SVCTASKS )列表信息
*               TestLink : seqDB-19589
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groups = commGetGroups( db );
   var nodeName = groups[0][1].HostName + ":" + groups[0][1].svcname;
   var array = db.list( SDB_LIST_SVCTASKS, { "NodeName": nodeName, "TaskID": 0, "TaskName": "Default" } ).toArray();
   if( array.length !== 1 )
   {
      throw new Error( "array.length = " + array.length );
   }
}
