/******************************************************************************
*@Description : test use decimal() make special decimal value 
*               seqDB-19165:使用Decimal函数插入/删除decimal类型的普通数据         
*@author      : luweikang
******************************************************************************/
main( test )
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   var docs = [{ a: NumberDecimal( "9223372036854775807198410" ) },
   { a: NumberDecimal( "-9223372036854775808197101" ) },
   { a: NumberDecimal( "-4.94065645841246544E-380" ) },
   { a: NumberDecimal( "4.94065645841246544E-390" ) },
   { a: NumberDecimal( "9223372036854775807198411", [1000, 100] ) },
   { a: NumberDecimal( "-9223372036854775808197102", [1000, 100] ) },
   { a: NumberDecimal( "-1.71E+398", [1000, 100] ) },
   { a: NumberDecimal( "1.71E+378", [1000, 100] ) },
   { a: NumberDecimal( "-4.964065645841246544E-380", [1000, 999] ) },
   { a: NumberDecimal( "4.964065645841246544E-390", [1000, 999] ) }];

   var expDocs = [{ a: { $decimal: "9223372036854775807198410" } },
   { a: { $decimal: "-9223372036854775808197101" } },
   { a: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841246544" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841246544" } },
   { a: { $decimal: "9223372036854775807198411.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 100] } },
   { a: { $decimal: "-9223372036854775808197102.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 100] } },
   { a: { $decimal: "-171000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 100] } },
   { a: { $decimal: "1710000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 100] } },
   { a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000049640656458412465440000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 999] } },
   { a: { $decimal: "0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004964065645841246544000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", $precision: [1000, 999] } }];
   cl.insert( docs );

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = cl.find( docs[i] );
      var expRecs = [expDocs[i]];
      commCompareResults( cursor, expRecs );
   }

   for( var i = 0; i < docs.length; i++ )
   {
      cl.remove( docs[i] );
      var recordNum = cl.count();
      var expRecordNum = docs.length - 1 - i;
      if( recordNum != expRecordNum )
      {
         throw new Error( "recordNum: " + recordNum + "\nexpRecordNum: " + expRecordNum );
      }
   }

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the end" );
}
