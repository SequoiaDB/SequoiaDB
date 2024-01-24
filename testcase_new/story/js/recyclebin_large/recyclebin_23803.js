/******************************************************************************
 * @Description   : seqDB-23803:CL开启lzw压缩，字典已构建，数据已压缩，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.20
 * @LastEditTime  : 2023.08.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23803";
   var clName = "cl_23803";
   var originName = csName + "." + clName;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { Compressed: true, CompressionType: "lzw", ReplSize: 0 } );
   var rgName = commGetCLGroups( db, originName );

   var number1 = 600000;
   var insertRecsNum = 1000000;
   var checkRecsNum = 10;

   // 插入大批量数据使构建字典
   insertRecs( dbcl, number1, insertRecsNum );

   // dropCL后恢复
   dbcs.dropCL( clName );
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 检查结果，检查组内每个节点数据正确性
   commCheckLSN( db );
   var dbcl = db.getCS( csName ).getCL( clName );
   checkLzwAttributeByDataNode( rgName, csName, clName );
   checkRecsByDataNode( dbcl, number1, insertRecsNum, checkRecsNum );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function insertRecs ( cl, number1, insertRecsNum )
{
   for( k = 0; k < number1; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( {
            total_account: i, account_id: i, tx_number: "test" + i,
            tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
         } )
      };
      cl.insert( doc );
   }

   for( k = number1; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( {
            total_exCount: i, exCount_id: i, tx_number: "testR" + i,
            tx_info: "hello/565bf18964f4f14fea84341b/image/20160101_1.png"
         } )
      };
      cl.insert( doc );
   }
}

function checkRecsByDataNode ( dbcl, number1, insertRecsNum, checkRecsNum )
{
   // 检查数据总数
   var recsCnt = dbcl.count();
   assert.equal( recsCnt, insertRecsNum );
   // 随机检查n条记录正确性
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      if( i < number1 )
      {
         var recsCnt = dbcl.find( {
            total_account: i, account_id: i, tx_number: "test" + i,
            tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
         } ).count();
      }
      else
      {
         var recsCnt = dbcl.find( {
            total_exCount: i, exCount_id: i, tx_number: "testR" + i,
            tx_info: "hello/565bf18964f4f14fea84341b/image/20160101_1.png"
         } ).count();
      }
      assert.equal( recsCnt, 1 );
   }
}

function checkLzwAttributeByDataNode ( rgName, csName, clName )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var clInfo = nodeDB.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         assert.equal( details.Attribute, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CompressionType, "lzw", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.DictionaryCreated, true, "clInfo = " + JSON.stringify( clInfo ) );
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}