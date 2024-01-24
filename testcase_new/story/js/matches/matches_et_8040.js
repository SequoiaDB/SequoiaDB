/************************************************************************
*@Description:    seqDB-8040:使用$et查询，目标字段为非数值型，不走索引查询
                     data type: null/string/bool/oid/regex/date/timestamp
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8040";

   var cl = readyCL( clName );

   var dataType = ["null", "string", "bool", "oid", "regex", "date", "timestamp"];
   var rawData = [{ null: null },
   { string: "hello world" },
   { bool: true },
   { oid: { "$oid": "123abcd00ef12358902300ef" } },
   { regex: { "$regex": "^rg", "$options": "i" } },
   { date: { "$date": "2038-01-18" } },
   { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
   insertRecs( cl, rawData, dataType );

   var findRecsArray = findRecs( cl, rawData, dataType );

   checkResult( findRecsArray, dataType );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData, dataType )
{

   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( { a: i, b: rawData[i][dataType[i]] } );
   }
}

function findRecs ( cl, rawData, dataType )
{

   var j = 0;
   var findRecsArray = [];
   for( i = 0; i < rawData.length; i++ )
   {

      var rc = cl.find( { b: { $et: rawData[i][dataType[i]] } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );;
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, dataType )
{

   for( i = 0; i < findRecsArray.length; i++ )
   {

      var expLen = 1;
      assert.equal( findRecsArray[i].length, expLen );

      for( j = 0; j < findRecsArray[i].length; j++ )
      {
         if( findRecsArray[i][j]["a"] !== i )
         {
            throw new Error( "checkResult fail,[compare records]" + '[{"a": ' + i + '}]' + '[{"a": ' + findRecsArray[i][j]["a"] + '}]' );
         }
      }
   }
}