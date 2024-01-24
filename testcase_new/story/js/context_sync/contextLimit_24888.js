/******************************************************************************
 * @Description   : seqDB-24887:maxsessioncontextnum配置参数校验
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.04.29
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_context_24887";

main( test );
function test ( testPara )
{
   var defaultMaxSessionContextNum = 100;
   var defaulConfig = { "maxsessioncontextnum": defaultMaxSessionContextNum };
   var maxConfig = { "maxsessioncontextnum": 0 };
   var minConfig = { "maxsessioncontextnum": 10 };

   var varCL = testPara.testCL;
   insertData( varCL, 10000 );
   try
   {
      // 合法参数11、10、2^31-1
      var maxSessionContextnum = 11;
      var config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxSessionContextnum = 10;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxSessionContextnum = 2147483647;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxSessionContextnum = 0;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      // 非法参数“10”、9、2^31、-10
      maxSessionContextnum = "10";
      config = { "maxsessioncontextnum": maxSessionContextnum };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } )
      checkConf( db, maxConfig );

      maxSessionContextnum = 9;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, minConfig );

      maxSessionContextnum = -10;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      checkConf( db, maxConfig );

      maxSessionContextnum = 2147483648;
      config = { "maxsessioncontextnum": maxSessionContextnum };
      db.updateConf( config );
      if( commIsArmArchitecture() == true )
      {
         maxConfig = { "maxsessioncontextnum": 2147483647 };
      }
      checkConf( db, maxConfig );
   }
   finally 
   {
      db.deleteConf( { "maxsessioncontextnum": 1 } );
   }
}

function checkConf ( db, configs )
{
   var snapshotInfo = db.snapshot( SDB_SNAP_CONFIGS ).next().toObj();
   for( var key in configs )
   {
      if( configs[key].toString().toUpperCase() !== snapshotInfo[key].toString().toUpperCase() )
      {
         throw new Error( "The expected result is " + configs[key].toString().toUpperCase() + ", but the actual resutl is " +
            snapshotInfo[key].toString().toUpperCase() );
      }
   }
}

function openContexts ()
{
   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var cl1 = db1.getCS( testConf.csName ).getCL( testConf.clName );
      for( var j = 0; j < 10; j++ )
      {
         var cursor = cl1.find();
         var obj = cursor.next();
      }

   } finally
   {
      db1.close();
   }
}