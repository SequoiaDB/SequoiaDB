/************************************
*@Description: 修改AutoIndexId属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14976
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName1 = CHANGEDPREFIX + "_14976_1";
   var clName2 = CHANGEDPREFIX + "_14976_2";

   var options1 = { AutoIndexId: true };
   var cl1 = commCreateCL( db, csName, clName1, options1, true, false, "create CL in the begin" );

   var options2 = { AutoIndexId: false };
   var cl2 = commCreateCL( db, csName, clName2, options2, true, false, "create CL in the begin" );

   for( i = 0; i < 5000; i++ )
   {
      cl1.insert( { a: i, b: "sequoiadh test split cl alter option" } );
      cl2.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   cl1.setAttributes( { AutoIndexId: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName1, "AttributeDesc", "Compressed | NoIDIndex" );
   try
   {
      cl1.remove();
      throw new Error( "FORBID_REMOVE_ERR" );
   }
   catch( e )
   {
      if( e.message != SDB_RTN_AUTOINDEXID_IS_FALSE )
      {
         throw e;
      }
   }

   cl2.setAttributes( { AutoIndexId: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName2, "AttributeDesc", "Compressed" );
   cl2.remove();

   commDropCL( db, csName, clName1, true, false, "clean cl1" );
   commDropCL( db, csName, clName2, true, false, "clean cl2" );
}