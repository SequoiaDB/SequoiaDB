/*******************************************************************************
*@Description : Aggregate skip test
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   testSkipFollowMinBound( cl, docNumber );
   testSkipMinBound( cl, docNumber );
   testSkipAnyValue( cl );
   testSkipMaxBound( cl );

   cl.drop();
}
function testSkipFollowMinBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $skip: -1 } );
   var retNumber = getRetNumber( cursor );
   assert.equal( retNumber, docNumber );
}

function testSkipMinBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $skip: 0 } );
   var retNumber = getRetNumber( cursor );
   assert.equal( retNumber, docNumber );

}

function testSkipAnyValue ( cl )
{
   cursor = cl.execAggregate( { $skip: 10 } );
   retNumber = getRetNumber( cursor );
   assert.equal( retNumber, 7 );

}

function testSkipMaxBound ( cl )
{
   var cursor = cl.execAggregate( { $skip: 17 } );
   var retNumber = getRetNumber( cursor );
   assert.equal( retNumber, 0 );
}
