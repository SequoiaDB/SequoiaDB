/* *****************************************************************************
@discretion: seqDB-20322:枚举数据库中所有的集合信息
@author：2018-11-06 zhaoxiaoni
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clName1 = CHANGEDPREFIX + "clName1";
   var clName2 = CHANGEDPREFIX + "clName2";
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );

   var names = [];
   var cursor = db.listCollections();
   while( cursor.next() )
   {
      var name = cursor.current().toObj()["Name"];
      names.push( name );
   }
   if( names.indexOf( COMMCSNAME + "." + clName1 ) === -1 || names.indexOf( COMMCSNAME + "." + clName2 ) === -1 )
   {
      throw new Error( "The actual collections are " + names );
   }

   commDropCL( db, COMMCSNAME, clName1, false, false );
   commDropCL( db, COMMCSNAME, clName2, false, false );
}
