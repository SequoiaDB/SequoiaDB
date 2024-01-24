/************************************************************************
*@Description:    算术运算的一个表达式中有多个字段_st.sql.arithExpre.007
*@Author:  2016/7/12  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6184";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );

   insertRecs( csName, clName );
   selectRecs( csName, clName );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function insertRecs ( csName, clName )
{

   db.execUpdate( "insert into " + csName + "." + clName + "(a) values(6)" );
}

function selectRecs ( csName, clName )
{

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.exec( "select a+b from " + csName + "." + clName );
   } );
}