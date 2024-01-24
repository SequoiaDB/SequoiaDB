/******************************************************************************
*@Description : test snapshot SDB_SNAP_COLLECTIONS
*               seqDB-18654:��ɾ�Ĳ�(��ɨ��/����ɨ��)/�зּ�¼�����Ͽ�����Ϣ��֤
*@auhor       : ����
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_18654";
   var fullName = COMMCSNAME + "." + clName;
   var dataGroupNames = getDataGroupNames();
   var groupName = dataGroupNames[0];
   commDropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Compressed: false, Group: groupName, ReplSize: 0 } );

   var nodeNames = [];
   var masterNode = db.getRG( groupName ).getMaster();
   var hostName = masterNode.getHostName();
   var serviceName = masterNode.getServiceName();
   nodeNames.push( hostName + ":" + serviceName );

   commCreateIndex( dbcl, "ab", { a: 1, b: 1 } );
   var doc = [];
   for( var i = 0; i < 1000; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( doc );

   var actStatistics = getStatistics( fullName, nodeNames );
   var expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 0, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 0, TotalRead: 0, TotalWrite: 1000, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   var cursor = dbcl.find().hint( { "": "ab" } );
   while( cursor.next() ) { }
   cursor.close();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 1000, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 1000, TotalRead: 1000, TotalWrite: 1000, TotalTbScan: 0, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   var cursor = dbcl.find().hint( { "": null } );
   while( cursor.next() ) { }
   cursor.close();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 2000, TotalDataWrite: 1000, TotalIndexWrite: 2000, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 2000, TotalWrite: 1000, TotalTbScan: 1, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   dbcl.update( { $set: { a: 1000 } } );
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 3000, TotalDataWrite: 2000, TotalIndexWrite: 4000, TotalUpdate: 1000, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 3000, TotalWrite: 2000, TotalTbScan: 2, TotalIxScan: 1 }];
   checkStatistics( actStatistics, expStatistics );

   dbcl.remove( {}, { "": "ab" } );
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 4000, TotalDataWrite: 3000, TotalIndexWrite: 6000, TotalUpdate: 1000, TotalDelete: 1000, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 4000, TotalWrite: 3000, TotalTbScan: 2, TotalIxScan: 2 }];
   checkStatistics( actStatistics, expStatistics );

   db.resetSnapshot( { Type: "collections" } );
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 0, TotalDataWrite: 0, TotalIndexWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 0, TotalSelect: 0, TotalRead: 0, TotalWrite: 0, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   dbcl.insert( doc );

   dbcl.alter( { ShardingKey: { a: 1 } } );

   dbcl.split( groupName, dataGroupNames[1], 50 );

   masterNode = db.getRG( dataGroupNames[1] ).getMaster();
   hostName = masterNode.getHostName();
   serviceName = masterNode.getServiceName();
   nodeNames.push( hostName + ":" + serviceName );

   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ NodeName: nodeNames[0], TotalDataRead: 2479, TotalDataWrite: 1479, TotalIndexWrite: 3437, TotalUpdate: 0, TotalDelete: 479, TotalInsert: 1000, TotalSelect: 2000, TotalRead: 2479, TotalWrite: 1479, TotalTbScan: 2, TotalIxScan: 479 },
   { NodeName: nodeNames[1], TotalDataRead: 0, TotalDataWrite: 479, TotalIndexWrite: 1437, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 479, TotalSelect: 0, TotalRead: 0, TotalWrite: 479, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
