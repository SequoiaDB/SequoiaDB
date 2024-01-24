/******************************************************************************
 * @Description   : seqDB-24887:maxcontextnum配置参数校验
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.03.15
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_context_24887";

main( test );
function test ( testPara )
{
   var defaultMaxContextnum = 100000;
   var defaulConfig = { "maxcontextnum": defaultMaxContextnum };
   var maxConfig = { "maxcontextnum": 0 };
   var minConfig = { "maxcontextnum": 1000 };

   var varCL = testPara.testCL;
   insertData( varCL, 10000 );
   try
   {
      // 合法参数1001、1000、2^31-1
      var maxContextnum = 1001;
      var config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxContextnum = 1000;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxContextnum = 2147483647;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      maxContextnum = 0;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, config );
      openContexts()

      // 非法参数“1000”、999、2^31、-1000
      db.deleteConf( { "maxcontextnum": 1 } );
      maxContextnum = "1000";
      config = { "maxcontextnum": maxContextnum };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } )
      checkConf( db, defaulConfig );

      maxContextnum = 999;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, minConfig );

      maxContextnum = -1000;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      checkConf( db, maxConfig );

      maxContextnum = 2147483648;
      config = { "maxcontextnum": maxContextnum };
      db.updateConf( config );
      if( commIsArmArchitecture() == true )
      {
         maxConfig = { "maxcontextnum": 2147483647 };
      }
      checkConf( db, maxConfig );
   }
   finally
   {
      db.deleteConf( { "maxcontextnum": 1 } );
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
   var sdbs = [];
   try
   {
      for( var i = 0; i < 9; i++ )
      {
         var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
         sdbs.push( db1 );
         var cl1 = db1.getCS( testConf.csName ).getCL( testConf.clName );
         for( var j = 0; j < 100; j++ )
         {
            var cursor = cl1.find();
            var obj = cursor.next();
         }
      }
   } finally
   {
      if( sdbs.length !== 0 )
      {
         for( var i in sdbs )
         {
            var db1 = sdbs[i];
            db1.close();
         }
      }
   }
}