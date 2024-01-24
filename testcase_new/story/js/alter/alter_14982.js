/************************************
*@Description: 修改固定集合size
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14982
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "_14982";
   var clName = CHANGEDPREFIX + "_14982";

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clOption = { Capped: true, Size: 96, Max: 100000, AutoIndexId: false, OverWrite: false };
   var cl = commCreateCL( db, csName, clName, clOption, true, true );

   //alter capped cl attribute
   cl.setAttributes( { Size: 32 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", 33554432 );
   checkAlterResult( cl, true );

   cl.setAttributes( { Size: 96 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", 100663296 );
   checkAlterResult( cl, false );

   commDropCS( db, csName, true, "drop cl in the end" );
}

//true is catch -307, false is don't catch any error
function checkAlterResult ( cl, onlyCatch307 )
{
   var str = null;

   for( i = 0; i < 32; i++ )
   {
      var str = str + "dljflksagrrdjsadfsdfasdfdjdwhudw";
   }

   var arr = new Array();
   for( j = 0; j < 100; j++ )
   {
      arr.push( { a: str } );
   }

   for( i = 0; i < 400; i++ )
   {
      try
      {
         cl.insert( arr );
      }
      catch( e )
      {
         if( e.message != SDB_OSS_UP_TO_LIMIT || !onlyCatch307 )
         {
            throw e;
         }
      }

   }
}