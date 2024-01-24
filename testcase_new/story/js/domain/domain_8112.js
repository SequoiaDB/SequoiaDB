/******************************************************************************
@Description : Test create 5 domain, specify the five group and is not equal.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var group = getGroup( db );
   if( group.length < 3 )
   {
      return;
   }
   // Domain names
   var name = csName + "_8112";
   var domNames = new Array();
   var csname = new Array();
   var clname = new Array();
   for( var i = 0; i < 3; ++i )
   {
      domNames[i] = name + i;
      csname[i] = csName + i;
      clname[i] = clName + i;

      commDropCS( db, csname[i], true );
      clearDomain( db, domNames[i] );
   }

   // create 5 domain and group is not equal with each other [Testing Point]
   for( var i = 0; i < 3; ++i )
   {
      commCreateDomain( db, domNames[i], [group[i]] );
   }

   // Inspect domain and create CS/CL
   for( var i = 0; i < 3; ++i )
   {
      // Create CS in domain and create collection [Testing Point]
      commCreateCS( db, csname[i], false, "create CS specify domain",
         { "Domain": domNames[i] } );
      commCreateCL( db, csname[i], clname[i], {}, false, false );
   }

   for( var i = 0; i < 3; ++i )
   {
      // Insert data
      insertData( db, csname[i], clname[i], 2000 );

      // Query data
      queryData( db, csname[i], clname[i] );

      // Update data
      updateData( db, csname[i], clname[i] );

      // Remove data
      removeData( db, csname[i], clname[i] );

      // Drop domain in the end
      clearDomain( db, domNames[i] );
   }
}