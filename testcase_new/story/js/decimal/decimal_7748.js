/************************************
*@Description: delete decimal data
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7748";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "9223372036854775807198410" } },
   { a: { $decimal: "-9223372036854775808197101" } },
   { a: { $decimal: "-1.7E+398" } },
   { a: { $decimal: "1.7E+378" } },
   { a: { $decimal: "-4.94065645841246544E-380" } },
   { a: { $decimal: "4.94065645841246544E-390" } },
   { a: { $decimal: "9223372036854775807198411", $precision: [1000, 100] } },
   { a: { $decimal: "-9223372036854775808197102", $precision: [1000, 100] } },
   { a: { $decimal: "-1.71E+398", $precision: [1000, 100] } },
   { a: { $decimal: "1.71E+378", $precision: [1000, 100] } },
   { a: { $decimal: "-4.964065645841246544E-380", $precision: [1000, 999] } },
   { a: { $decimal: "4.964065645841246544E-390", $precision: [1000, 999] } }];
   dbcl.insert( doc );

   //delete data
   dbcl.remove();

   //check result
   var recordNum = dbcl.count();
   if( 0 !== parseInt( recordNum ) )
   {
      throw new Error( "parseInt( recordNum ): " + parseInt( recordNum ) );
   }
   commDropCL( db, COMMCSNAME, clName );
}

