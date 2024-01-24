/******************************************************************************
@Description : 1. Test before create domain range/percent split and AutoSplit.
               2. Test insert/update/find/remove operation.
@Modify list :
               2014-6-25  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_DomRangeHashAutoSplit";
   var rangeCL = clName + "_Range";
   var commonCL = clName + "_Common";

   commDropCS( db, csName, true );
   clearDomain( db, domName );

   /***************************************************************************
   @ Begin to create domain and do AutoSplit operation
   ***************************************************************************/
   // Get all data groups and create domain by specify AutoSplit
   var group = getGroup( db );
   commCreateDomain( db, domName, group, { "AutoSplit": true } );

   // Create CS in domain and create collection

   commCreateCS( db, csName, false, "create CS specify domain", { "Domain": domName } );
   commCreateCL( db, csName, rangeCL, { ShardingKey: { "No": -1 }, ShardingType: "range", Partition: 1024 }, false, false, "create collection in domain" );
   // Comman CL
   commCreateCL( db, csName, commonCL, { ShardingKey: { "No": -1 } }, false, false, "create collection in domain" );

   // Insert data
   insertData( db, csName, rangeCL, 1000 );
   insertData( db, csName, commonCL, 1000 );

   // inspect the AutoSplit is take effect or not
   inspectAutoSplit( db, csName, rangeCL, domName );
   inspectAutoSplit( db, csName, commonCL, domName );

   // Query data
   queryData( db, csName, rangeCL );
   queryData( db, csName, commonCL );

   // Update data
   updateData( db, csName, rangeCL );
   updateData( db, csName, commonCL );

   // Remove data
   removeData( db, csName, rangeCL );
   removeData( db, csName, commonCL );

   clearDomain( db, domName );
}