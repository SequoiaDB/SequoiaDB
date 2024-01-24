/************************************
*@Description: sync()指定CollectionSpace为主表的集合空间
*@author:      wangkexin
*@createDate:  2019.5.20
*@testlinkCase: seqDB-18344
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var maincsName = "maincs18344";
   var subcsName = "subcs18344";
   var mainCL_Name = "maincl18344";
   var subCL_Name = "subcl18344";

   var groups = commGetGroups( db );
   var dataRGName = groups[0][0]["GroupName"];

   //创建主子表
   var mainCLOption = { ShardingKey: { "a": 1 }, ReplSize: 0, IsMainCL: true };
   var maincl = commCreateCL( db, maincsName, mainCL_Name, mainCLOption, true, true );

   var subClOption = { ShardingKey: { "b": 1 }, Group: dataRGName };
   var subcl = commCreateCL( db, subcsName, subCL_Name, subClOption, true, true );

   //attach子表
   var options = { LowBound: { a: 0 }, UpBound: { a: 10 } };
   maincl.attachCL( subcsName + "." + subCL_Name, options );

   //sync()指定CollectionSpace为主表的集合空间
   db.sync( { CollectionSpace: maincsName } );

   //清除环境
   commDropCS( db, maincsName, true, "drop mainCS in the end" );
   commDropCS( db, subcsName, true, "drop subCS in the end" );
}
