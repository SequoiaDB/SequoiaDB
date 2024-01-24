/************************************
*@Description: 指定domain创建子表，设置分区数小于domain所包含组个数 
*@author:      wangkexin
*@createDate:  2019.3.11
*@testlinkCase: seqDB-15669
**************************************/
main( test );
function test ()
{
   var csName = "cs15669";
   var mainCL_Name = "maincl15669";
   var subCL_Name = "subcl15669";
   var domainName = "domain15669";

   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var dataRGNum = groupsArray.length;
   if( dataRGNum < 3 )
   {
      return;
   }

   var hostNames = [];
   for( var i = 1; i < groupsArray[0].length; i++ )
   {
      hostNames.push( groupsArray[0][i].HostName );
   }

   //新创建6个组
   var newDataRGNames = createDataGroups( db, hostNames, 6 );

   try
   {
      //将新建的6个组和环境中原有的3个（或3个以上）的组名一起存放在totalDataRGNames中
      var totalDataRGArray = commGetGroups( db, false, "", true, true, true );
      var totalDataRGNames = [];
      for( var i = 0; i < totalDataRGArray.length; i++ )
      {
         totalDataRGNames.push( totalDataRGArray[i][0].GroupName );
      }

      commDropDomain( db, domainName );
      commCreateDomain( db, domainName, totalDataRGNames, { AutoSplit: true } );
      commCreateCS( db, csName, true, "", { Domain: domainName } );

      var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
      var dbcl = commCreateCL( db, csName, mainCL_Name, mainCLOption, true, true );

      var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, Partition: 8, ReplSize: 0 };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getCS( csName ).createCL( subCL_Name, subClOption );
      } )
   }
   finally
   {
      //清除环境
      commDropCS( db, csName, true, "drop CS in the end" );
      removeDataRG( db, newDataRGNames );
      commDropDomain( db, domainName );
   }
}
