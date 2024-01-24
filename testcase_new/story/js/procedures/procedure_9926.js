/******************************************************************************
@Description : seqDB-9926:在不同的coord上操作存储过程
@Modify list :
               2016-9-11   TingYU      Init
******************************************************************************/
var pcdName = COMMCLNAME + '_procedurename_9926';

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var coordArr = getCoord();
   if( coordArr.length < 2 )
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );
   createPcd( db1, db2 );
   removePcd( db1, db2 );
   createPcdAgain( db1, db2 );
   clean();
}

function createPcd ( db1, db2 )
{
   var str = "db.createProcedure( function " + pcdName + "() {return 100;} )";
   db1.eval( str );


   var expPcds = [];

   var expPcd1 = {};
   expPcd1["name"] = pcdName;
   expPcd1["func"] = { "$code": "function " + pcdName + "() {\n    return 100;\n}" };
   expPcd1["funcType"] = 0;
   expPcds.push( expPcd1 );

   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function removePcd ( db1, db2 )
{
   db1.removeProcedure( pcdName );

   var expPcds = [];
   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function createPcdAgain ( db1, db2 )
{
   var str = "db.createProcedure( function " + pcdName + "() {return 99;} )";
   db1.eval( str );


   var expPcds = [];

   var expPcd1 = {};
   expPcd1["name"] = pcdName;
   expPcd1["func"] = { "$code": "function " + pcdName + "() {\n    return 99;\n}" };
   expPcd1["funcType"] = 0;
   expPcds.push( expPcd1 );

   var rc = db2.listProcedures( { name: pcdName } );
   checkResult( rc, expPcds );
}

function ready ()
{
   fmpRemoveProcedures( [pcdName], true );
}

function clean ()
{
   fmpRemoveProcedures( [pcdName], true );
}

function getCoord ()
{
   var coordInfo = db.getRG( 'SYSCoord' ).getDetail().current().toObj().Group;
   var coordArr = [];
   for( var i in coordInfo )
   {
      var hostname = coordInfo[i].HostName;
      var port = coordInfo[i].Service[0].Name;
      var hostPort = hostname + ':' + port;
      coordArr.push( hostPort );
   }

   return coordArr;
}