/******************************************************************************
 * @Description   : seqDB-24807:验证锁升级配置的生效规则
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.16
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24807";

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbcs2 = db2.getCS( COMMCSNAME );
   var dbcl2 = dbcs2.getCL( testConf.clName );
   try
   {
      db.transBegin();
      db.updateConf( { "transallowlockescalation": false, "transmaxlocknum": 10, "transmaxlogspaceratio": 10 }, { Global: true } );
      var docs = insertData( dbcl, 1000 );
      db.transCommit();

      db.transBegin();
      assert.tryThrow( SDB_DPS_TRANS_LOCK_UP_TO_LIMIT, function()
      {
         insertData( dbcl, 1000 );
      } );
      db.transCommit();

      var actResult1 = dbcl.find().sort( { a: 1 } );
      commCompareResults( actResult1, docs );

      db2.transBegin();
      insertData( dbcl2, 10 );
      assert.tryThrow( SDB_DPS_TRANS_LOCK_UP_TO_LIMIT, function()
      {
         insertData( dbcl2, 1000 );
      } );
      db2.transCommit();

      var actResult2 = dbcl.find().sort( { a: 1 } );
      commCompareResults( actResult2, docs );
   } finally
   {
      db.deleteConf( { "transallowlockescalation": false, "transmaxlocknum": 10, "transmaxlogspaceratio": 10 }, { Global: true } )
      db2.close();
   }
}
