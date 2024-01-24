/******************************************************************************
@Description : [seqDB-22937][SEQUOIADBMAINSTREAM-6188]
               编目检查递增TaskID, 重启编目组前后进行数据切分
@Athor : Zixian Yan 2020/10/24
******************************************************************************/
testConf.clName = COMMCLNAME + "_22937";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup;
testConf.skipStandAlone;
main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;

   var data = [];
   for( var i = 0; i < 100; i++ )
   {
      data.push( { "a": i } );
   }
   cl.insert( data );

   var cataRG = db.getCataRG();
   var cataMaster = cataRG.getMaster();
   var hostName = cataMaster.getHostName();
   var serviceName = cataMaster.getServiceName();
   var hostAndService = hostName + ":" + serviceName;

   var value_a = 5;
   var value_b = value_a + 5;
   for( var i = 0; i < 3; i++ )
   {
      for( var pos = 0; pos < dstGroupNames.length; pos++ )
      {
         var cursor = new Sdb( hostAndService ).SYSINFO.SYSDCBASE.find();
         var originalTaskID = cursor.current().toObj()["TaskHWM"];

         if( value_b > data.length ) { break; }
         cl.split( srcGroupName, dstGroupNames[pos], { "a": value_a }, { "a": value_b } );

         value_a = value_b + 5;
         value_b = value_a + 5;
         // Restart Catalog Group
         try
         {
            cataRG.stop();
         }
         finally
         {
            cataRG.start();
         }

         // check business
         commCheckBusinessStatus( db, 180 );

         // check results
         cursor = new Sdb( hostAndService ).SYSINFO.SYSDCBASE.find();
         var currentTaskID = cursor.current().toObj()["TaskHWM"];

         var difference = currentTaskID - originalTaskID;
         if( difference != 1 )
         {
            throw new Error( "Failed!!! TaskID has't increase 1 number\nTaskID before split:" + originalTaskID + "\nTaskID after split: " + currentTaskID );
         }

      }
   }

}
