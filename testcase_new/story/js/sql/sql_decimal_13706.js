/************************************
*@Description: 内置SQL支持decimal带条件查询
*@author:      luweikang
*@createdate:  2017.12.20
*@testlinkCase:seqDB-13706
**************************************/
testConf.csName = CHANGEDPREFIX + "_13706_CS", testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_13706_CL", testConf.clOpt = {};

main( test );

function test ()
{
   isnotnullSQL( testPara.testCL, testConf.csName, testConf.clName );

   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(2147483648.2147483648)', { $decimal: "2147483648.2147483648" }, true );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(1.7E+309)', { $decimal: "1.7E+309" }, true );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(\"100.01\")', { $decimal: "100.01" }, false );

   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(2147483648.2147483648)', 'decimal(\"9223372036854775808\")', { $decimal: "9223372036854775808" }, false );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(2147483648.2147483648)', 'decimal(9223372036854775808)', { $decimal: "9223372036854775808" }, true );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(1.7E+309)', 'decimal(100.01)', { $decimal: "100.01" }, true );

   selectSQL( db, testConf.csName, testConf.clName, 'decimal(\"9223372036854775808\")', false );
   selectSQL( db, testConf.csName, testConf.clName, 'decimal(9223372036854775808)', true );
   selectSQL( db, testConf.csName, testConf.clName, 'decimal(100.01)', true );

   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(\"9223372036854775808\")', { $decimal: "9223372036854775808" }, false );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(9223372036854775808)', { $decimal: "9223372036854775808" }, true );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'decimal(100.01)', { $decimal: "100.01" }, true );
}

function isnotnullSQL ( cl, csName, clName )
{
   var doc = [
      { num: 1, textFields: null },
      { num: 2, textFields: 'textstr' }];
   cl.insert( doc );
   var sql = 'update ' + csName + "." + clName + ' set num=10 where textFields is not null';
   db.execUpdate( sql );
   var cursor = cl.find( { num: 10 } );
   if( cursor.next() == null )
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
         throw new Error( "insertSQL() check record," + "Expect result:have data" + "Real result:no data" );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG && e.message != SDB_SQL_SYNTAX_ERROR )
         {
            throw new Error( e );
         }
      }
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
         throw new Error( "updateSQL() check record," + "Expect result:have data" + "Real result:no data" );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG && e.message != SDB_SQL_SYNTAX_ERROR )
         {
            throw new Error( e );
         }
      }
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
         throw new Error( "selectSQL() check record," + "Expect result:have data" + "Real result:no data" );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG && e.message != SDB_SQL_SYNTAX_ERROR )
         {
            throw new Error( e );
         }
      }
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
         throw new Error( "deleteSQL() check record," + "Expect result:no data" + "Real result:have data" );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG && e.message != SDB_SQL_SYNTAX_ERROR )
         {
            throw new Error( e );
         }
      }
   }
}