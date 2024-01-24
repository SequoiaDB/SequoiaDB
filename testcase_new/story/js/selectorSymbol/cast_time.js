/************************************
*@Description: use cast:{field:<$cast:Value>},
               cast numberic to timestamp
*@author:      zhaoyu
*@createdate:  2016.8.10
*@testlinkCase:
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ a: -2147483648 },
   { a: 2147483647 },
   { a: 0 },
   { a: { $numberLong: "-2147483647000" } },
   { a: { $numberLong: "2147483647000" } },
   { a: { $decimal: "-2147483647000" } },
   { a: { $decimal: "2147483647000" } },
   { a: -2147483648000.48 },
   { a: 2147483647000.48 }];;
   dbcl.insert( doc );

   //source type:all,destination type:all,for string express,lower letters
   var selectCondition1 = { a: { $cast: "timestamp" } };
   var expRecs1 = [{ a: -2147483648 },
   { a: 2147483647 },
   { a: 0 },
   { a: -2147483647 },
   { a: 2147483647 },
   { a: -2147483647 },
   { a: 2147483647 },
   { a: -2147483648 },
   { a: 2147483647 }];

   var timeStampexpRecs1 = castNumbericToTimeStamp( expRecs1 );
   checkResult( dbcl, null, selectCondition1, timeStampexpRecs1, { _id: 1 } );
}

/************************************
*@Description: change a number to timestamp 
*@author:      zhaoyu
*@createDate:  2015.8.10
**************************************/
function castNumbericToTimeStamp ( arr )
{
   var getTimeArr = [];
   var cmd = new Cmd();
   for( var i in arr )
   {
      castTime = cmd.run( 'date -d@"' + arr[i].a + '" +"%Y-%m-%d-%H.%M.%S.000000"' ).replace( /(^\s*)|(\s*$)/g, "" );
      var castStamptime = {};
      castStamptime = { $timestamp: castTime };
      arr[i] = { a: castStamptime };
   }
   return arr;
}
