/******************************************************************************
*@Description : test illegal special decimal value
*               seqDB-19166 : Decimal函数参数校验       
*@author      : luweikang 
******************************************************************************/
main( test )
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   var legalDocs = [{ a: NumberDecimal( "mAx" ) },
   { a: NumberDecimal( "MiN" ) },
   { a: NumberDecimal( "-Inf" ) },
   { a: NumberDecimal( "iNF" ) },
   { a: NumberDecimal( "nan" ) }];
   cl.insert( legalDocs );

   try
   {
      var a = { a: NumberDecimal( "MAX1" ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100", [4] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100.1001", [100, 2, 1] ) };
      throw new Error( "need throw error" );;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100", ["a", "b"] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100abc", [4, 1] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal() };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100", [1.1, 1.2] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100", [100, "a"] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      var a = { a: NumberDecimal( "100", [3, 1] ) };
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
}
