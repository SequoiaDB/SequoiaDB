/******************************************************************************
@Description :   seqDB-15980:  truncate集合  
@Modify list :   2018-10-15  xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15980";
   var fields = ["id1", "a.a"];

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: [{ Field: fields[0] }, { Field: fields[1] }] } );

   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "b": i } );
      expRecs.push( { "b": i, "id1": 1 + i * 1000, "a": { "a": 1 + i * 1000 } } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   dbcl.truncate();

   checkCurrentValue( db, COMMCSNAME, clName, fields );

   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "b": i } );
      expRecs.push( { "b": i, "id1": 1 + i * 1000, "a": { "a": 1 + i * 1000 } } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

function checkCurrentValue ( db, csName, clName, fields )
{
   var clID = getCLID( db, csName, clName );
   for( var i in fields )
   {
      var sequenceName = "SYS_" + clID + "_" + fields[i] + "_SEQ";
      var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
      var currentValue = cursor.current().toObj().CurrentValue;
      var startValue = cursor.current().toObj().StartValue;
      assert.equal( currentValue, startValue );
   }

}