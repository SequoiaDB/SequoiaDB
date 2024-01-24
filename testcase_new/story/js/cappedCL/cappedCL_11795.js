/************************************
*@Description:capped cl, repeate insert and pop, direciton set 1 
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11795
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11795";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   var repeatedTimes = 10;
   var minLength = 0;
   var maxLength = 2048;
   var string = "a";
   repeatedInsertAndPopLastRecord( dbcl, repeatedTimes, minLength, maxLength, string );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function repeatedInsertAndPopLastRecord ( dbcl, repeatedTimes, minLength, maxLength, string )
{
   var preLogicalID = 0;
   //repeat pop and insert,check LogicalID is the same
   for( var i = 1; i < repeatedTimes; i++ )
   {
      //insert record;
      var doc = new StringBuffer();
      var range = maxLength - minLength;
      var stringLength = minLength + parseInt( Math.random() * range );
      doc.append( stringLength, string );
      var strings = doc.toString();
      var recs = [{ a: strings }];
      dbcl.insert( recs );
      var record = dbcl.find().sort( { _id: -1 } ).limit( 1 );
      while( record.next() )
      {
         //check logicalID
         var logicalID = record.current().toObj()._id;
         var expectLogicalID = preLogicalID;
         assert.equal( logicalID, expectLogicalID );

         //check record
         checkRecords( dbcl, null, null, null, null, null, recs );
      }

      var recordLength = stringLength + recordHeader;
      if( recordLength % 4 !== 0 )
      {
         recordLength = recordLength + ( 4 - recordLength % 4 );

      }
      preLogicalID = logicalID + recordLength;

      dbcl.pop( { LogicalID: logicalID, Direction: 1 } );
   }
} 
