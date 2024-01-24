/******************************************************************************
 * @Description   : seqDB-18654:增删改查(表扫描/索引扫描)/切分记录，集合快照信息验证
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.24
 * @LastEditTime  : 2022.09.05
 * @LastEditors   : Xu Mingxing
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_18654";
testConf.csName = COMMCSNAME + "_18654";
main( test );

function test ( testPara )
{
   var fullName = testConf.csName + "." + testConf.clName;
   var dataGroupNames = testPara.dstGroupNames;
   var groupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;

   var nodeNames = [];
   var masterNode = db.getRG( groupName ).getMaster();
   var hostName = masterNode.getHostName();
   var serviceName = masterNode.getServiceName();
   nodeNames.push( hostName + ":" + serviceName );

   var indexName = "ab";
   commCreateIndex( dbcl, indexName, { a: 1, b: 1 } );
   var doc = [];
   for( var i = 0; i < 1000; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
   }

   db.transBegin();
   dbcl.insert( doc );
   db.transCommit();
   var actStatistics = getStatistics( fullName, nodeNames );
   var expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 0, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 0, TotalRead: 0, TotalWrite: 1000, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   db.transBegin();
   dbcl.find().hint( { "": "ab" } ).toArray();
   db.transCommit();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 1000, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 1000, TotalRead: 1000, TotalWrite: 1000, TotalTbScan: 0, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   db.transBegin();
   dbcl.find().hint( { "": null } ).toArray();
   db.transCommit();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 2000, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 2000, TotalWrite: 1000, TotalTbScan: 1, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   db.transBegin();
   dbcl.update( { $set: { a: 1000 } } )
   db.transCommit();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 3000, TotalDataWrite: 2000, TotalIndexWrite: 4000, TotalUpdate: 1000, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 3000, TotalWrite: 2000, TotalTbScan: 2, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   db.transBegin();
   dbcl.remove( {}, { "": "ab" } );
   db.transCommit();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 4000, TotalDataWrite: 3000, TotalIndexWrite: 6000, TotalUpdate: 1000, TotalDelete: 1000, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 4000, TotalWrite: 3000, TotalTbScan: 2, TotalIxScan: 2 }];
   checkStatistics( actStatistics, expStatistics );

   db.resetSnapshot( { Type: "collections", Collection: fullName } );

   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 0, TotalDataWrite: 0, TotalIndexWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 0, TotalSelect: 0, TotalRead: 0, TotalWrite: 0, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   dbcl.insert( doc );

   dbcl.alter( { ShardingKey: { a: 1 } } );

   dbcl.split( groupName, dataGroupNames[0], 50 );
   masterNode = db.getRG( dataGroupNames[0] ).getMaster();
   hostName = masterNode.getHostName();
   serviceName = masterNode.getServiceName();
   nodeNames.push( hostName + ":" + serviceName );
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 2479, TotalDataWrite: 1479, TotalIndexWrite: 3437, TotalUpdate: 0, TotalDelete: 479, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 2479, TotalWrite: 1479, TotalTbScan: 2, TotalIxScan: 479 },
   { NodeName: nodeNames[1], TotalDataRead: 0, TotalDataWrite: 479, TotalIndexWrite: 1437, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 479, TotalSelect: 0, TotalRead: 0, TotalWrite: 479, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );
}
