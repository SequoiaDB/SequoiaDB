/******************************************************************************
@Description :   seqDB-16012:  修改自增字段名
@Modify list :   2018-10-22    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16012";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   //insert records and check
   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      for( var j = 0; j < 1000; j++ )
      {
         cl.insert( { "a": j } );
         expRecs.push( { "a": j, "id1": 1 + j + i * 1000 } );
      }
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   expRecs.sort( compare( "id1" ) );
   checkRec( rc, expRecs );

   assert.tryThrow( SDB_AUTOINCREMENT_FIELD_NOT_EXIST, function()
   {
      dbcl.setAttributes( { AutoIncrement: { Field: "id2" } } );
   } );

   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i } );
      expRecs.push( { "a": i, "id1": coordNodes.length * 1000 + 1 + i * 1000 } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   expRecs.sort( compare( "id1" ) )
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

