/****************************************************************
@decription:   Deploy SequoiaDB

@input:        hosts:      String
               eg: bin/sdb -f checksdb.js -e 'var host="192.168.31.1"'

@author:       Yin Zhen 2019-09-24
****************************************************************/

// check parameter
if( typeof ( host ) === "undefined" )
{
   var host = "localhost";
}
else if( host.constructor !== String )
{
   throw "Invalid param[host], should be String";
}

// set global variable
var HOST = host;
var COORD_SVC = 11810;

// run!
main();

function main ()
{
   if( HOST !== "" )
   {
      checkSequoiadb();
   }
   else
   {
      println( "Do not check sdb, because of host: " + HOST );
   }
}

function checkSequoiadb ()
{
   var checkTimes = 0;
   while( checkTimes++ < 300 )
   {
      sleep( 1000 );
      try
      {
         var db = new Sdb( HOST + ":" + COORD_SVC );
         try
         {
            db.createCS( "testcs_check_sdb" );
         }
         catch( e )
         {
            if( e != -33 )
            {
               throw e;
            }
         }
         try
         {
            db.testcs_check_sdb.createCL( "testcl_check_sdb", { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 } );
         }
         catch( e )
         {
            if( e != -22 )
            {
               throw e;
            }
         }
         insertList = [];
         for( var i = 0; i < 1000; i++ )
         {
            insertList.push( { "a": i, b: i } );
         }
         db.testcs_check_sdb.testcl_check_sdb.insert( insertList );
         db.dropCS( "testcs_check_sdb" );
      }
      catch( e )
      {
         println( "Check SDB deploy ok throw: " + e );
         continue;
      }
      break;
   }
   if( checkTimes == 301 )
   {
      throw HOST + ":" + COORD_SVC + " Check Sdb fail";
   }
   println( HOST + ":" + COORD_SVC + " Check Sdb ok" );
}
