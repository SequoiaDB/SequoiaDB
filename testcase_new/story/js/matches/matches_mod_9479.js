/******************************************************************************
@Description : jira1913 seqDB-9479:取模运算
@author :Shitong Wen
               
******************************************************************************/

main( test );

function test ()
{

   //create cs cl
   var csName = "cs_9479";
   var clName = "cl_9479";
   // Drop CS
   commDropCS( db, csName, true );

   //create cs and cl      
   commCreateCL( db, csName, clName, {}, true, false, "create cs and cl" );

   var cl = db.getCS( csName ).getCL( clName );
   cl.remove();
   var recs = [];

   recs.push( { a: -1E+40 } );
   recs.push( { a: { $numberLong: "-9223372036854775808" } } );
   recs.push( { a: -2147483648 } );

   cl.insert( recs );
   checkResult( cl, recs );
   //clean environment
   commDropCS( db, csName, true );

}
//check the result;
function checkResult ( cl, recs )
{
   var findParame = { a: { $mod: [{ $numberLong: "-2147483648" }, 0] } };
   var cursor = cl.find( findParame ).sort( { a: 1 } );
   var i = 0;

   while( cursor.next() )
   {
      if( cursor.current().toObj()["a"]["$numberLong"] == undefined )
      {
         assert.equal( cursor.current().toObj()["a"], recs[i]["a"] );

      } else
      {
         assert.equal( cursor.current().toObj()["a"]["$numberLong"], recs[i]["a"] );

      }
      i++;
   }

   assert.equal( i, 3 );
}
