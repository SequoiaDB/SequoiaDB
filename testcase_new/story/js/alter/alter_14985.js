/************************************
*@Description: 普通表修改固定集合属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14985
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14985";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create CL in the begin" );

   //alter cl attribute
   clSetAttributes( cl, { Size: 32 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", undefined );

   clSetAttributes( cl, { Max: 1000 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Max", undefined );

   clSetAttributes( cl, { OverWrite: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "OverWrite", undefined );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}