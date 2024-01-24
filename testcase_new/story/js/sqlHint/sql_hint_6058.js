/************************************************************************
*@Description:    seqDB-6058:hint指定的表和索引跟select语句中表名不对应_st.sql.hint.010
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6058";
   var idxName = CHANGEDPREFIX + "_idx";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );
   createIndex( csName, clName, idxName );

   insertRecs( csName, clName );
   var preTotalIndexRead = snapshot();
   var rtRecsArray = selectRecs( csName, clName, idxName );
   var aftTotalIndexRead = snapshot();

   checkResult( rtRecsArray, preTotalIndexRead, aftTotalIndexRead );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function createIndex ( csName, clName, idxName )
{

   db.execUpdate( "create index " + idxName + " on " + csName + "." + clName + "( a )" );
}

function insertRecs ( csName, clName )
{

   db.execUpdate( "insert into " + csName + "." + clName + "(a,b,c) values(1,1,1)" );
   db.execUpdate( "insert into " + csName + "." + clName + "(a,b,c) values(2,2,2)" );
}

function selectRecs ( csName, clName, idxName )
{

   var rc = db.exec( "select t1.c from " + csName + "." + clName + " as t1 where t1.b = 2 /*+use_index(t2, " + idxName + ")*/" );

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
   var expC = 2;
   var actRecsCount = rtRecsArray.length;
   var actC = rtRecsArray[0]["c"];
   if( expRecsCount !== actRecsCount || expC !== actC )
   {
      throw new Error( "checkResult fail,[ compare records ]" +
         "[recsCount:" + expRecsCount + ", c:" + expC + "]" +
         "[recsCount:" + actRecsCount + ", c:" + actC + "]" );
   }

   //judge whether to walk index
   var times = aftTotalIndexRead - preTotalIndexRead;
   var expWalkIndex = false;
   //init "walkIndex"
   var walkIndex = false;
   if( times === 0 )
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