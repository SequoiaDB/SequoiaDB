/*******************************************************************************
*@Description : seqDB-7526::shell_输入strict格式，查询显示
seqDB-7527::shell_输入js格式，查询显示
*@author : 2020-4-16 Zhao Xiaoni  Init
*******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_7526";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   var record = { a: NumberLong( "-2147483647" ) };
   cl.insert( record );
   var rc = cl.find();
   var expRecord = [{ a: -2147483647 }];
   commCompareResults( rc, expRecord );

   record = { a: NumberLong( -2147483648 ) };
   cl.insert( record );
   rc = cl.find();
   expRecord.push( { a: -2147483648 } );
   commCompareResults( rc, expRecord );

   record = { a: { $numberLong: "2147483647" } };
   cl.insert( record );
   rc = cl.find();
   expRecord.push( { a: 2147483647 } );
   commCompareResults( rc, expRecord );

   record = { a: { $numberLong: "9007199254740992" } };
   cl.insert( record );
   rc = cl.find();
   expRecord.push( record );
   commCompareResults( rc, expRecord );

   commDropCL( db, COMMCSNAME, clName, false, false );
}
