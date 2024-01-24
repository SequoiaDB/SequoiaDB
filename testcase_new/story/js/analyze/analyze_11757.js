/************************************
*@Description:  SampleRecords取不同值执行统计
*@author:      liuxiaoxuan
*@createdate:  2017.11.09
*@testlinkCase: seqDB-11757
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "11757";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl
   var clName = COMMCLNAME + "11757";
   var dbcl = commCreateCL( db, csName, clName );

   var clFullName = csName + "." + clName;

   //insert datas
   var insertNums = 150;
   insertDiffDatas( dbcl, insertNums );

   commCreateIndex( dbcl, "a", { a: 1 } );

   //analyze with SampleNum
   var options = { CollectionSpace: csName, SampleNum: 200 };
   db.analyze( options );

   //check SampleNum
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "", false, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var expResult = { "SampleRecords": 150, "TotalRecords": 150 };
   checkInfoState( csName, clName, expResult );

   //insert datas again
   insertDiffDatas( dbcl, insertNums );

   //analyze again with SampleNum
   var options = { CollectionSpace: csName, SampleNum: 150 };
   db.analyze( options );

   //check SampleNum again
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "", false, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var expResult = { "SampleRecords": 150, "TotalRecords": 300 };
   checkInfoState( csName, clName, expResult );

   //analyze again without SampleNum
   var options = { CollectionSpace: csName };
   db.analyze( options );

   //check SampleNum again
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "", false, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   var expResult = { "SampleRecords": 200, "TotalRecords": 300 };
   checkInfoState( csName, clName, expResult );

   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkInfoState ( csName, clName, expResult )
{
   var groups = commGetCLGroups( db, csName + "." + clName );
   var datas = getNodesInGroups( db, groups );

   for( var i in datas )
   {
      var nodesInGroup = datas[i];
      for( var j in nodesInGroup )
      {
         //check collection state
         var matcher = { "CollectionSpace": csName, "Collection": clName };
         var actResult = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" ).find( matcher );

         if( 0 < actResult.length )
         {
            var expSampleNum = expResult["SampleRecords"];
            var expTotalRecord = expResult["TotalRecords"];

            var actSampleNum = actResult[0]["SampleRecords"];
            var actTotalRecord = actResult[0]["TotalRecords"];

            if( expSampleNum !== actSampleNum && expTotalRecord !== actTotalRecord )
            {
               throw new Error( "check failed" );
            }
         }

         //check index state
         actResult = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find( matcher );
         if( 0 < actResult.length )
         {
            var expSampleNum = expResult["SampleRecords"];
            var expTotalRecord = expResult["TotalRecords"];

            var actSampleNum = actResult[0]["SampleRecords"];
            var actTotalRecord = actResult[0]["TotalRecords"];

            if( expSampleNum !== actSampleNum && expTotalRecord !== actTotalRecord )
            {
               throw new Error( "check failed" );
            }
         }
      }
   }
}