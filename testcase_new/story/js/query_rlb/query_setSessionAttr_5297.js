/************************************
*@Description: setSessionAttr，字段值取边界值_ST.basicOperate.setSessionAttr.001
*@author:      wangkexin
*@createDate:  2019.6.5
*@testlinkCase: seqDB-5297
**************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, true );

   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5297";
   var rgName = "datagroup5297";
   var dataNum = 100;
   var instanceidArr = [1, 255];
   var hostName = groupsArray[0][1].HostName;
   var logSourcePaths = [];

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );
   removeDataRG( rgName );

   try
   {
      var nodes = createDataGroups( rgName, hostName, instanceidArr, logSourcePaths );
      var optionObj = { Group: rgName, ReplSize: 0 };
      var cl = commCreateCL( db, csName, clName, optionObj, true, false );

      insertData( cl, dataNum );
      //PreferedInstance字段值分别取1、7、M
      db.setSessionAttr( { "PreferedInstance": 1 } );
      var queryNode = cl.find().explain( { Run: true } ).current().toObj().NodeName;
      checkNodeByInstanceId( queryNode, nodes, 1 );

      db.setSessionAttr( { "PreferedInstance": 255 } );
      var queryNode2 = cl.find().explain( { Run: true } ).current().toObj().NodeName;
      checkNodeByInstanceId( queryNode2, nodes, 255 );

      db.setSessionAttr( { "PreferedInstance": "M" } );
      var queryNode3 = cl.find().explain( { Run: true } ).current().toObj().NodeName;
      checkRole( queryNode3, rgName, true );
   }
   catch( e )
   {
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/5297";
      File.mkdir( backupDir );
      for( var i = 0; i < logSourcePaths.length; i++ )
      {
         File.scp( logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      //清理环境
      commDropCL( db, csName, clName, true, true, "drop CL in the end." );
      removeDataRG( rgName );
   }
}