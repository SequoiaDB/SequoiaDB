/******************************************************************************
*@Description : test snapshot SDB_SNAP_COLLECTIONS
*                seqDB-18655:�̶������в���/pop/��ѯ��¼�����Ͽ�����Ϣ��֤
*@auhor       : ����
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "_18655";
   var clName = COMMCLNAME + "_18655";
   var fullName = csName + "." + clName;

   commDropCS( db, csName );
   commCreateCS( db, csName, false, "", { Capped: true } );

   var dataGroupNames = getDataGroupNames();
   var groupName = dataGroupNames[0];
   var dbcl = commCreateCL( db, csName, clName, { Capped: true, Size: 1024, Compressed: false, Group: groupName } );

   var masterNode = db.getRG( groupName ).getMaster();
   var hostName = masterNode.getHostName();
   var serviceName = masterNode.getServiceName();
   var nodeNames = [hostName + ":" + serviceName];

   var doc = [];
   for( var i = 0; i < 1000; i++ )
   {
      doc.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( doc );

   var actStatistics = getStatistics( fullName, nodeNames );
   var expStatistics = [{ "TotalDataRead": 0, "TotalDataWrite": 1000, "TotalIndexWrite": 0, "TotalUpdate": 0, "TotalDelete": 0, "TotalInsert": 1000, "TotalSelect": 0, "TotalRead": 0, "TotalWrite": 1000, "TotalTbScan": 0, "TotalIxScan": 0, "NodeName": nodeNames[0] }];
   checkStatistics( actStatistics, expStatistics );

   var cursor = dbcl.find();
   while( cursor.next() ) { }
   cursor.close();
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ "NodeName": nodeNames[0], TotalDataRead: 1000, TotalDataWrite: 1000, TotalIndexWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 1000, TotalRead: 1000, TotalWrite: 1000, TotalTbScan: 1, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   dbcl.pop( { LogicalID: 0, Direction: -1 } )
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ "NodeName": nodeNames[0], TotalDataRead: 1000, TotalDataWrite: 1000, TotalIndexWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 1000, TotalSelect: 1000, TotalRead: 1000, TotalWrite: 1000, TotalTbScan: 1, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   db.resetSnapshot( { Type: "collections" } );
   actStatistics = getStatistics( fullName, nodeNames );
   expStatistics = [{ "NodeName": nodeNames[0], TotalDataRead: 0, TotalDataWrite: 0, TotalIndexWrite: 0, TotalUpdate: 0, TotalDelete: 0, TotalInsert: 0, TotalSelect: 0, TotalRead: 0, TotalWrite: 0, TotalTbScan: 0, TotalIxScan: 0 }];
   checkStatistics( actStatistics, expStatistics );

   commDropCS( db, csName );
}