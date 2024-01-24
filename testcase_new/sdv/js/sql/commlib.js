/* *****************************************************************************
@discretion: SQL common functions
@modify list:
   2014-3-14 Jianhui Xu  Init
***************************************************************************** */


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
      try
      {
         db.execUpdate( "delete from " + csName + "." + clName );
      }
      catch( e )
      {
         println( "Remove records failed: " + e + ", message: " + message );
         throw e;
      }
   }
   // insert
   try
   {
      for( var i = 0; i < num; ++i )
      {
         var name = "Name_" + i;
         db.execUpdate( "insert into " + csName + "." + clName + "(age, name) values(" + i + ",\"" + name + "\")" );
      }
   }
   catch( e )
   {
      println( "Insert " + i + " records failed: " + e + ", message: " + message );
      throw e;
   }

   if( !needCheck )
   {
      return;
   }
   // check1 - by all
   var size = 0;
   try
   {
      var rc = db.exec( "select * from " + csName + "." + clName );
      size = rc.size();
      if( size != num )
      {
         println( "Count all size: " + size + " is not same with " + num + ", message: " + message );
         throw "count all check failed"
      }
   }
   catch( e )
   {
      println( "Count all exception: " + e + ", message: " + message );
      throw e;
   }
   // check2 - by find
   var rc;
   try
   {
      var rc = db.exec( "select * from " + csName + "." + clName + " where age >= 0" );
      size = rc.size();
      if( size != num )
      {
         println( "Count find size: " + size + " is not same with " + num + ", message: " + message );
         throw "count find check failed"
      }
   }
   catch( e )
   {
      println( "Count find exception: " + e + ", message: " + message );
      throw e;
   }
}
