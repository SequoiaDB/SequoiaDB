/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), before group have data.
               2. Test the data/idx in domain groups.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_DomDataGroupData";
   commDropCS( db, csName, true );
   clearDomain( db, domName );

   // Data operation before create domain [Testing Point 1]
   commCreateCS( db, csName, false, "create CS specify domain" );

   // Create Collection
   commCreateCL( db, csName, clName, {}, false, false );

   // Create index
   var cs = db.getCS( csName );
   var cl = cs.getCL( clName );
   cl.createIndex( "befDomIdx", { "No": 1 }, true, true );

   // Insert data
   insertData( db, csName, clName, 1000 );

   // Query data
   queryData( db, csName, clName );

   // Update data
   updateData( db, csName, clName );

   // Remove data
   removeData( db, csName, clName );

   // Get collection space located in group
   var csGroup = getCSGroup( db, csName, clName );

   // Create Domain specify this Group and inspect
   db.createDomain( domName, [csGroup] );
   //db.createDomain( domName, [ "group1" ] ) ;

   // After create domain, create collection space and collection
   var csname = csName + "_DomAfter";
   var clname = clName + "_DomAfter";

   commCreateCS( db, csname, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csname, clname, {}, false, false );

   // Create index
   var CS = db.getCS( csname );
   var CL = CS.getCL( clname );
   CL.createIndex( "aftDomIdx", { "cardID": -1 } );

   // Insert data
   insertData( db, csname, clname, 1000 );

   // Query data
   queryData( db, csname, clname );

   // Update data
   updateData( db, csname, clname );

   // Remove data
   removeData( db, csname, clname );

   clearDomain( db, domName );
   commDropCS( db, csName, true );
}