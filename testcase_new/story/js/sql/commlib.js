/* *****************************************************************************
@discretion: SQL common functions
@modify list:
   2014-3-14 Jianhui Xu  Init
***************************************************************************** */
import( "../lib/main.js" )
import( "../lib/basic_operation/commlib.js" )

// common functions
function sqlInsertAndCheck ( db, csName, clName, num, removeAll, needCheck, message )
{
   if( undefined == num ) { num = 1; }
   if( undefined == message ) { message = "" }
   if( undefined == needCheck ) { needCheck = true; }
   if( undefined == removeAll ) { removeAll = true; }

   // remove first
   if( removeAll )
   {
      db.execUpdate( "delete from " + csName + "." + clName );
   }
   // insert
   for( var i = 0; i < num; ++i )
   {
      var name = "Name_" + i;
      db.execUpdate( "insert into " + csName + "." + clName + "(age, name) values(" + i + ",\"" + name + "\")" );
   }

   if( !needCheck )
   {
      return;
   }
   // check1 - by all
   var size = 0;
   var rc = db.exec( "select * from " + csName + "." + clName );
   size = rc.size();
   assert.equal( size, num );

   // check2 - by find
   var rc;
   var rc = db.exec( "select * from " + csName + "." + clName + " where age >= 0" );
   size = rc.size();
   assert.equal( size, num );
}
