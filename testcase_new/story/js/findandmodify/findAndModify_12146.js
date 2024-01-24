/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-3 wenjing wang  Init
*******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_12146";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   testRuleEmpty( cl );
   testReturnNewNonBoolean( cl );
   testNormal( cl );
   testNormal( cl, true );
   testReturnNewIsTrue( cl );
   testSortNotExistIndex( cl );
   testSortExistIndex( cl );
   testWithCount( cl )
   testUsedSkipNonSplit( cl );
   testUsedLimitNonSplit( cl );
   testUsedSkipAndLimitNonSplit( cl );
   commDropCL( db, COMMCSNAME, clName );
}
/*******************************************************************************
*@Description：测试op为update时, 更新规则字段为空
*@Input：find( {a:1} ).update( {} )
*@Expectation：预期抛出如下异常:-6 SDB_INVALIDARG
SdbQuery.update(): the 1st param should be non-empty object
********************************************************************************/
function testRuleEmpty ( cl )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { a: 1 } ).update( {} ); 
   } );
}

/*******************************************************************************
*@Description：测试op为update时, returnnew为非boolean值
*@Input：find( {a:1} ).update( {$inc:{a:1}}, 1 )
*@Expectation：预期抛出如下异常:-6 SDB_INVALIDARG
SdbQuery.update(): the 2nd param should be boolean
********************************************************************************/
function testReturnNewNonBoolean ( cl )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, 1 ).toArray();
   } );
   
}

/*******************************************************************************
*@Description：测试op为update时, 正常操作
*@Input：find( {a:1} ).update( {$inc:{a:1}}, false )
*@Expectation：返回文档的a字段为了2，数据库中{a:2}的文档数为1
********************************************************************************/
function testNormal ( cl, useflag )
{
   if( undefined != useflag )
   {
      var returnNew = false;
   }

   cl.insert( { a: 1 } );
   if( undefined == returnNew )
   {
      var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } } ).toArray();
   }
   else
   {
      var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, returnNew ).toArray();
   }

   var obj = eval( "( " + arr[0] + " )" )
   if( 1 != obj["a"] )
   {
      throw new Error( "obj[\"a\"]: " + obj["a"] );
   }

   var recordnum = cl.find( { a: 2 } ).count();
   if( 1 != parseInt( recordnum ) )
   {
      throw new Error( "recordnum: " + recordnum );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, returnnew为真
*@Input：find( {a:1} ).update( {$inc:{a:1}}, true )
*@Expectation：返回文档的a字段为了2，数据库中{a:2}的文档数为1
********************************************************************************/
function testReturnNewIsTrue ( cl )
{
   var funname = "testReturnNewIsTrue";
   cl.insert( { a: 1 } );
   var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, true ).toArray();
   var obj = eval( "( " + arr[0] + " )" )
   if( 2 != obj["a"] )
   {
      throw new Error( "obj[\"a\"]: " + obj["a"] );
   }

   var recordnum = cl.find( { a: 2 } ).count()
   if( 1 != parseInt( recordnum ) )
   {
      throw new Error( "recordnum: " + recordnum );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 对返回结果排序，排序字段存在索引
*@Input：find().update( {$set:{c:1}}, true ).sort( {_id:-1} )
*@Expectation：结果按降序输出
********************************************************************************/
function testSortExistIndex ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arr = cl.find().update( { $set: { c: 1 } }, true ).sort( { _id: -1 } ).hint( { "": "$id" } ).toArray();
   if( 10 != arr.length )
   {
      throw new Error( "arr: " + JSON.stringify( arr ) );
   }

   var maxval = 0;
   for( var i = 0; i < arr.length; ++i )
   {
      var obj = eval( "( " + arr[i] + " )" );
      if( 0 == i )
      {
         maxval = obj["_id"];
      }
      else if( maxval < obj["_id"] )
      {
         throw new Error( "maxval: " + maxval + "\nobj[\"_id\"]: " + obj["_id"] );
      }
      else if( 1 != obj["c"] )
      {
         throw new Error( "obj[\"c\"]: " + obj["c"] );
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 对返回结果排序，排序字段不存在索引
*@Input：find().update( {$set:{c:1}}, true ).sort( {c:-1} )
*@Expectation：报-288错误
********************************************************************************/
function testSortNotExistIndex ( cl )
{
   try
   {
      loadMultipleDoc( cl, 10 );
      var cursor = cl.find().update( { $set: { c: 1 } }, true ).sort( { c: -1 } );
      while( cursor.next() ) { }
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_SORT_NO_IDX != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 给合count
*@Input：find( {a:1} ).update( {$inc:{a:1}} ).count()
*@Expectation：throw如下异常:-6 SDB_INVALIDARG
count() cannot be executed with update() or remove()
********************************************************************************/
function testWithCount ( cl )
{
   cl.insert( { a: 1 } );   
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { a: 1 } ).update( { $inc: { a: 1 } } ).count();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用skip
*@Input：find().skip( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedSkipNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arrdoc = cl.find().skip( 5 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用limit
*@Input：find().limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedLimitNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );

   var arrdoc = cl.find().limit( 5 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用skip + limit
*@Input：find().skip( 5 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedSkipAndLimitNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );

   var arrdoc = cl.find().skip( 5 ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}
