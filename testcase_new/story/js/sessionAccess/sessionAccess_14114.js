/******************************************************************************
 * @Description   : seqDB-14114:设置不同的timeout值执行操作超时
 * @Author        : wuyan
 * @CreateTime    : 2018.01.29
 * @LastEditTime  : 2022.10.06
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = CHANGEDPREFIX + "_14114";
testConf.clOpt = { ReplSize: 0 };
main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   bulkInsert( dbcl, 100000 );

   var timeoutValues = [1, 1000, 2000];
   for( var i = 0; i < timeoutValues.length; i++ )
   {
      db.setSessionAttr( { PreferedInstance: "M", Timeout: timeoutValues[i] } );
      try
      {
         //如果执行时间小于2s内不会超时
         dbcl.update( { $set: { a: "aaaaaa" } } );
      }
      catch( e )
      {
         if( e.message != SDB_TIMEOUT )
         {
            throw new Error( e );
         }
      }

      checkTimeoutValue( timeoutValues[i] );
      db.setSessionAttr( { Timeout: -1 } );
   }
}

function checkTimeoutValue ( timeoutValue )
{
   var timeout = db.getSessionAttr().toObj().Timeout;
   if( timeoutValue < 1000 && timeoutValue != -1 )
   {
      timeoutValue = 1000
   }
   assert.equal( timeout, timeoutValue );
}

function bulkInsert ( cl, insertNums )
{
   var batchNums = 10000;
   var times = insertNums / batchNums;

   for( var k = 0; k < times; k++ )
   {
      var doc = [];
      for( var i = 0; i < batchNums; ++i )
      {
         doc.push( { a: "string" } );
      }
      cl.insert( doc );
   }
}

