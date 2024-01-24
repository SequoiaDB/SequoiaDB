/******************************************************************************
*@Description : seqDB-11375:查询并排序，且排序为外排的索引选择 
*@author      : Li Yuanyue
*@Date        : 2020.5.8
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11375_cl";

main( test );

function test ( args )
{
   var idxNamea = "index_a_11375";
   var idxNameb = "index_b_11375";
   var tbIdx = "";
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var cl = args.testCL;

   cl.createIndex( idxNamea, { a: 1 } );
   cl.createIndex( idxNameb, { b: -1 } );

   var field = new Array( 1024 * 10 ).toString();

   try
   {
      db.updateConf( { sortbuf: 128 } );

      // 灌 128MB 数据
      var rd = new commDataGenerator();
      for( var i = 0; i < 14; i++ )
      {
         var value = rd.getRecords( 1000, "int", ["a", "b", field] );
         cl.insert( value );
      }

      db.analyze( { Collection: fullclName } );

      var cond = {};
      var expIndexName = tbIdx;
      var expScanType = "tbscan";
      var sortCond = { "a": 1, "b": -1 };
      checkExplain( cl, cond, expIndexName, expScanType, sortCond );
   }
   finally
   {
      db.updateConf( { sortbuf: 256 } );
   }
}
