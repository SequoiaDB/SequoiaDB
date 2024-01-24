/************************************
*@Description: 内置SQL支持timestamp带条件查询 
*@author:      wangkexin
*@createdate:  2019.3.2
*@testlinkCase:seqDB-13705
**************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "cl13705", testConf.clOpt = {};

main( test );

function test ()
{
   //正常timestamp类型数据、边界值、非法值
   //Timestamp类型能表示的时间范围为1902-01-01 00:00:00.000000至2037-12-31 23:59:59.999999
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2019-03-02-10:48:50.000000")', { $timestamp: "2019-03-02-10.48.50.000000" }, true );
   var timestamp = insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1902-01-01-00:00:00.000000")', { $timestamp: "1902-01-01-00.00.00.000000" }, true );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2037-12-31-23:59:59.999999")', { $timestamp: "2037-12-31-23.59.59.999999" }, true );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
   insertSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("978192000000")', false );

   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("2019-03-02-10:48:50.000000")', { $timestamp: "2019-03-02-10.48.50.000000" }, true );
   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("1902-01-01-00:00:00.000000")', { $timestamp: timestamp }, true );
   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("2037-12-31-23:59:59.999999")', { $timestamp: "2037-12-31-23.59.59.999999" }, true );
   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
   selectSQL( db, testConf.csName, testConf.clName, 'Timestamp("978192000000")', false );

   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2019-03-02-10:48:50.000000")', 'Timestamp("2019-03-01-10:48:50.000000")', { $timestamp: "2019-03-01-10.48.50.000000" }, true );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1902-01-01-00:00:00.000000")', 'Timestamp("1902-12-01-00:00:00.000000")', { $timestamp: "1902-12-01-00.00.00.000000" }, true );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2037-12-31-23:59:59.999999")', 'Timestamp("2036-12-31-23.59.59.999999")', { $timestamp: "2036-12-31-23.59.59.999999" }, true );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1800-01-01-00:00:00.000000")', 'Timestamp("1800-12-01-00.00.00.000000")', false );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2038-12-20-00:00:00.000000")', 'Timestamp("2038-12-30-00:00:00.000000")', false );
   updateSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("978192000000")', 'Timestamp("978192000000")', false );

   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2019-03-01-10:48:50.000000")', { $timestamp: "2019-03-01-10.48.50.000000" }, true );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1902-12-01-00:00:00.000000")', { $timestamp: "1902-12-01-00.00.00.000000" }, true );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2036-12-31-23:59:59.999999")', { $timestamp: "2036-12-31-23.59.59.999999" }, true );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
   deleteSQL( db, testPara.testCL, testConf.csName, testConf.clName, 'Timestamp("978192000000")', false );

   if( testPara.testCL.count() != 0 )
   {
      throw new Error( "main() check cl data," + "Expect result:no data" + "Real result:have data" );
   }
}

function insertSQL ( db, cl, csName, clName, insertValue, checkValue, result )
{
   var sql = 'insert into ' + csName + '.' + clName + '(num, textFields ) values (3, ' + insertValue + ')';
   if( result )
   {
      var cursor = null;
      db.execUpdate( sql );
      if( JSON.stringify( checkValue ) == '{"$timestamp":"1902-01-01-00.00.00.000000"}' )
      {
         // 在1928年1月1日前，由UTC时间戳转成中国本地时区的时间，会加上5分52秒，为规避此问题，这里使用三个时间戳进行匹配，避免不同机器上产生的误差
         var checkValue1 = { $timestamp: "1901-12-31-23.54.08.000000" };
         var checkValue2 = { $timestamp: "1902-01-01-00.05.52.000000" };
         var count = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } ).count();
         if( Number( count ) !== 1 )
         {
            throw new Error( "insertSQL() check record," + "Expect result:1," + "Real result:" + Number( count ) );
         }
         var cursor = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } );
         var timestamp = cursor.next().toObj()["textFields"]["$timestamp"];
         return timestamp;
      }
      else
      {
         cursor = cl.find( { textFields: checkValue }, { "_id": { "$include": 0 } } );
         var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
         var actString = JSON.stringify( cursor.next().toObj() );
         if( expstring !== actString )
         {
            throw new Error( "insertSQL() check record," + "Expect result:" + expstring + ",Real result:" + actString );
         }
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
      var cursor = null;
      db.execUpdate( sql );
      if( JSON.stringify( checkValue ) == '{"$timestamp":"1902-12-01-00.00.00.000000"}' )
      {
         // 在1928年1月1日前，由UTC时间戳转成中国本地时区的时间，会加上5分52秒，为规避此问题，这里使用三个时间戳进行匹配，避免不同机器上产生的误差
         var checkValue1 = { $timestamp: "1902-11-30-23.54.08.000000" };
         var checkValue2 = { $timestamp: "1902-12-01-00.05.52.000000" };
         var count = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } ).count();
         if( Number( count ) !== 1 )
         {
            throw new Error( "updateSQL() check record," + "Expect result:" + oldValue + ",Real result:" + Number( count ) );
         }
      }
      else
      {
         cursor = cl.find( { textFields: checkValue }, { "_id": { "$include": 0 } } );
         var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
         var actString = JSON.stringify( cursor.next().toObj() );
         if( expstring !== actString )
         {
            throw new Error( "updateSQL() check record," + "Expect result:" + oldValue + ",Real result:" + actString );
         }
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

function selectSQL ( db, csName, clName, value, checkValue, result )
{
   var sql = 'select num,textFields from ' + csName + "." + clName + ' where textFields=' + value;
   if( result )
   {
      var cursor = db.exec( sql );
      var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
      var actString = JSON.stringify( cursor.next().toObj() );
      if( expstring !== actString )
      {
         throw new Error( "selectSQL() check record," + "Expect result:" + expstring + ",Real result:" + actString );
      }
   }
   else
   {
      try
      {
         db.exec( sql );
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
         throw new Error( "deleteSQL() check record," + "Expect result:no data,Real result:have data" );
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