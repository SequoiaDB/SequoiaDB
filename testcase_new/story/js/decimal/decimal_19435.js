/******************************************************************************
*@Description : seqDB-19435:存储过程调用Decimal函数
*@author      : luweikang
*@createDate  : 2019.9.12
******************************************************************************/

main( test )
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var csName = "cs19435";
   var clName = "cl19435";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var cl = commCreateCL( db, csName, clName );

   db.createProcedure( function insert19435 () { db.getCS( "cs19435" ).getCL( "cl19435" ).insert( { a: NumberDecimal( "100.04", [5, 2] ) } ); } );
   db.eval( "insert19435()" );

   var cursor = cl.find( { a: NumberDecimal( "100.04", [5, 2] ) } );
   var expRecs = [{ "a": { "$decimal": "100.04", "$precision": [5, 2] } }];
   commCompareResults( cursor, expRecs );

   db.createProcedure( function delete19435 () { db.getCS( "cs19435" ).getCL( "cl19435" ).remove( { a: NumberDecimal( "100.04", [5, 2] ) } ); } );
   db.eval( "delete19435()" );

   cl.insert( { a: 1 } );

   var cursor = cl.find();
   var expRecs = [{ "a": 1 }];
   commCompareResults( cursor, expRecs );

   db.removeProcedure( "insert19435" );
   db.removeProcedure( "delete19435" );
   commDropCS( db, csName, true, "drop CS in the end" );
}
