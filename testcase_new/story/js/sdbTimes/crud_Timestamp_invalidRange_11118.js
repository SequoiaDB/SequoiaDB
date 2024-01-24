/************************************************************************
*@Description:  Timestamp, invalid range
*testcases:        seqDB-11118、seqDB-11119、seqDB-11120、seqDB-11121
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11118_2";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   var rawData = ["1901-12-13-00.00.00.000000", //0
      "2038-01-20-00.00.00.000000",
      "1901-12-13T00:00:00.000Z",
      "2038-01-19T15:00:00.000Z",
      "1901-12-14T00:00:00.000+0800",
      "2038-01-19T15:00:00.000+0800", //5
      -2147483649,
      2147483648]
   testSdbDate( rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function testSdbDate ( rawData )
{

   for( i = 0; i < rawData.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         if( i < rawData.length - 2 )
         {
            Timestamp( rawData[i] );
         }
         else
         {
            Timestamp( rawData[i], 0 );
         }
      } );
   }
}
