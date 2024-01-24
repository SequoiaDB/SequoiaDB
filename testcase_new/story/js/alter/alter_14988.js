/* *****************************************************************************
@discretion: cl alter strictDataMode
@author��2018-4-26 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclstrictDataMode_14988";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   dbcl.insert( { no: { '$numberLong': '-9223372036854775808' }, a: 1 } );

   //alter cl strictDataMode is true, numoverflow error
   var strictDataMode = true;
   dbcl.setAttributes( { StrictDataMode: strictDataMode } );
   checkResult( clName, "AttributeDesc", "Compressed | StrictDataMode" );
   numoverflowError( dbcl );

   //alter cl strictDataMode is false, numoverflow error
   var strictDataMode1 = false;
   dbcl.setAttributes( { StrictDataMode: strictDataMode1 } );
   checkResult( clName, "AttributeDesc", "Compressed" );
   numoverflowConversion( dbcl )

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}

function numoverflowError ( dbcl )
{
   try
   {
      var rc = dbcl.find( {}, { "no": { "$abs": 1 } } ).next();
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_VALUE_OVERFLOW )
      {
         throw e;
      }
   }
}

function numoverflowConversion ( dbcl )
{
   var rc = dbcl.find( {}, { "no": { "$abs": 1 }, "_id": { "$include": 0 } } );
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   var expRecs = [];
   expRecs.push( { "no": { "$decimal": "9223372036854775808" }, "a": 1 } );
   assert.equal( actRecs, expRecs );
}

function checkResult ( clName, fieldName, expFieldValue )
{
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];
   assert.equal( expFieldValue, actualFieldValue );

}