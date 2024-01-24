/************************************
*@Description: 修改固定集合属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14981, seqDB-14983, seqDB-14984
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "_14981";
   var clName = CHANGEDPREFIX + "_14981";

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clOption = { Capped: true, Size: 1024, Max: 100000, AutoIndexId: false, OverWrite: false };
   var cl = commCreateCL( db, csName, clName, clOption, true, true );

   //alter capped cl attribute
   clSetAttributes( cl, { Capped: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "NoIDIndex | Capped" );

   cl.setAttributes( { Size: 32 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", 33554432 );

   cl.setAttributes( { Max: 1000 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Max", 1000 );

   cl.setAttributes( { OverWrite: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "OverWrite", true );

   commDropCS( db, csName, true, "drop cl in the end" );
}