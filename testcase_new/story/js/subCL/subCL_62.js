/************************************
*@Description: attach和detach同一个子表多次 
*@author:      wangkexin
*@createDate:  2019.3.16
*@testlinkCase: seqDB-62
**************************************/
main( test );
function test ()
{
   var csName = "cs62";
   var mainCL_Name = "maincl62";
   var subCL_Name = "subcl62";
   var subCLFullName = csName + "." + subCL_Name;
   var operationNum = 50;

   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //创建主表
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var maincl = commCreateCL( db, csName, mainCL_Name, mainCLOption, true, true );

   //创建子表
   var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
   var subcl = commCreateCL( db, csName, subCL_Name, subClOption, true, true );

   //循环多次attach子表/detach子表 
   var options = { LowBound: { a: 0 }, UpBound: { a: operationNum } };
   for( var i = 0; i < operationNum; i++ )
   {
      maincl.attachCL( subCLFullName, options );
      maincl.insert( { a: i } );
      maincl.detachCL( subCLFullName );
   }

   cursor = subcl.find( {}, { "_id": { "$include": 0 } } );
   var i = 0;
   while( cursor.next() != null )
   {
      var insert_obj = { "a": i };
      var act_obj = cursor.current().toObj();
      assert.equal( insert_obj["a"], act_obj["a"] );
      i++;
   }
   cursor.close();

   //清除环境
   commDropCS( db, csName, true, "drop CS in the end" );
}