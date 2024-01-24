/******************************************************************************
@Description : 1. Test dom.alter(<options>), specify {AutoSplit:true}.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8120";
   // Clear domain in the beginning
   clearDomain( db, domName );
   commDropCS( db, csName, true );

   // Create domain without group and autosplit
   commCreateDomain( db, domName );

   var group = new Array();
   group = getGroup( db );
   dom = db.getDomain( domName );
   dom.alter( { "Groups": group, AutoSplit: true } );

   // Create collection space and collection
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, { ShardingKey: { "No": -1 }, ShardingType: "hash", Partition: 1024 }, false, false, "create collection in domain" );

   // Inspect data to SDB
   insertData( db, csName, clName, 1000 )

   // inspect the AutoSplit is take effect or not
   inspectAutoSplit( db, csName, clName, domName )

   // Clear domain in the end
   clearDomain( db, domName );
}