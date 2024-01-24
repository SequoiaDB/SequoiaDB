/******************************************************************************
*@Description : seqDB-4950:字段名不满足格式_ST.basicOperate.insert.02.006
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/

// SEQUOIADBMAINSTREAM-1172
// main( test );

function test ()
{
   var clName = COMMCLNAME + "_4950";
   var cl = readyCL( clName );
   insertRecordsWithIllegalFieldName( cl );
   var actRecords = cl.find();
   var expRecords = [];
   commCompareResults( actRecords, expRecords, false );
   commDropCL( db, COMMCSNAME, clName );
}

function insertRecordsWithIllegalFieldName ( cl )
{
   var illegalFieldName = ["$a", "a.b", ""];
   for( var i = 0; i < illegalFieldName.length; i++ )
   {
      try
      {
         var fieldName = illegalFieldName[i];
         var obj = {};
         obj[fieldName] = "test" + i;
         cl.insert( obj );
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
