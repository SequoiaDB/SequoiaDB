/******************************************************************************
@Description : 1. Test dom.alter(<options>), specify alter Groups.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8119";

   clearDomain( db, domName );

   // Alter the group to domain [Testing Point]
   var group = getGroup( db );

   // Create domain without group and autosplit
   commCreateDomain( db, domName );

   dom = db.getDomain( domName );

   dom.alter( { Groups: group } );

   // inspect the alter
   for( var i = 0; i < group.length; ++i )
   {
      inspectAlter( db, group[i], domName );
   }

   // Clear domain in the end
   clearDomain( db, domName );
}

function inspectAlter ( db, group, domName )
{

   var i = 0;
   do
   {
      // Using { "Name" : domName } is Wrong
      var listDom = db.listDomains( { Name: domName } );
      var groupArray = new Array();
      while( listDom.next() )
      {
         groupArray.push( listDom.current().toObj()["Groups"][i]["GroupName"] );
      }

      // "GroupName" just have one
      if( group == groupArray )
      {
         break;
      }
      if( groupArray.length == 0 )
      {
         throw new Error( "NoDomainGroup" );
      }
      ++i; // addselt 1
   } while( i < 100 );
}