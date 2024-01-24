/************************************
*@Description: ��ͨ������domain����AutoSplitΪtrue������shardingKey��shardingType
*@author:      wangkexin
*@createdate:  2019.5.24
*@testlinkCase:seqDB-18349
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups to split
   var allGroupName = getGroupName( db, true );
   if( allGroupName.length < 2 )
   {
      return;
   }

   var csName = CHANGEDPREFIX + "_cs_18349";
   var clName1 = CHANGEDPREFIX + "_cl_18349a";
   var clName2 = CHANGEDPREFIX + "_cl_18349b";
   var domainName = "domain18349";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning." );
   commDropDomain( db, domainName );

   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   commCreateDomain( db, domainName, [group1, group2], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } );
   var cl_1 = commCreateCL( db, csName, clName1 );

   var optionObj = { Group: group1 };
   var cl_2 = commCreateCL( db, csName, clName2, optionObj );

   //test a: ִ��setAttributes����shardingKey��shardingType������shardingTypeָ��Ϊhash
   var shardingKey = { a: 1 };
   var shardingType = "hash";
   cl_1.setAttributes( { ShardingKey: shardingKey, ShardingType: shardingType } );
   checkAlterResult( clName1, "ShardingKey", shardingKey, csName );
   checkAlterResult( clName1, "ShardingType", shardingType, csName );

   //test b: ִ��enableSharding�����з֣�����shardingKey��shardingType������shardingTypeָ��Ϊrange
   var shardingKey2 = { a: 1 };
   var shardingType2 = "range";
   cl_2.enableSharding( { ShardingKey: shardingKey2, ShardingType: shardingType2 } );
   checkAlterResult( clName2, "ShardingKey", shardingKey2, csName );
   checkAlterResult( clName2, "ShardingType", shardingType2, csName );

   insertData( cl_1, 5000 );
   insertData( cl_2, 5000 );

   checkSplitResult( csName, clName1, group1, group2, 5000 );
   checkNotSplitResult( csName, clName2, group1, group2, 5000 );

   //clean
   commDropCS( db, csName, true, "clean cs" );
   commDropDomain( db, domainName );
}