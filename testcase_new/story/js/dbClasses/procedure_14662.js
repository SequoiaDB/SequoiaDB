/*******************************************************************
* @Description : create procedure and eval to return
*                Sdb SdbCS SdbCollection SdbCursor SdbQuery
*                SdbReplicaGroup SdbNode SdbDomain CLCount
*                BinData ObjectId Timestamp Regex MinKey MaxKey
*                NumberLong SdbDate
*                seqDB-14662:执行存储过程返回db/cs/cl等对象
* @author      : Liang XueWang
*                2018-03-10
*******************************************************************/
var clName = COMMCLNAME + "_dbClasses14662";
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   commCreateCL( db, COMMCSNAME, clName );

   evalSdb( db ); // can't return Sdb
   evalSdbCS( db );
   evalSdbCollection( db );
   evalSdbCursor( db );
   evalSdbQuery( db );
   evalSdbReplicaGroup( db );
   evalSdbNode( db );
   evalSdbDomain( db );
   evalCLCount( db );
   evalBinData( db );
   evalObjectId( db );
   evalTimestamp( db );
   evalRegex( db );
   evalMinKey( db );
   evalMaxKey( db );
   evalNumberLong( db );
   evalSdbDate( db );

   commDropCL( db, COMMCSNAME, clName );
}

function evalSdb ( db )
{
   commCreateProcedure( db, function getSdb ( host, svc )
   {
      var sdb = new Sdb( host, svc );
      return sdb;
   } );
   assert.tryThrow( SDB_SYS, function()
   {
      db.eval( "getSdb( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME + "\" )" );
   } );
   commRemoveProcedure( db, "getSdb" );
}

function evalSdbCS ( db )
{
   commCreateProcedure( db, function getSdbCS ( host, svc, csname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getCS( csname );
   } );
   var cs = db.eval( "getSdbCS( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME +
      "\", \"" + COMMCSNAME + "\" )" );
   cs.toString();
   var cl = cs.getCL( clName );
   commRemoveProcedure( db, "getSdbCS" );

}

function evalSdbCollection ( db )
{
   commCreateProcedure( db, function getSdbCollection ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getCS( csname ).getCL( clname );
   } );
   var cl = db.eval( "getSdbCollection( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME +
      "\", \"" + COMMCSNAME + "\", \"" + clName + "\" )" );
   // cl.toString();
   cl.find();
   commRemoveProcedure( db, "getSdbCollection" );

}

function evalSdbCursor ( db )
{
   commCreateProcedure( db, function getSdbCursor ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var collection = db.getCS( csname ).getCL( clname );
      return collection.find().explain( { Run: true } );
   } );
   var cursor = db.eval( "getSdbCursor( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME + "\", \"" +
      clName + "\" )" );
   cursor.next();
   commRemoveProcedure( db, "getSdbCursor" );

}

function evalSdbQuery ( db )
{
   commCreateProcedure( db, function getSdbQuery ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var collection = db.getCS( csname ).getCL( clname );
      return collection.find();
   } );
   var query = db.eval( "getSdbQuery( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME + "\", \"" +
      clName + "\" )" );
   query.size();
   commRemoveProcedure( db, "getSdbQuery" );

}

function evalSdbReplicaGroup ( db )
{
   commCreateProcedure( db, function getSdbReplicaGroup ( host, svc, rgname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getRG( rgname );
   } );
   var rg = db.eval( "getSdbReplicaGroup( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"SYSCoord\" )" );
   rg.getDetail();
   commRemoveProcedure( db, "getSdbReplicaGroup" );

}

function evalSdbNode ( db )
{
   commCreateProcedure( db, function getSdbNode ( host, svc, rgname )
   {
      var sdb = new Sdb( host, svc );
      var rg = sdb.getRG( rgname );
      return rg.getSlave();
   } );

   var node = db.eval( "getSdbNode( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"SYSCoord\" )" );
   node.getHostName();
   commRemoveProcedure( db, "getSdbNode" );

}

function evalSdbDomain ( db )
{
   var groups = commGetDataGroupNames( db );
   var domainName = "testDomain14662";
   commDropDomain( db, domainName );
   commCreateDomain( db, domainName, groups );

   commCreateProcedure( db, function getSdbDomain ( host, svc, domainname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getDomain( domainname );
   } );
   var domain = db.eval( "getSdbDomain( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"" + domainName + "\" )" );
   commRemoveProcedure( db, "getSdbDomain" );
   commDropDomain( db, domainName );
}

