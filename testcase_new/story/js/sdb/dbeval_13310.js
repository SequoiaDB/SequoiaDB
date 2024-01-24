/******************************************************************************
*@Description : test get collection with eval
*               TestLink : seqDB-13310:执行eval获取集合后插入查询数据
*@auhor       : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = CHANGEDPREFIX + "_cl13310";
   commCreateCL( db, COMMCSNAME, clName );

   var code = "db." + COMMCSNAME + "." + clName;
   var cl = db.eval( code );
   cl.insert( { a: 1 } );

   var value = cl.find( {}, { a: "" } ).next().toObj()["a"];
   if( value !== 1 )
   {
      throw new Error( "expect value is 1, but act value is " + value );
   }

   commDropCL( db, COMMCSNAME, clName );
}
