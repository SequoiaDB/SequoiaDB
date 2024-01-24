/************************************
*@Description: ����������domain����AutoSplitΪfalse���޸�shardingKey
*@author:      wangkexin
*@createdate:  2019.5.24
*@testlinkCase:seqDB-18352
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

   var csName = CHANGEDPREFIX + "_cs_18352";
   var clName1 = CHANGEDPREFIX + "_cl_18352a";
   var clName2 = CHANGEDPREFIX + "_cl_18352b";
   var domainName = "domain18352";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning." );
   commDropDomain( db, domainName );

   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   commCreateDomain( db, domainName, [group1, group2], { AutoSplit: false } );
   db.createCS( csName, { Domain: domainName } );

   var subOption1 = { a: 1 };
   var optionObj1 = { ShardingKey: subOption1, Group: group1 };
   var cl_1 = commCreateCL( db, csName, clName1, optionObj1 );

   var subOption2 = { a: 1 };
   var optionObj2 = { ShardingKey: subOption2 };
   var cl_2 = commCreateCL( db, csName, clName2, optionObj2 );

   //test a: �޸�shardingKey���ԣ���alter�޸�shardingKeyΪ{b��1}
   var shardingKey = { b: 1 };
   cl_1.alter( { ShardingKey: shardingKey } );
   checkAlterResult( clName1, "ShardingKey", shardingKey, csName );

   //test b: �޸�shardingKey���ԣ�����autosplitΪtrue
   var shardingKey2 = { b: 1 };
   var autoSplit = true;
   cl_2.alter( { ShardingKey: shardingKey2, AutoSplit: autoSplit } );
   checkAlterResult( clName2, "ShardingKey", shardingKey2, csName );
   checkAlterResult( clName2, "AutoSplit", true, csName );

   insertData( cl_1, 5000 );
   insertData( cl_2, 5000 );

   checkNotSplitResult( csName, clName1, group1, group2, 5000 );
   checkSplitResult( csName, clName2, group1, group2, 5000 );

   //clean
   commDropCS( db, csName, true, "clean cs" );
   commDropDomain( db, domainName );
}