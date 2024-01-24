/************************************************************************
*@Description:  seqDB-6178:字段值为数值型，做单列的算术运算（加/减/乘/除/模）_st.sql.arithExpre.001
*@Author:  2016/7/11  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6178";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );

   insertRecs( csName, clName );
   var rc = selectRecs( csName, clName );
   checkResult( rc );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function insertRecs ( csName, clName )
{

   db.execUpdate( "insert into " + csName + "." + clName + "(a,b,c,d) values(1,1,1,1)" );
}

function selectRecs ( csName, clName )
{

   var rc = db.exec( "select a+2, b-2, c*2, d/2 from " + csName + "." + clName );

   return rc;
}

function checkResult ( rc )
{

   //compare the records
   var expA = 3;
   var expB = -1;
   var expC = 2;
   var expD = 0;

   var actA = rc.current().toObj().a;
   var actB = rc.current().toObj().b;
   var actC = rc.current().toObj().c;
   var actD = rc.current().toObj().d;
   if( expA !== actA || expB !== actB || expC !== actC || expD !== actD )
   {
      throw new Eror( "checkResult fail,[ compare records ]" +
         "[a:" + expA + ",b:" + expB + ",c:" + expC + ",d:" + expD + "]" +
         "[a:" + actA + ",b:" + actB + ",c:" + actC + ",d:" + actD + "]" );
   }

}