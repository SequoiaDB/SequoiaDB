/*******************************************************************************
*@Description: findandremove basic testcases
*@Modify list:
*   2014-4-3 wenjing wang  Init
*******************************************************************************/

main( test );

function test ()
{
   var clName = COMMCLNAME + "_12142";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   testNormal( cl );
   testSortNotExistIndex( cl );
   testSortExistIndex( cl );
   testWithCount( cl )
   testUsedSkipNonSplit( cl );
   testUsedLimitNonSplit( cl );
   testUsedSkipAndLimitNonSplit( cl );
   commDropCL( db, COMMCSNAME, clName );
}

/***********************************************************************
*@Description：op为remove时, 正常操作
*@Input：find( {a:1} ).remove()
*@Expectation：删除存在{a:1}的文档
*************************************************************************/
function testNormal ( cl )
{
   cl.insert( { a: 1 } );
   var arr = cl.find( { a: 1 } ).remove().toArray();
   var obj = eval( "( " + arr[0] + " )" )
   if( 1 != obj["a"] )
   {
      throw new Error( "obj[\"a\"]: " + obj["a"] );
   }

   var recordnum = cl.find( { a: 1 } ).count()
   if( 0 != parseInt( recordnum ) )
   {
      throw new Error( "recordnum: " + recordnum );
   }
   cl.truncate();
}

/****************************************************************
*@Description：op为remove时，针对结果做排序，排序字段上存在索引
*@Input：find().remove().sort( {_id:-1} )
*@Expectation：所有记录被删除，结果按降序输出
*****************************************************************/
function testSortExistIndex ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arr = cl.find().remove().sort( { _id: -1 } ).hint( { "": "$id" } ).toArray();
   if( 10 != arr.length )
   {
      throw new Error( "arr.length: " + arr.length );
   }

   var maxval = 0;
   for( var i = 0; i < arr.length; ++i )
   {
      var obj = eval( "( " + arr[i] + " )" );
      if( 0 == i )
      {
         maxval = obj._id;
      }
      else if( maxval < obj._id )
      {
         throw new Error( "maxval: " + maxval + "\nobj._id: " + obj._id );
      }
   }

   var recordnumber = cl.find().count();
   if( 0 != parseInt( recordnumber ) )
   {
      throw new Error( "recordnumber: " + recordnumber );
   }
   cl.truncate();
}

/****************************************************************
*@Description：op为remove时，针对结果做排序，排序字段上不存在索引
*@Input：find().remove().sort( {c:-1} )
*@Expectation：报-288错误
*****************************************************************/
function testSortNotExistIndex ( cl )
{
   try
   {
      loadMultipleDoc( cl, 10 );
      cl.find().remove().sort( { c: -1 } ).toArray();
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

/****************************************************************
*@Description：op为remove时，针对结果做count
*@Input：find( {a:1} ).remove().count()
*@Expectation：报
count() cannot be executed with update() or remove()
*****************************************************************/
function testWithCount ( cl )
{  
   cl.insert( { a: 1 } ); 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { a: 1 } ).remove().count(); 
   } );
   cl.truncate();
}

/****************************************************************
*@Description：op为remove时，给合skip
*@Input：find().skip( 5 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedSkipNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arrdoc = cl.find().skip( 5 ).remove().toArray();
   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/****************************************************************
*@Description：op为remove时，给合limit
*@Input：find().limit( 5 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedLimitNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arrdoc = cl.find().limit( 5 ).remove().toArray();
   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/****************************************************************
*@Description：op为remove时，给合limit
*@Input：find().skip( 5 ).limit( 2 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedSkipAndLimitNonSplit ( cl )
{
   loadMultipleDoc( cl, 10 );
   var arrdoc = cl.find().skip( 5 ).limit( 2 ).remove().toArray();
   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}
