/******************************************************************************
@Description : seqDB-15555:同时修改AutoIndexId及AutpSplit
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = CHANGEDPREFIX + "_split15555";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoIndexId": false };

main( test );
function test ( arg )
{
   var cl = arg.testCL;

   // alter cl, [true, true]
   cl.alter( { "AutoIndexId": true, "AutoSplit": true } );

   // alter cl again, [false, false]
   try
   {
      cl.alter( { "AutoIndexId": false, "AutoSplit": false } );
      throw new Error( "expected failure, actual return success." );
   }
   catch( e )
   {
      if( SDB_OPTION_NOT_SUPPORT != e.message )
      {
         throw e;
      }
   }

   // alter cl again, [true, false]
   try
   {
      cl.alter( { "AutoIndexId": true, "AutoSplit": false } );
      throw new Error( "expected failure, actual return success." );
   }
   catch( e )
   {
      if( SDB_OPTION_NOT_SUPPORT != e.message )
      {
         throw e;
      }
   }

   // alter cl again, [false, true]
   try
   {
      cl.alter( { "AutoIndexId": false, "AutoSplit": true } );
      throw new Error( "expected failure, actual return success." );
   }
   catch( e )
   {
      if( SDB_OPTION_NOT_SUPPORT != e.message )
      {
         throw e;
      }
   }
}
