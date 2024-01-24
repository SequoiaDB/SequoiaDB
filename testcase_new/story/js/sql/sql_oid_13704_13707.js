/************************************
*@Description: 内置SQL支持oid带条件查询,支持is not null查询
*@author:      luweikang
*@createdate:  2017.12.20
*@testlinkCase:seqDB-13704，seqDB-13707
**************************************/
testConf.csName = CHANGEDPREFIX + "_13704_CS", testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_13704_CL", testConf.clOpt = {};

main( test );

function test ()
{
   isnotnullSQL( testPara.testCL, testConf.csName, testConf.clName );

   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713F7953E6769804000001\")', { $oid: "55713f7953e6769804000001" }, true );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713f7953e\")', { $oid: "55713f7953e6769804000001" }, false );

   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713f7953e6769804000001\")', 'oid(\"55713f7953e\")', null, false );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713F7953E6769804000001\")', 'oid(\"55713f7953e6769804000111\")', { $oid: "55713f7953e6769804000111" }, true );

   selectSQL( db, testConf.csName, testConf.clName, 'oid(\"55713f7953e6769804000111\")', true );
   selectSQL( db, testConf.csName, testConf.clName, 'oid(\"55713f7953e\")', false );

   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713f7953e\")', null, false );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'oid(\"55713F7953E6769804000111\")', { $oid: "55713f7953e6769804000111" }, true );
}

function isnotnullSQL ( cl, csName, clName )
{
   var doc = [
      { num: 1, textFields: null },
      { num: 2, textFields: 'textstr' }];
   cl.insert( doc );
   var sql = 'select * from ' + csName + "." + clName + ' where textFields is not null';
   var cursor = db.exec( sql );
   if( cursor.next() != null )
   {
      var obj = cursor.current().toObj();
      var num = obj.num;
      if( num !== 2 )
      {
         throw new Error( "isnotnullSQL() check record," + "Expect result:{num:2, textFields:'textstr'}" + "Real result:" + obj );
      }
   }
   else
   {
      throw new Error( "isnotnullSQL() check record," + "Expect result:have data" + "Real result:no data" );
   }
}


function insertSQL ( db, cl, csName, clName, insertValue, checkValue, result )
{
   var sql = 'insert into ' + csName + '.' + clName + '(num, textFields ) values (3, ' + insertValue + ')';
   if( result )
   {
      db.execUpdate( sql );
      var cursor = cl.find( { textFields: checkValue } );
      if( cursor.next() == null )
      {
         throw new Error( "insertSQL() check record," + "Expect result:have data" + "Real result:no data" + obj );
      }
   }
   else
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.execUpdate( sql );
      } );
   }

}

function updateSQL ( db, cl, csName, clName, oldValue, newValue, checkValue, result )
{
   var sql = 'update ' + csName + '.' + clName + ' set textFields=' + newValue + ' where textFields=' + oldValue;
   if( result )
   {
      db.execUpdate( sql );
      var cursor = cl.find( { textFields: checkValue } );
      if( cursor.next() == null )
      {
         throw new Error( "updateSQL() check record," + "Expect result:have data" + "Real result:no data" + obj );
      }
   }
   else
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.execUpdate( sql );
      } );
   }
}

function selectSQL ( db, csName, clName, value, result )
{
   var sql = 'select * from ' + csName + "." + clName + ' where textFields=' + value;
   if( result )
   {
      var cursor = db.exec( sql );
      if( cursor.next() == null )
      {
         throw new Error( "selectSQL() check record," + "Expect result:have data" + "Real result:no data" + obj );
      }
   }
   else
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.execUpdate( sql );
      } );
   }
}

function deleteSQL ( db, cl, csName, clName, deleteValue, checkValue, result )
{
   var sql = 'delete from ' + csName + '.' + clName + ' where textFields=' + deleteValue;
   if( result )
   {
      db.execUpdate( sql );
      var cursor = cl.find( { textFields: checkValue } );
      if( cursor.next() != null )
      {
         throw new Error( "deleteSQL() check record," + "Expect result:no data" + "Real result:have data" + obj );
      }
   }
   else
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.execUpdate( sql );
      } );
   }
}