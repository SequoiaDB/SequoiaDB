/************************************
*@Description: find out of range data,
*@author:      zhaoyu
*@createdate:  2016.12.27
*@testlinkCase:seqDB-10895
**************************************/

main( test );

function test ()
{
   //set find data from primary Node
   db.setSessionAttr( { PreferedInstance: "M" } );

   //clean environment before test
   mainCL_Name = COMMCLNAME + "_maincl10895";
   subCL_Name1 = COMMCLNAME + "_subcl10895_1";
   subCL_Name2 = COMMCLNAME + "_subcl10895_2";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

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
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   commCreateCL( db, COMMCSNAME, subCL_Name1 );
   commCreateCL( db, COMMCSNAME, subCL_Name2 );

   //attach subcl
   attachCL( dbcl, COMMCSNAME + "." + subCL_Name1, { LowBound: { a: 1 }, UpBound: { a: 100 } } );
   attachCL( dbcl, COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 1000 }, UpBound: { a: 10000 } } );

   //insert data 
   var doc = [{ a: 1 }, { a: 1000 }];
   insertData( dbcl, doc );

   //find out of range data
   var findCondition1 = { a: 0 };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { a: 1 };
   var expRecs2 = [{ a: 1 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { a: 100 };
   var expRecs3 = [];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var findCondition4 = { a: 1000 };
   var expRecs4 = [{ a: 1000 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var findCondition5 = { a: 10000 };
   var expRecs5 = [];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}

function insertData ( dbcl, condition )
{
   dbcl.insert( condition );
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition )
{
   var rc = sortFindData( dbcl, findCondition, findCondition2, sortCondition );
   checkRec( rc, expRecs );
}

/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/************************************
*@Description: find and sort data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function sortFindData ( dbcl, findCondition, findCondition2, sortCondition )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = null; }
   var sortResult = dbcl.find( findCondition, findCondition2 ).sort( sortCondition );
   return sortResult;
}