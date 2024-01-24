/******************************************************************************
*@Description : seqDB-20031:指定Type，Collection，重置普通集合的集合快照信息
*@author      : Zhao xiaoni
*@Date        : 2019-10-24
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = "cl_20031";
   var indexName = "index_20031";
   var replSize = 0;
   var indexDef = { a: 1 };
   var fullName = COMMCSNAME + "." + clName;

   commDropCL( db, COMMCSNAME, clName );
   var groupName = commGetGroups( db )[0][0].GroupName;
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: replSize } );
   commCreateIndex( cl, indexName, indexDef, { Unique: true } );

   for( var i = 0; i < 100; i++ )
   {
      cl.insert( [{ a: i }, { b: i }] );
      cl.update( { "$set": { a: ( 100 + i ) } }, { a: i } );
      cl.remove( { a: ( 100 + i ) } );
      var cursor = cl.find().hint( { "": "a" } );
      while( cursor.next() ) { }
      cursor = cl.find().hint( { "": null } );
      while( cursor.next() ) { }
   }

   db.resetSnapshot( { Type: "collections", Collection: fullName } );

   var masterNode = db.getRG( groupName ).getMaster();
   var hostName = masterNode.getHostName();
   var serviceName = masterNode.getServiceName();
   var nodeNames = [hostName + ":" + serviceName];

   var actStatistics = getStatistics( fullName, nodeNames );
   var expStatistics = [
      {
         "NodeName": nodeNames[0], "TotalDataRead": 0, "TotalIndexRead": 0, "TotalDataWrite": 0, "TotalIndexWrite": 0, "TotalUpdate": 0, "TotalDelete": 0, "TotalInsert": 0, "TotalSelect": 0, "TotalRead": 0, "TotalWrite": 0,
         "TotalTbScan": 0, "TotalIxScan": 0
      }];
   checkStatistics( actStatistics, expStatistics );

   commDropCL( db, COMMCSNAME, clName, false, false );
}
