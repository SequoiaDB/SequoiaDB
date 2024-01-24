/******************************************************************************
@Description :   seqDB-16020:  修改Generated属性值  
@Modify list :   2018-10-23    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16020";
   var acquireSize = 10;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1", AcquireSize: acquireSize } } );

   var doc = [{ "a": 1 }, { "a": 2, "id1": 1 }];
   dbcl.insert( doc );

   var rc = dbcl.find().sort( { "id1": 1 } );
   var expRecs = [{ "a": 1, "id1": 1 }, { "a": 2, "id1": 1 }];
   checkRec( rc, expRecs );

   //alter Generated always
   dbcl.setAttributes( { AutoIncrement: { Field: "id1", Generated: "always" } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement[0].Generated !== "always" )
   {
      throw new Error( "alter failed!" );
   }

   //insert records and check
   var coordNodes = getCoordNodeNames( db );
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i, "b": i, "id1": i } );
      expRecs.push( { "a": i, "b": i, "id1": i * acquireSize + 11 } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   //alter Generated strict
   dbcl.setAttributes( { AutoIncrement: { Field: "id1", Generated: "strict" } } );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement[0].Generated !== "strict" )
   {
      throw new Error( "alter failed!" );
   }

   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.insert( { "id1": "a" + i } );
      } );

      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   //alter Generated default
   dbcl.setAttributes( { AutoIncrement: { Field: "id1", Generated: "default" } } );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement[0].Generated !== "default" )
   {
      throw new Error( "alter failed!" );
   }

   dbcl.truncate();
   dbcl.insert( { "a": "a", "id1": 50 } );
   dbcl.insert( { "a": "a1" } );

   var rc = dbcl.find().sort( { "id1": 1 } );
   var expRecs = new Array();
   expRecs.push( { "a": "a", "id1": 50 } );
   expRecs.push( { "a": "a1", "id1": 51 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
