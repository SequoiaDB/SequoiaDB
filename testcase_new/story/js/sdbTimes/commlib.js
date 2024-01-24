/*******************************************************************************
*@Description : matches testcase common functions and varialb
*@Modify list :
*              2017-02-28 xiaoni huang
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var cmd = new Cmd();

function cmdRun ( str )
{
   var rc = cmd.run( str ).split( "\n" )[0];
   return rc;
}

/* ****************************************************
@description: turn to local time
@parameter:
time: Timestamp with time zone to millisecond, eg:'1901-12-31T15:54:03.000Z'
format: eg:%Y-%m-%d-%H.%M.%S.000000
@return:
localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   var localtime;
   var msecond = new Date( time ).getTime();
   var second = parseInt( msecond / 1000 ); //millisecond to second
   localtime = cmdRun( 'date -d@"' + second + '" "+' + format + '"' );

   return localtime;
}

/*******************************************************************************
*@Description : 集合中不存在记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function checkCount ( cl, expRecordNum, findConf )
{
   if( typeof ( expRecordNum ) == "undefined" ) { expRecordNum = 0; };
   if( typeof ( findConf ) == "undefined" ) { findConf = null; };
   var cnt = cl.count( findConf );
   var actCnt = Number( cnt );
   assert.equal( expRecordNum, actCnt );

}

/*******************************************************************************
*@Description : 插入无效的记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function insertRecs ( cl, rawData )
{
   var i = 0;
   for( i = 0; i < rawData.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.insert( rawData[i] );
      } );
   }
}

/*******************************************************************************
*@Description : 删除多条记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function removeDatas ( dbcl, array )
{
   var i = 0;
   for( i = 0; i < array.length; i++ )
   {
      dbcl.remove( array[i] );
   }

}

/*******************************************************************************
@Description : 比较2个数组是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( actRecs, expRecs )
{
   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }

}

/*******************************************************************************
@Description : 从数组中获取元素作为查询条件，将匹配的记录存入array中返回
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function cusorToArray ( cl, array )
{
   var i = 0;
   //get actual records to array
   var rcData = [];
   for( i = 0; i < array.length; i++ )
   {
      var cursor = cl.find( array[i], { _id: { $include: 0 } } ).sort( { a: 1 } );
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   return rcData
}

/*******************************************************************************
*@Description : 更新多条记录
*@Modify list :
*              2019-11-19 zhaoyu
*******************************************************************************/
function updateDatas ( dbcl, array )
{
   var i = 0;
   for( i = 0; i < array.length; i++ )
   {
      dbcl.update( { $set: { up: array[i]["b"] } }, array[i] );
   }

}

/*******************************************************************************
*@Description : 比对记录2个字段值是否相等
*@Modify list : 入参：游标
*              2019-11-19 zhaoyu
*******************************************************************************/
function checkUpdateRec ( rc )
{
   //get actual records to array
   var rcData = [];

   while( tmpRec = rc.next() )
   {
      //check result
      assert.equal( tmpRec.toObj()["b"], tmpRec.toObj()["up"] );
   }

}
