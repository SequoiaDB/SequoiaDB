/******************************************************************************
*@Description : test + - * / % with sql of special decimal value
*               seqDB-13997:使用内置sql对特殊decimal值做数学运算           
*@author      : Liang XueWang 
******************************************************************************/

main( test )
function test ()
{
   var clName = COMMCLNAME + "_13997";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   var operators = ["+", "-", "*", "/", "%"];
   var expRecs = [{ a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } }];
   for( var i = 0; i < operators.length; i++ )
   {
      var op = operators[i];
      var sql = "select a" + op + "2 from " + COMMCSNAME + "." + clName;
      checkSqlResult( db, sql, expRecs );
   }
   commDropCL( db, COMMCSNAME, clName );
}
