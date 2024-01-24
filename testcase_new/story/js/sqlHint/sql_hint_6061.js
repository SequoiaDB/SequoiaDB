/************************************************************************
*@Description:   seqDB-6054:在嵌套查询中的不同SELECT语句中使用hint_st.sql.hint.006
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName1 = COMMCLNAME + "_6053_1";
   var clName2 = COMMCLNAME + "_6053_2";
   var clName3 = COMMCLNAME + "_6053_3";
   var idxName = CHANGEDPREFIX + "_idx";

   dropCL( csName, clName1, true, "Failed to drop cl in the begin." );
   dropCL( csName, clName2, true, "Failed to drop cl in the begin." );
   dropCL( csName, clName3, true, "Failed to drop cl in the begin." );
   createCL( csName, clName1, true, true, "Failed to create cl." );
   createCL( csName, clName2, true, true, "Failed to create cl." );
   createCL( csName, clName3, true, true, "Failed to create cl." );
   createIndex( csName, clName1, idxName );
   createIndex( csName, clName3, idxName );

   insertRecs( csName, clName1, clName2, clName3 );
   var preTotalIndexRead = snapshot();
   var rtRecsArray = selectRecs( csName, clName1, clName2, clName3, idxName );
   var aftTotalIndexRead = snapshot();

   checkResult( rtRecsArray, preTotalIndexRead, aftTotalIndexRead );

   dropCL( csName, clName1, false, "Failed to drop cl in the end." );
   dropCL( csName, clName2, false, "Failed to drop cl in the end." );
   dropCL( csName, clName3, false, "Failed to drop cl in the end." );
}

function createIndex ( csName, clName, idxName )
{

   db.execUpdate( "create index " + idxName + " on " + csName + "." + clName + "( a )" );
}

function insertRecs ( csName, clName1, clName2, clName3 )
{

   db.execUpdate( "insert into " + csName + "." + clName1 + "(a,b,c) values(2,1,1)" );
   db.execUpdate( "insert into " + csName + "." + clName2 + "(a,b,c) values(2,2,2)" );
   db.execUpdate( "insert into " + csName + "." + clName3 + "(a,b,c) values(3,3,2)" );
}

function selectRecs ( csName, clName1, clName2, clName3, idxName )
{

   var rc = db.exec( "select t1.a, t2.cnt from " + csName + "." + clName1 + " as t1 inner join ( select t3.b, t3.cnt, t4.c from ( select count(a) as cnt, b from " + csName + "." + clName2 + " group by b ) as t3 right outer join " + csName + "." + clName3 + " as t4 on t3.b = t4.c /*+use_index(t4, " + idxName + ") use_hash()*/ ) as t2 on t1.a = t2.b /*+use_index(t1, " + idxName + ") use_hash()*/" );

   var rtRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      rtRecsArray.push( tmpRecs.toObj() );
   }
   return rtRecsArray;
}

function snapshot ()
{

   var TotalIndexRead = db.snapshot( 6 ).current().toObj()["TotalIndexRead"];

   return TotalIndexRead;
}

function checkResult ( rtRecsArray, preTotalIndexRead, aftTotalIndexRead )
{

   //compare the records
   var expRecsCount = 1;
   var expA = 2;
   var expC = 1;  //t3.cnt
   var actRecsCount = rtRecsArray.length;
   var actA = rtRecsArray[0]["a"];
   var actC = rtRecsArray[0]["cnt"];
   if( expRecsCount !== actRecsCount || expA !== actA || expC !== actC )
   {
      throw new Error( "checkResult fail,[ compare records ]" +
         "[recsCount:" + expRecsCount + ", a:" + expA + ", c:" + expC + "]" +
         "[recsCount:" + actRecsCount + ", a:" + actA + ", c:" + actC + "]" );
   }

   //judge whether to walk index
   var times = aftTotalIndexRead - preTotalIndexRead;
   var expWalkIndex = true;
   //init "walkIndex"
   var walkIndex = false;
   if( times >= 1 )
   {
      walkIndex = true;
   }
   else
   {
      throw new Error( "checkResult fail,[ compare index ]" +
         "[walkIndex:" + expWalkIndex + "]" +
         "[walkIndex:" + walkIndex + "]" );
   }

}