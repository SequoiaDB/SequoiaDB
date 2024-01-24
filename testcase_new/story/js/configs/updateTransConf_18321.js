/******************************************************************************
 * @Description : the connection takes effect immediately after the transaction configuration is updated
                  seqDB-18321:更改事务配置后连接立即生效可用
 * @author      : luweikang
 * @date        ：2019.04.04
 ******************************************************************************/

var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   try
   {
      var snaphostCur = db.snapshot( 13, { "svcname": COORDSVCNAME.toString() } );
      var obj = snaphostCur.next().toObj();
      snaphostCur.close()
      if( obj.transisolation != 0 )
      {
         db.updateConf( { "transisolation": 0 }, { Global: false } );
      }

      var csName = COMMCSNAME;
      var clName = "cl_18321";
      var r1 = { "_id": 1, "a": 1 };
      var r2 = { "_id": 2, "a": 2 }

      var cl1 = commCreateCL( db1, csName, clName );
      var cl2 = db2.getCS( csName ).getCL( clName );

      db1.transBegin();
      db2.transBegin();

      cl1.insert( r1 );
      var cursor = cl2.find( { "a": 1 } );
      var record = cursor.next();
      cursor.close();
      db1.transCommit();
      db2.transCommit();

      if( JSON.stringify( r1 ) != JSON.stringify( record.toObj() ) )
      {
         throw new Error( "r1: " + JSON.stringify( r1 ) + ", record: " + JSON.stringify( record.toObj() ) );
      }

      db.updateConf( { "transisolation": 1 }, { Global: false } );

      db1.transBegin();
      db2.transBegin();

      cl1.insert( r2 );
      var cursor = cl2.find( { "a": 2 } );
      if( cursor.next() )
      {
         throw new Error( "record: " + cursor.current().toString() );
      }
      cursor.close();

      db1.transCommit();

      var cursor = cl2.find( { "a": 2 } );
      var record = cursor.next();
      cursor.close();
      db2.transCommit();
      if( JSON.stringify( r2 ) != JSON.stringify( record.toObj() ) )
      {
         throw new Error( "r2: " + JSON.stringify( r2 ) + ", record: " + JSON.stringify( record.toObj() ) );
      }

      db.getCS( csName ).dropCL( clName );
   } finally
   {
      db1.transCommit();
      db2.transCommit();
      db1.close();
      db2.close();
      db.deleteConf( { "transisolation": 1 }, { Global: false } );
   }
}
