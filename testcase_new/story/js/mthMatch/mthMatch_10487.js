/************************************
*@Description: main-sub table use string functions,
*@author:      zhaoyu
*@createdate:  2016.11.18
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
   mainCL_Name = CHANGEDPREFIX + "_maincl10487";
   subCL_Name1 = CHANGEDPREFIX + "_subcl104871";
   subCL_Name2 = CHANGEDPREFIX + "_subcl104872";
   subCL_Name3 = CHANGEDPREFIX + "_subcl104873";

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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: "h" } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: "h" }, UpBound: { a: "o" } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: "o" }, UpBound: { a: { $maxKey: 1 } } } );

   //insert data 
   var doc = [{ No: 1, a: " \n\t\raString\n\r\t ", b: " \n\t\raString\n\r\t ", c: " \n\t\raString\n\r\t " },
   { No: 2, a: "aString", b: "aString", c: "aString" },
   { No: 3, a: "hString", b: "hString", c: "hString" },
   { No: 4, a: "oString", b: "oString", c: "oString" }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $substr: [1, 3], $et: "Str" }, b: { $substr: [1, 3], $et: "Str" }, c: { $substr: [1, 3], $et: "Str" } };
   var expRecs1 = [{ No: 2, a: "aString", b: "aString", c: "aString" },
   { No: 3, a: "hString", b: "hString", c: "hString" },
   { No: 4, a: "oString", b: "oString", c: "oString" }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $strlen: 1, $et: 7 }, b: { $strlen: 1, $et: 7 }, c: { $strlen: 1, $et: 7 } };
   var expRecs2 = [{ No: 2, a: "aString", b: "aString", c: "aString" },
   { No: 3, a: "hString", b: "hString", c: "hString" },
   { No: 4, a: "oString", b: "oString", c: "oString" }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { a: { $upper: 1, $substr: [1, 3], $et: "STR" }, b: { $upper: 1, $substr: [1, 3], $et: "STR" }, c: { $upper: 1, $substr: [1, 3], $et: "STR" } };
   var expRecs3 = [{ No: 2, a: "aString", b: "aString", c: "aString" },
   { No: 3, a: "hString", b: "hString", c: "hString" },
   { No: 4, a: "oString", b: "oString", c: "oString" }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { a: { $lower: 1, $substr: [1, 3], $et: "str" }, b: { $lower: 1, $substr: [1, 3], $et: "str" }, c: { $lower: 1, $substr: [1, 3], $et: "str" } };
   var expRecs4 = [{ No: 2, a: "aString", b: "aString", c: "aString" },
   { No: 3, a: "hString", b: "hString", c: "hString" },
   { No: 4, a: "oString", b: "oString", c: "oString" }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var findCondition5 = { a: { $trim: 1, $et: "aString" }, b: { $trim: 1, $et: "aString" }, c: { $trim: 1, $et: "aString" } };
   var expRecs5 = [{ No: 1, a: " \n\t\raString\n\r\t ", b: " \n\t\raString\n\r\t ", c: " \n\t\raString\n\r\t " },
   { No: 2, a: "aString", b: "aString", c: "aString" }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $ltrim: 1, $substr: [0, 3], $et: "aSt" }, b: { $ltrim: 1, $substr: [0, 3], $et: "aSt" }, c: { $ltrim: 1, $substr: [0, 3], $et: "aSt" } };
   var expRecs6 = [{ No: 1, a: " \n\t\raString\n\r\t ", b: " \n\t\raString\n\r\t ", c: " \n\t\raString\n\r\t " },
   { No: 2, a: "aString", b: "aString", c: "aString" }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   var findCondition7 = { a: { $rtrim: 1, $et: " \n\t\raString" }, b: { $rtrim: 1, $et: " \n\t\raString" }, c: { $rtrim: 1, $et: " \n\t\raString" } };
   var expRecs7 = [{ No: 1, a: " \n\t\raString\n\r\t ", b: " \n\t\raString\n\r\t ", c: " \n\t\raString\n\r\t " }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}
