/******************************************************************************
*@Description : seqDB-6710:插入binary类型$type非法_ST.basicOperate.insert.03.001
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_6710";
   var cl = readyCL( clName );

   //test $type is 256
   insertWithTypeErrorA( cl );
   //test $type is -1  
   insertWithTypeErrorB( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function insertWithTypeErrorA ( cl )
{
   try
   {
      var binary = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "256" };
      cl.insert( { binary: binary } );
      throw new Error( "need throw error" );
   }
   catch( e )   
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }

   var expCount = 0;
   var count = cl.count();
   if( Number( expCount ) !== Number( count ) )
   {
      throw new Error( "expCount: " + expCount + "\ncount: " + count );
   }
}

function insertWithTypeErrorB ( cl )
{
   var binary = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "-1" };
   cl.insert( { binary: binary } );

   var expBinaryValue = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" };
   var expRecords = [];
   expRecords.push( { binary: expBinaryValue } );

   var actRecords = cl.find( {}, { "_id": { "$include": 0 } } );
   commCompareResults( actRecords, expRecords, false );
}
