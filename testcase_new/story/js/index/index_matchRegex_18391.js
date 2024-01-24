/************************************************************************
@Description : create index, query with regex with "|"  
@Modify list :
               2019-6-5  wuyan
************************************************************************/

main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_indexcl18391";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   insertRecords( dbcl );
   dbcl.createIndex( "idxa", { 'test': 1 } );

   var condition1 = { test: { $regex: '^1234$|^4567$' } };
   var expRecs1 = [{ "test": "1234" }, { "test": "4567" }];
   queryAndCheckResult( dbcl, condition1, expRecs1 );

   var condition2 = { test: { $regex: '^1234\\|$|^\\|4567$' } };
   var expRecs2 = [{ "test": "1234|" }, { "test": "|4567" }];
   queryAndCheckResult( dbcl, condition2, expRecs2 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the ending" );

}

function queryAndCheckResult ( dbcl, condition, expRecs )
{
   var rc = dbcl.find( condition, { "_id": { "$include": 0 } } ).hint( { "": "idxa" } );
   checkRec( rc, expRecs );
}

function insertRecords ( cl )
{
   var values = ["1234", "|4567", "4567", "1234|", "atest1", 1234, "atest3"];
   var docs = [];
   for( var i = 0; i < values.length; ++i )
   {
      var value = values[i];
      var objs = { "test": value };
      docs.push( objs );
   }
   cl.insert( docs );
}