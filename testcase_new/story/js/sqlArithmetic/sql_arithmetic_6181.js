/************************************************************************
*@Description:     seqDB-6181:针对不同的数据类型做算术运算_st.sql.arithExpre.004
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
   var clName = COMMCLNAME + "_6181";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );

   insertRecs( csName, clName );
   var rc = selectRecs( csName, clName );
   checkResult( rc );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function insertRecs ( csName, clName )
{

   db.execUpdate( "insert into " + csName + "." + clName + "(a, b, c, d, e, f) values(-2147483648, 2147483647, -9223372036854775808, 9223372036854775807, -1.0000000000001, 1.999999999999)" );
}

function selectRecs ( csName, clName )
{

   var rc0 = db.exec( "select a+1, b+1, c+1, d+1, e+1, f+1 from " + csName + "." + clName );
   var rc1 = db.exec( "select a-1, b-1, c-1, d-1, e-1, f-1 from " + csName + "." + clName );
   var rc2 = db.exec( "select a*1.1, b*1.1, c*1.1, d*1.1, e*1.1, f*1.1 from " + csName + "." + clName );
   var rc3 = db.exec( "select a/0.9, b/0.9, c/0.9, d/0.9, e/0.9, f/0.9 from " + csName + "." + clName );

   var rc = [rc0, rc1, rc2, rc3];
   return rc;
}

function checkResult ( rc )
{

   //compare the records for rc[0]
   var expA = -2147483647;
   var expB = 2147483648;
   var expC = "-9223372036854775807";
   var expD = "9223372036854775808";
   var expE = -9.992007221626409e-14;
   var expF = 2.999999999999;

   var actA = rc[0].current().toObj()["a"];
   var actB = rc[0].current().toObj()["b"];
   var actC = String( rc[0].current().toObj()["c"]["$numberLong"] );
   var actD = String( rc[0].current().toObj()["d"]["$decimal"] );
   var actE = rc[0].current().toObj()["e"];
   var actF = rc[0].current().toObj()["f"];
   if( expA !== actA || expB !== actB || expC !== actC || expD !== actD
      || expE !== actE || expF !== actF )
   {
      throw new Error( "checkResult fail,[ rc[0] ]" +
         "[a:" + expA + ",b:" + expB + ",c:" + expC + ",d:" + expD + ",e:" + expE + ",f:" + expF + "]" +
         "[a:" + actA + ",b:" + actB + ",c:" + actC + ",d:" + actD + ",e:" + actE + ",f:" + actF + "]" );
   }

   //compare the records for rc[1]
   var expA = -2147483649;
   var expB = 2147483646;
   var expC = "-9223372036854775809";  //expC: "-9223372036854775809"
   var expD = "9223372036854775806";
   var expE = -2.0000000000001;
   var expF = 0.9999999999989999;

   var actA = rc[1].current().toObj()["a"];
   var actB = rc[1].current().toObj()["b"];
   var actC = String( rc[1].current().toObj()["c"]["$decimal"] );
   var actD = String( rc[1].current().toObj()["d"]["$numberLong"] );
   var actE = rc[1].current().toObj()["e"];
   var actF = rc[1].current().toObj()["f"];
   if( expA !== actA || expB !== actB || expC !== actC || expD !== actD
      || expE !== actE || expF !== actF )
   {
      throw new Error( "checkResult fail,[ rc[1] ]" +
         "[a:" + expA + ",b:" + expB + ",c:" + expC + ",d:" + expD + ",e:" + expE + ",f:" + expF + "]" +
         "[a:" + actA + ",b:" + actB + ",c:" + actC + ",d:" + actD + ",e:" + actE + ",f:" + actF + "]" );
   }

   //compare the records for rc[2]
   var expA = -2362232012.8;
   var expB = 2362232011.7;
   var expC = -10145709240540250000;
   var expD = 10145709240540250000;
   var expE = -1.10000000000011;
   var expF = 2.1999999999989;

   var actA = rc[2].current().toObj()["a"];
   var actB = rc[2].current().toObj()["b"];
   var actC = rc[2].current().toObj()["c"];
   var actD = rc[2].current().toObj()["d"];
   var actE = rc[2].current().toObj()["e"];
   var actF = rc[2].current().toObj()["f"];
   if( expA !== actA || expB !== actB || expC !== actC || expD !== actD
      || expE !== actE || expF !== actF )
   {
      throw new Error( "checkResult fail,[ rc[2] ]" +
         "[a:" + expA + ",b:" + expB + ",c:" + expC + ",d:" + expD + ",e:" + expE + ",f:" + expF + "]" +
         "[a:" + actA + ",b:" + actB + ",c:" + actC + ",d:" + actD + ",e:" + actE + ",f:" + actF + "]" );
   }

   //compare the records for rc[3]
   var expA = -2386092942.222222;
   var expB = 2386092941.111111;
   var expC = -10248191152060860000;
   var expD = 10248191152060860000;
   var expE = -1.111111111111222;
   var expF = 2.222222222221111;

   var actA = rc[3].current().toObj()["a"];
   var actB = rc[3].current().toObj()["b"];
   var actC = rc[3].current().toObj()["c"];
   var actD = rc[3].current().toObj()["d"];
   var actE = rc[3].current().toObj()["e"];
   var actF = rc[3].current().toObj()["f"];
   if( expA !== actA || expB !== actB || expC !== actC || expD !== actD
      || expE !== actE || expF !== actF )
   {
      throw new Error( "checkResult fail,[ rc[3] ]" +
         "[a:" + expA + ",b:" + expB + ",c:" + expC + ",d:" + expD + ",e:" + expE + ",f:" + expF + "]" +
         "[a:" + actA + ",b:" + actB + ",c:" + actC + ",d:" + actD + ",e:" + actE + ",f:" + actF + "]" );
   }

}