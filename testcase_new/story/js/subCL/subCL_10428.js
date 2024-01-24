/******************************************************
 * @Description: seqDB-10429:未attach子表的主表做数据插入
 * @Author: linsuqiang 
 * @Date: 2016-11-23
 ******************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "_cs";
   var mainCLName = COMMCLNAME + "_mcl";

   commDropCS( db, csName, true, "Failed to drop CS." );

   db.setSessionAttr( { PreferedInstance: "M" } );
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mainCL = createMainCL( csName, mainCLName );

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( { a: "adfadfadf", b: "ijijkkkijikji" } );
   } )

   commDropCS( db, csName, false, "Failed to drop CS." );

}

function createMainCL ( csName, mainCLName )
{

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false, true );
   return mainCL;
}