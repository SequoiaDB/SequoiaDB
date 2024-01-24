/************************************************************************
*@Description:  seqDB-6055:select查询多个表中字段，指定hint为+use_index(myindex)_st.sql.hint.007
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName1 = COMMCLNAME + "_6055_1";
   var clName2 = COMMCLNAME + "_6055_2";
   var idxName = CHANGEDPREFIX + "_idx";

   dropCL( csName, clName1, true, "Failed to drop cl in the begin." );
   dropCL( csName, clName2, true, "Failed to drop cl in the begin." );
   createCL( csName, clName1, true, true, "Failed to create cl." );
   createCL( csName, clName2, true, true, "Failed to create cl." );
   createIndex( csName, clName1, idxName );
   createIndex( csName, clName2, idxName );

   insertRecs( csName, clName1, clName2 );
   var preTotalIndexRead = snapshot();
   var rtRecsArray = selectRecs( csName, clName1, clName2, idxName );
   var aftTotalIndexRead = snapshot();

   checkResult( rtRecsArray, preTotalIndexRead, aftTotalIndexRead );

   dropCL( csName, clName1, false, "Failed to drop cl in the end." );
   dropCL( csName, clName2, false, "Failed to drop cl in the end." );
}

function createIndex ( csName, clName, idxName )
{

   db.execUpdate( "create index " + idxName + " on " + csName + "." + clName + "( a )" );
}

function insertRecs ( csName, clName1, clName2 )
{

   db.execUpdate( "insert into " + csName + "." + clName1 + "(a,b,c) values(1,2,1)" );
   db.execUpdate( "insert into " + csName + "." + clName2 + "(a,b,c) values(2,1,2)" );
}

function selectRecs ( csName, clName1, clName2, idxName )
{

   var rc = db.exec( "select t1.a, t2.c from " + csName + "." + clName1 + " as t1 inner join " + csName + "." + clName2 + " as t2 on t1.a = t2.b /*+use_index(" + idxName + ")*/" );

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
   var expA = 1;
   var expC = 2;
   var actRecsCount = rtRecsArray.length;
   var actA = rtRecsArray[0]["a"];
   var actC = rtRecsArray[0]["c"];
   if( expRecsCount !== actRecsCount || expA !== actA || expC !== actC )
   {
      throw new Error( "checkResult fail,[ compare records ]" +
         "[recsCount:" + expRecsCount + ", a:" + expA + ", c:" + expC + "]" +
         "[recsCount:" + actRecsCount + ", a:" + actA + ", c:" + expC + "]" );
   }

   //judge whether to walk index
   var times = aftTotalIndexRead - preTotalIndexRead;
   var expWalkIndex = false;
   //init "walkIndex"
   var walkIndex = false;
   if( times == 0 )
   {
      println( "   walkIndex: " + walkIndex );
   }
   else
   {
      walkIndex = true;
      /*
      throw buildException("checkResult", null, "[ compare index ]", 
                       "[walkIndex:"+ expWalkIndex +"]",
                       "[walkIndex:"+ walkIndex +"]");
      */
   }

}