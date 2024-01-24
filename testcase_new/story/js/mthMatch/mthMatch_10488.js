/************************************
*@Description: main-sub cl use type
*@author:      zhaoyu
*@createdate:  2016.11.1
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl10488";
   subCL_Name1 = CHANGEDPREFIX + "_subcl104881";
   subCL_Name2 = CHANGEDPREFIX + "_subcl104882";
   subCL_Name3 = CHANGEDPREFIX + "_subcl104883";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

   //create maincl for range split
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": 1 }, ShardingType: "hash" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 200 }, UpBound: { a: 300 } } );

   //insert data 
   var doc = [{ No: 1, a: 12, b: 12, c: 12 },
   { No: 2, a: { $decimal: "123" }, b: { $decimal: "123" }, c: { $decimal: "123" } },
   { No: 3, a: 200.12, b: 200.12, c: 200.12 }];
   dbcl.insert( doc );

   //many field,some exists,some Non-exists,use gt/lt
   var findCondition1 = { a: { $type: 1, $et: 100 }, b: { $type: 1, $et: 100 }, c: { $type: 1, $et: 100 } };
   var expRecs1 = [{ No: 2, a: { $decimal: "123" }, b: { $decimal: "123" }, c: { $decimal: "123" } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

}
