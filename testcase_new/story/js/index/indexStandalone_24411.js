/******************************************************************************
 * @Description   : seqDB-24411:备节点创建本地索引，创建/删除相同一致性索引    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2022.05.21
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.skipExistOneNodeGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24411";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ()
{
   var indexNameA = "Index_24411a";
   var indexNameB = "Index_24411b";
   var indexNameC = "Index_24411c";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   insertBulkData( dbcl, recordNum );

   var nodeInfo = db.getRG( groupName ).getSlave();
   var nodeName = nodeInfo["_nodename"];

   dbcl.createIndex( indexNameA, { no: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   dbcl.createIndex( indexNameB, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   dbcl.createIndex( indexNameC, { c: 1 }, { Standalone: true }, { NodeName: nodeName } );

   //场景a：创建一致性索引，其中索引名和索引定义都相同
   dbcl.createIndex( indexNameA, { no: 1, b: 1 } );
   checkOneIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexNameA );
   commCheckLSN( db, groupName );
   checkExistIndexConsistent( db, COMMCSNAME, testConf.clName, indexNameA );
   dbcl.dropIndex( indexNameA );
   checkOneIndexTaskResult( "Drop index", COMMCSNAME, testConf.clName, indexNameA );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexNameA, false );

   //场景b：创建一致性索引，其中索引名相同，索引定义不同 
   dbcl.createIndex( indexNameB, { b: -1 } );
   checkOneIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexNameB );
   checkIndexConsistent( db, COMMCSNAME, testConf.clName, indexNameB, nodeName, { a: 1 }, { b: -1 } );
   dbcl.dropIndex( indexNameB );
   checkOneIndexTaskResult( "Drop index", COMMCSNAME, testConf.clName, indexNameB );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexNameB, false );

   //场景c：创建一致性索引，其中索引名不同，索引定义相同  
   var indexNameC1 = "testindexc_24411c_1";
   dbcl.createIndex( indexNameC1, { c: 1 } );
   checkOneIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexNameC1 );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexNameC, nodeName, true );
   checkIndexExist( db, COMMCSNAME, testConf.clName, indexNameC1, true );
   dbcl.dropIndex( indexNameC );
   dbcl.dropIndex( indexNameC1 );
   checkOneIndexTaskResult( "Drop index", COMMCSNAME, testConf.clName, [indexNameA, indexNameB, indexNameC1] );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexNameC, nodeName, false );
   checkIndexExist( db, COMMCSNAME, testConf.clName, indexNameC1, false );
}

function checkIndexConsistent ( db, csname, clname, idxname, standaloneNodeName, indexKey1, indexKey2 )
{
   var doTime = 0;
   var timeOut = 300000;
   var nodes = commGetCLNodes( db, csname + "." + clname );
   var indexAttrs = [];
   var standaloneIndexAttr = [];
   var actIndex = null;
   do
   {
      var sucNodes = 0;
      for( var i = 0; i < nodes.length; i++ )
      {
         var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
         try
         {
            var dbcl = seqdb.getCS( csname ).getCL( clname );
         } catch( e )
         {
            if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         try
         {
            actIndex = dbcl.getIndex( idxname );
            sucNodes++;
         } catch( e )
         {
            if( e != SDB_IXM_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         seqdb.close();

         var indexDef = actIndex.toObj().IndexDef;
         delete indexDef.CreateTime;
         delete indexDef.RebuildTime;
         if( ( nodes[i].HostName + ":" + nodes[i].svcname ) == standaloneNodeName )
         {
            standaloneIndexAttr = indexDef;
         }
         else
         {
            indexAttrs.push( indexDef );
         }
      }
      sleep( 200 );
      doTime += 200;
   } while( doTime < timeOut && sucNodes < nodes.length );

   if( doTime >= timeOut )
   {
      throw new Error( "check timeout index not synchronized !" );
   }

   //检查独立索引对应索引键信息
   var standaloneIndexKey = standaloneIndexAttr.key;
   assert.equal( standaloneIndexKey, indexKey1 );
   //检查一致性索引信息
   for( var i = 0; i < indexAttrs.length; i++ )
   {
      assert.equal( indexAttrs[0], indexAttrs[i] );
   }
   var indexIndexKey = indexAttrs[0].key;
   assert.equal( indexIndexKey, indexKey2 );
}

