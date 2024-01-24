/******************************************************************************
 * @Description   : seqDB-24896:会话在多个节点开启上下文超过限制 
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.10
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clOpt = { "ShardingKey": { "no": 1 }, "ShardingType": "range" };
testConf.clName = COMMCLNAME + "_context_24886";
testConf.useSrcGroup = true
testConf.useDstGroup = true

main( test );
function test ( testPara )
{
   var varCL = testPara.testCL;
   var maxSessioncontextnum = 100;
   var config = { "maxsessioncontextnum": maxSessioncontextnum };
   db.updateConf( config );
   var dataNum = 5000;
   insertData( varCL, 5000 );
   varCL.split( testPara.srcGroupName, testPara.dstGroupNames[0], { "no": dataNum / 2 }, { "no": dataNum } );

   // 查询覆盖源组和目标组数据，打开上下文
   openContext( 1000, 4000, testConf.clName, maxSessioncontextnum );

   db.deleteConf( { "maxsessioncontextnum": 1 } );
}

function listContexts ( db )
{
   var cursor = db.list( SDB_LIST_CONTEXTS, {}, { "Contexts": { "$include": 0 } } );
   var actListInof = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      actListInof.push( obj );
   }
   cursor.close();
   println( "---actList=" + JSON.stringify( actListInof ) );
}

function openContext ( beginNo, endNo, clName, maxSessioncontextnum )
{
   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var dbcl = db1.getCS( COMMCSNAME ).getCL( clName );
      for( var j = 0; j < maxSessioncontextnum; j++ )
      {
         var cursor = dbcl.find( { $and: [{ a: { $gte: beginNo } }, { a: { $lt: endNo } }] } );
         var obj = cursor.next();
      }
      assert.tryThrow( SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT, function()
      {
         var cursor = dbcl.find( { $and: [{ no: { $gte: beginNo } }, { no: { $lt: endNo } }] } );
         var obj = cursor.next();
         listContexts( db1 );
      } )
   } finally
   {
      db1.close();
   }
}