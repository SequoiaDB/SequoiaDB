/******************************************************************************
*@Description : seqDB-11086 _id中字段名不满足格式
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_11086";
   var cl = readyCL( clName );
   insertRecordsWithIllegalFieldName( cl );
   var actRecords = cl.find();
   var expRecords = [];
   commCompareResults( actRecords, expRecords );
   commDropCL( db, COMMCSNAME, clName, false, false, "Failed to drop CL in the end-condition" );
}

function insertRecordsWithIllegalFieldName ( cl )
{
   var illegalFieldName = ["$a", "a.b"];
   for( var i = 0; i < illegalFieldName.length; i++ )
   {
      try
      {
         var fieldName = illegalFieldName[i];
         var obj = {};
         obj[fieldName] = "test" + i;
         cl.insert( { "_id": obj } );
         throw new Error( "need throw error" );
      }
      catch( e )   
      {
         if( SDB_INVALIDARG != e.message )
         {
            throw e;
         }
      }
   }
}
