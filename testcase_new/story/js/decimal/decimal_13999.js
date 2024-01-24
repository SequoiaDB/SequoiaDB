/******************************************************************************
*@Description : test insert special decimal value to mainCL
*               seqDB-13999:垂直分区表插入特殊decimal值          
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var groups = getGroupName( db );
   if( groups.length < 2 )
   {
      return;
   }

   // test main cl
   var csName = COMMCSNAME + "_maincs13999";
   var mainClName = COMMCLNAME + "_maincl13999";
   var subClName1 = COMMCLNAME + "_subcl1";
   var subClName2 = COMMCLNAME + "_subcl2";
   commDropCL( db, csName, mainClName, true, true, "drop CL in the beginning" );
   commDropCL( db, csName, subClName1, true, true, "drop CL in the beginning" );
   commDropCL( db, csName, subClName2, true, true, "drop CL in the beginning" );

   var option = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" };
   var mainCl = commCreateCL( db, csName, mainClName, option, true, true );
   option = { ShardingKey: { a: 1 }, ShardingType: "range" };
   var subCl1 = commCreateCL( db, csName, subClName1, option, true, true );
   var subCl2 = commCreateCL( db, csName, subClName2, option, true, true );
   var attachOption = { LowBound: { a: { $decimal: "NaN" } }, UpBound: { a: { $decimal: "0" } } };
   mainCl.attachCL( csName + "." + subClName1, attachOption );
   attachOption = { LowBound: { a: { $decimal: "0" } }, UpBound: { a: { $decimal: "MAX" } } };
   mainCl.attachCL( csName + "." + subClName2, attachOption );

   try
   {
      mainCl.insert( { a: { $decimal: "MAX" } } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_CAT_NO_MATCH_CATALOG )
      {
         throw e;
      }
   }

   var docs = [{ a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   mainCl.insert( docs );
   var cursor = subCl1.find().sort( { _id: 1 } );
   commCompareResults( cursor, docs );

   // drop cs
   commDropCS( db, csName, true, "drop CS in the end" );
}
