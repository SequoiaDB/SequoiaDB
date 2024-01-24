/******************************************************************************
*@Description : test illegal special decimal value
*               seqDB-14001:特殊decimal值参数校验         
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_14001";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   var legalDocs = [{ a: { $decimal: "mAx" } },
   { a: { $decimal: "MiN" } },
   { a: { $decimal: "-Inf" } },
   { a: { $decimal: "iNF" } },
   { a: { $decimal: "nan" } }];
   cl.insert( legalDocs );

   var illegalDocs = [{ a: { $decimal: "MAX1" } },
   { a: { $decimal: "1Max" } },
   { a: { $decimal: "MMAX" } },
   { a: { $decimal: "Maxx" } },
   { a: { $decimal: "maax" } },
   { a: { $decimal: " max" } },
   { a: { $decimal: "m ax" } },
   { a: { $decimal: "ma x" } },
   { a: { $decimal: "max " } },
   { a: { $decimal: "abc" } }];
   for( var i = 0; i < illegalDocs.length; i++ )
   {
      try
      {
         cl.insert( illegalDocs[i] );
         throw new Error( "need throw error" );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG )
         {
            throw e;
         }
      }
   }
   commDropCL( db, COMMCSNAME, clName );
}