function evalCLCount ( db )
{
   commCreateProcedure( db, function getCLCount ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var cl = sdb.getCS( csname ).getCL( clname );
      return cl.count();
   } );

   var cnt = db.eval( "getCLCount( \"" + COORDHOSTNAME + "\", " +
      "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME +
      "\", \"" + clName + "\" )" );
   commRemoveProcedure( db, "getCLCount" );

}

function evalBinData ( db )
{
   commCreateProcedure( db, function getBinData ( data, type )
   {
      return BinData( data, type );
   } );
   var data = "aGVsbG8gd29ybGQ";
   var type = "1";
   var bindata = db.eval( "getBinData( \"" + data + "\", \"" + type + "\" )" );
   var expectval = BinData( data, type );
   assert.equal( bindata.toString(), expectval.toString() );

   commRemoveProcedure( db, "getBinData" );

}

function evalObjectId ( db )
{
   commCreateProcedure( db, function getObjectId ( data )
   {
      return ObjectId( data );
   } );
   var data = "55713f7953e6769804000001";
   var oid = db.eval( "getObjectId( \"" + data + "\" )" );
   var expectval = ObjectId( data );
   assert.equal( oid.toString(), expectval.toString() );

   commRemoveProcedure( db, "getObjectId" );
}

function evalTimestamp ( db )
{
   commCreateProcedure( db, function getTimestamp ( time )
   {
      return Timestamp( time );
   } );

   var time = "2015-06-05-16.10.33.000000";
   var timestamp = db.eval( "getTimestamp( \"" + time + "\" )" );
   var expectval = Timestamp( time );
   assert.equal( timestamp.toString(), expectval.toString() );

   commRemoveProcedure( db, "getTimestamp" );
}

function evalRegex ( db )
{
   commCreateProcedure( db, function getRegex ( pattern, options )
   {
      return Regex( pattern, options );
   } );
   var pattern = "^W";
   var options = "i";
   var regex = db.eval( "getRegex( \"" + pattern + "\", \"" + options + "\" )" );
   var expectval = Regex( pattern, options );
   assert.equal( regex.toString(), expectval.toString() );

   commRemoveProcedure( db, "getRegex" );
}

function evalMinKey ( db )
{
   commCreateProcedure( db, function getMinKey ()
   {
      return MinKey();
   } );
   var minKey = db.eval( "getMinKey()" );
   commRemoveProcedure( db, "getMinKey" );
}

function evalMaxKey ( db )
{
   commCreateProcedure( db, function getMaxKey ()
   {
      return MaxKey();
   } );
   var maxKey = db.eval( "getMaxKey()" );
   commRemoveProcedure( db, "getMaxKey" );

}

function evalNumberLong ( db )
{
   commCreateProcedure( db, function getNumberLong ( number )
   {
      return NumberLong( number );
   } );
   var number = 2147483648;
   var numberLong = db.eval( "getNumberLong( " + number + " )" );
   var expectval = NumberLong( number );
   assert.equal( numberLong.toString(), expectval.toString() );

   commRemoveProcedure( db, "getNumberLong" );
}

function evalSdbDate ( db )
{
   commCreateProcedure( db, function getSdbDate ( date )
   {
      return SdbDate( date );
   } );
   var date = "2015-03-13";
   var sdbDate = db.eval( "getSdbDate( \"" + date + "\" )" );
   var expectval = SdbDate( date );
   assert.equal( sdbDate.toString(), expectval.toString() );

   commRemoveProcedure( db, "getSdbDate" );
}

/******************************************************************************
@description  create procedure
@author  zhaoyu
@parameter
***************************************************************************** */
function commCreateProcedure ( db, code, ignoreExisted )
{
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   try
   {
      db.createProcedure( code );
   } catch( e )
   {
      if( !commCompareErrorCode( e, SDB_FMP_FUNC_EXIST ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateProcedure, create procedure: " + code + " failed: " + e );
      }
   }
}

/******************************************************************************
@description  remove procedure
@author  zhaoyu
@parameter
***************************************************************************** */
function commRemoveProcedure ( db, functionName, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   try
   {
      db.removeProcedure( functionName );
   } catch( e )
   {
      if( !commCompareErrorCode( e, SDB_FMP_FUNC_NOT_EXIST ) || !ignoreNotExist )
      {
         commThrowError( e, "commRemoveProcedure, remove procedure: " + functionName + " failed: " + e );
      }
   }
}
