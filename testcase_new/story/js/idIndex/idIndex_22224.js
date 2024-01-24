/***************************************************************************
@Description :seqDB-22224:带idIndex和不带idIndex的集合执行insert/remove操作（验证回放记录）
@Modify list :
              2020-5-27  wuyan  Init
****************************************************************************/
var csName = CHANGEDPREFIX + "_index22224";
var clName1 = CHANGEDPREFIX + "_index22224a";
var clName2 = CHANGEDPREFIX + "_index22224b";
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   commDropCS( db, csName, true, "drop CS in the beginning." );

   var groups = commGetGroups( db );
   var groupName = groups[0][0].GroupName;
   var dbcl1 = commCreateCL( db, csName, clName1, { Group: groupName } );
   var dbcl2 = commCreateCL( db, csName, clName2, { Group: groupName, AutoIndexId: false } );
   dbcl1.createIndex( 'a', { a: 1 }, true );

   //insert data   
   for( var i = 0; i < 50; i++ )
   {
      for( var j = 0; j < 100; j++ )
         dbcl1.insert( { a: j } );
      for( var j = 0; j < 100; j++ )
         dbcl1.remove( { a: j } );
      for( var j = 0; j < 40; j++ )
         dbcl2.insert( { a: j } );
   }

   //check result
   var clCount1 = dbcl1.count();
   var clCount2 = dbcl2.count();

   var num = 2000;
   if( Number( clCount1 ) !== 0 || Number( clCount2 ) !== num )
   {
      throw new Error( "clCount1 is " + clCount1 + ".expect count is 0."
         + "\nclCount2 is " + clCount2 + ".expect count is " + num );
   }

   commDropCS( db, csName, true, "drop CS in the ending." );
}
