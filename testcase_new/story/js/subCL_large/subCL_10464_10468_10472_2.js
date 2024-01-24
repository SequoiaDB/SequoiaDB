/************************************
*@Description: find and sort use limit,
*@author:      zhaoyu
*@createdate:  2016.11.23
*@testlinkCase:seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467/seqDB-10468/seqDB-10469/
               seqDB-10470/seqDB-10471/seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475 
**************************************/

main( test );
function test ()
{
   db.setSessionAttr( { PreferedInstance: "M" } );
   //clean environment before test
   mainCL_Name = COMMCLNAME + "_maincl10464_2";
   subCL_Name1 = COMMCLNAME + "_subcl104641_2";
   subCL_Name2 = COMMCLNAME + "_subcl104642_2";
   subCL_Name3 = COMMCLNAME + "_subcl104643_2";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean " + subCL_Name1 );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean " + subCL_Name2 );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean " + subCL_Name3 );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean " + mainCL_Name );

   //check test environment before split
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

   //create maincl for range split
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { ShardingKey: { "a": -1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range", ReplSize: 0 };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": -1 }, ShardingType: "hash", ReplSize: 0 };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   attachCL( dbcl, COMMCSNAME + "." + subCL_Name1, { LowBound: { a: 200 }, UpBound: { a: 100 } } );
   attachCL( dbcl, COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 0 } } );
   attachCL( dbcl, COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 0 }, UpBound: { a: -100 } } );

   //insert data; 
   var recordNum = 4000;
   var recordStart = -10;
   var recordEnd = 5;
   insertBulkData( dbcl, recordNum, recordStart, recordEnd );

   //set sort
   sortOptions = { a: 1, b: -1, c: 1 };

   //find data and check result use limit ,seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467
   limitNum = 3000;
   var rc = dbcl.find().sort( sortOptions ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //find data and check result use skip ,seqDB-10468/seqDB-10469/seqDB-10470/seqDB-10471
   skipNum = 375;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum );

   checkRec( rc, recordNum - skipNum, sortOptions );

   //find data and check result use skip and limit;seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475
   skipNum = 375;
   limitNum = 2900;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //set sort
   sortOptions = { b: 1, a: -1, c: -1 };

   //find data and check result use limit ,seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467
   limitNum = 3000;
   var rc = dbcl.find().sort( sortOptions ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //find data and check result use skip ,seqDB-10468/seqDB-10469/seqDB-10470/seqDB-10471
   skipNum = 375;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum );

   checkRec( rc, recordNum - skipNum, sortOptions );

   //find data and check result use skip and limit;seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475
   skipNum = 375;
   limitNum = 2900;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //set sort
   sortOptions = { c: 1, a: -1 };

   //find data and check result use limit ,seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467
   limitNum = 3000;
   var rc = dbcl.find().sort( sortOptions ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //find data and check result use skip ,seqDB-10468/seqDB-10469/seqDB-10470/seqDB-10471
   skipNum = 375;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum );

   checkRec( rc, recordNum - skipNum, sortOptions );

   //find data and check result use skip and limit;seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475
   skipNum = 375;
   limitNum = 2900;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //set sort
   sortOptions = { c: -1, b: 1 };

   //find data and check result use limit ,seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467
   limitNum = 3000;
   var rc = dbcl.find().sort( sortOptions ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //find data and check result use skip ,seqDB-10468/seqDB-10469/seqDB-10470/seqDB-10471
   skipNum = 375;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum );

   checkRec( rc, recordNum - skipNum, sortOptions );

   //find data and check result use skip and limit;seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475
   skipNum = 375;
   limitNum = 2900;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //set sort
   sortOptions = { b: -1, a: 1 };

   //find data and check result use limit ,seqDB-10464/seqDB-10465/seqDB-10466/seqDB-10467
   limitNum = 3000;
   var rc = dbcl.find().sort( sortOptions ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   //find data and check result use skip ,seqDB-10468/seqDB-10469/seqDB-10470/seqDB-10471
   skipNum = 375;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum );

   checkRec( rc, recordNum - skipNum, sortOptions );

   //find data and check result use skip and limit;seqDB-10472/seqDB-10473/seqDB-10474/seqDB-10475
   skipNum = 375;
   limitNum = 2900;
   var rc = dbcl.find().sort( sortOptions ).skip( skipNum ).limit( limitNum );

   checkRec( rc, limitNum, sortOptions );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean collection" );
}
