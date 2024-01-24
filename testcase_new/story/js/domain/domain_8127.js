/******************************************************************************
@Description : 1. Test db.listDomains(<name>,[option]), list all domains.
               2. Test insert/update/find/remove operation.
               3. Test create four domains.
               4. Test listDomains(), where domain name is no exist.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domNames = new Array( CHANGEDPREFIX + "DomTest1", CHANGEDPREFIX + "DomTest2",
      CHANGEDPREFIX + "DomTest3", CHANGEDPREFIX + "DomTest4" );

   // Drop all domains, if have
   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
   }

   var group = getGroup( db );
   for( var i = 0; i < domNames.length; ++i )
   {
      db.createDomain( domNames[i], group );
   }

   // List not exit domain name [Testing Point]
   assert.tryThrow( SDB_DMS_EOC, function()
   {
      var listDom = db.listDomains( { "Name": "NotExistDomName" } );
      var notDomName = listDom.current().toObj()["Name"];
   } );

   // List all domains and inspect[Testing Point]
   listDom = db.listDomains();
   listDomArray = new Array();
   while( listDom.next() )
   {
      listDomArray.push( listDom.current().toObj()["Name"] );
   }
   // Inspect the domains
   for( var i = 0; i < domNames.length; ++i )
   {
      for( var j = 0; j <= listDomArray.length; ++j )
      {
         if( listDomArray[j] == domNames[i] )
         {
            break;
         }
         if( j == listDomArray.length )
         {
            throw new Error( "ErrDomains" );
         }
      }
   }

   for( var i = 0; i < domNames.length; ++i )
   {
      // Drop domain int the end
      clearDomain( db, domNames[i] );
   }
}