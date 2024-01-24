/************************************************************************
*@Description:    seqDB-6180:select嵌套语句中使用算术运算_st.sql.arithExpre.003
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
   var clName = COMMCLNAME + "_6180";

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

   var rc0 = db.exec( "select t.b+1 as c from (select a as b from " + csName + "." + clName + " split by a) as t" );
   var rc1 = db.exec( "select t4.mult as div from ( select t3.subt*3 as mult from ( select t2.sum-2 as subt from ( select t1.a+1 as sum from " + csName + "." + clName + " as t1 ) as t2 ) as t3 ) as t4" );

   var rc = [rc0, rc1];
   return rc;
}

function checkResult ( rc )
{

   //compare the records for rc[0]
   var expValue = 7;
   var actValue = rc[0].current().toObj().c;
   assert.equal( expValue, actValue );

   //compare the records for rc[1]
   var expValue = 15;
   var actValue = rc[1].current().toObj().div;
   assert.equal( expValue, actValue );

}