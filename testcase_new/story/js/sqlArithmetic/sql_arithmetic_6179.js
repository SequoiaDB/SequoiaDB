/************************************************************************
*@Description:   seqDB-6179:复合运算_st.sql.arithExpre.002
*@Author:  2016/7/12  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   if( isTransautocommit() ) 
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6179";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );

   insertRecs( csName, clName );
   var rc = selectRecs( csName, clName );
   checkResult( rc );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function insertRecs ( csName, clName )
{

   db.execUpdate( "insert into " + csName + "." + clName + "(a) values(6)" );
}

function selectRecs ( csName, clName )
{

   var rc0 = db.exec( "select ( a + 1) * ( a - 1 ) / ( a % 4) from " + csName + "." + clName );
   var rc1 = db.exec( "select a + 2 * a - a / 2 % 3 from " + csName + "." + clName );
   var rc = [rc0, rc1];
   return rc;
}

function checkResult ( rc )
{

   //compare the records for rc[0]
   var expA = 17;
   var actA = rc[0].current().toObj().a;
   assert.equal( expA, actA );

   //compare the records for rc[1]
   var expA = 18;
   var actA = rc[1].current().toObj().a;
   assert.equal( expA, actA );

}