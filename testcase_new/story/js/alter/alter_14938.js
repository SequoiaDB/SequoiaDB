/************************************
*@Description: 主表修改ShardingKey字段
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14938
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainName = CHANGEDPREFIX + "_14938_main";
   var clName = CHANGEDPREFIX + "_14938_attach";

   var options = { IsMainCL: true, ShardingType: 'range', ShardingKey: { a: 1 } };
   var mainCL = commCreateCL( db, csName, mainName, options, true, false, "create mainCL in the begin" );
   commCreateCL( db, csName, clName, {}, true, false, "create cl1 in the begin" );

   //主表未挂载子表
   var alterOption1 = { ShardingKey: { b: 1 } };
   mainCL.alter( alterOption1 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, mainName, "ShardingKey", { b: 1 } );

   //主表attach子表
   mainCL.attachCL( csName + "." + clName, { LowBound: { b: 0 }, UpBound: { b: 100 } } );
   clSetAttributes( mainCL, { ShardingKey: { a: -1 } } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, mainName, "ShardingKey", { b: 1 } );

   //主表detach子表
   mainCL.detachCL( csName + "." + clName );
   var alterOption3 = { ShardingKey: { b: -1 } };
   mainCL.alter( alterOption3 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, mainName, "ShardingKey", { b: -1 } );

   //clean in the end
   commDropCL( db, csName, mainName, true, false, "clean main cl" );
   commDropCL( db, csName, clName, true, false, "clean cl" );
}