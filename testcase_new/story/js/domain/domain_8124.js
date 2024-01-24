/******************************************************************************
@Description : 1. Test db.dropDomains(<name>), specify name .
               2. Test create four domains.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   domname1 = "DomTest_1_8124" + COMMCLNAME;
   domname2 = "DomTest_2_8124" + COMMCLNAME;
   domname3 = "DomTest_3_8124" + COMMCLNAME;
   domname4 = "DomTest_4_8124" + COMMCLNAME;

   domNames = new Array( domname1, domname2, domname3, domname4 );

   // Drop all domains, if have
   for( var i = 0; i < domNames.length; ++i )
   {
      clearDomain( db, domNames[i] );
   }

   var group = getGroup( db );
   //println( "Get Groups = " + group ) ;
   for( var i = 0; i < domNames.length; ++i )
   {
      db.createDomain( domNames[i], group );
   }

   for( var i = 0; i < domNames.length; ++i )
   {
      // Drop domains that were created
      db.dropDomain( domNames[i] );
   }
}