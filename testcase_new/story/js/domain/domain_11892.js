/******************************************************************************
@Description : Test create 10 domain,the domains' group is equal with each other.
@Modify list :
               2014-6-24  xiaojun Hu  Init
			   seqDB-11892
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = new Array();
   var csname = new Array();
   var clname = new Array();
   var name = csName + "_11892";
   for( var i = 0; i < 10; ++i )
   {
      domName[i] = name + i;
      csname[i] = csName + i;
      clname[i] = clName + i;

      commDropCS( db, csname[i], true );
      clearDomain( db, domName[i] );
   }

   var group = getGroup( db );
   for( var i = 0; i < 10; ++i )
   {
      db.createDomain( domName[i], group );
   }

   // inspect domain and create CS/CL
   for( var i = 0; i < 10; ++i )
   {
      commCreateCS( db, csname[i], false, "create CS specify domain",
         { "Domain": domName[i] } );
      commCreateCL( db, csname[i], clname[i], {}, false, false );
   }

   for( var i = 0; i < 10; ++i )
   {
      // Insert data
      insertData( db, csname[i], clname[i], 1000 );

      // Query data
      queryData( db, csname[i], clname[i] );

      // Update data
      updateData( db, csname[i], clname[i] );

      // Remove data
      removeData( db, csname[i], clname[i] );

      // Clear domain in the end and drop cs
      clearDomain( db, domName[i] );
   }
}