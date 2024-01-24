/*******************************************************************
* @Description : test resetSnapshot() option function
*                seqDB-14445:Type字段测试
*                seqDB-14446:SessionID字段测试
*                seqDB-14447:命令位置参数测试
*                seqDB-15383:ResetTimestamp字段测试
* @author      : linsuqiang
* 
*******************************************************************/
// 本用例实现是文本用例的简化，因为功能测试是手工覆盖的，
// 本自动化只是驱动接口测试

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = "resetSnapshot14445";
   var clName = "resetSnapshot14445";
   var cl = commCreateCL( db, csName, clName, /*option*/{},
         /*autoCreateCS*/true, /*ignoreExisted*/true );
   insertData( cl );

   var clFullName = csName + "." + clName;
   var rgName = commGetCLGroups( db, clFullName )[0];
   var dataDB = db.getRG( rgName ).getMaster().connect();
   var dbTime1 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime1 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS );

   var notExistID = 999999; // is nearly impossible to be such many sessions
   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "sessions", SessionID: notExistID } );
   if( isSessionSnapClean( dataDB ) )
   {
      throw new Error( "session snap shouldn't be reset(notExistID)" );
   }
   if( isDatabaseSnapClean( dataDB ) )
   {
      throw new Error( "database snap shouldn't be reset(notExistID)" );
   }

   var currID = getCurrentID( dataDB );
   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "sessions", SessionID: currID } );
   if( !isSessionSnapClean( dataDB ) )
   {
      throw new Error( "session snap should be reset(validID)" );
   }
   if( isDatabaseSnapClean( dataDB ) )
   {
      throw new Error( "database snap shouldn't be reset(validID)" );
   }
   var dbTime2 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime2 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS_CURRENT );
   if( dbTime1 !== dbTime2 )
   {
      throw new Error( "database time1 equal database time2." );
   }
   if( ssTime1 >= ssTime2 )
   {
      throw new Error( "session time2 more than session time1." );
   }

   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "database", NodeSelect: "secondary" } );
   if( isSessionSnapClean( dataDB ) )
   {
      throw new Error( "session snap shouldn't be reset(notSelected)" );
   }
   if( isDatabaseSnapClean( dataDB ) )
   {
      throw new Error( "database snap shouldn't be reset(notSelected)" );
   }

   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "database", NodeSelect: "primary" } );
   if( isSessionSnapClean( dataDB ) )
   {
      throw new Error( "session snap shouldn't be reset(selected)" );
   }
   if( !isDatabaseSnapClean( dataDB ) )
   {
      throw new Error( "database snap should be reset(selected)" );
   }
   var dbTime3 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime3 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS_CURRENT );
   if( dbTime2 >= dbTime3 )
   {
      throw new Error( "session time3 more than session time2." );
   }
   if( ssTime2 !== ssTime3 )
   {
      throw new Error( "database time2 equal database time3." );
   }

   dataDB.close();
   commDropCS( db, csName );
}

function insertData ( cl )
{
   var docNum = 100;
   var docs = [];
   for( var i = 0; i < docNum; ++i )
   {
      docs.push( { a: 1 } );
   }
   cl.insert( docs );
}

function createStatisInfo ( dataDB, csName, clName )
{
   var cl = dataDB.getCS( csName ).getCL( clName );
   var cur = cl.find();
   while( cur.next() ) { }
   cur.close();
}

// if another one also directly connects data node, 
// this judge may be not excat. 
function isSessionSnapClean ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_SESSIONS, { "Type": "Agent" } );
   var totalRead = cur.next().toObj().TotalRead;
   cur.close();
   return ( totalRead == 0 );
}

function isDatabaseSnapClean ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_DATABASE );
   var totalRead = cur.next().toObj().TotalRead;
   cur.close();
   return ( totalRead == 0 );
}

function getCurrentID ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_SESSIONS_CURRENT );
   var sessionID = cur.next().toObj().SessionID;
   cur.close();
   return sessionID;
}

function getResetSnapTime ( dataDB, snapType )
{
   var cur = dataDB.snapshot( snapType );
   var time = cur.next().toObj().ResetTimestamp;
   cur.close();
   return time;
}
