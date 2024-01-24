/*****************************************************************
@description:   sort object in array, rg:
                array.sort(compare("key1", compare("key2", compare("key3"))));       
@input:         key
                o: object1's key
                p: object2's key					 
******************************************************************/
function compare ( name, minor )
{
   return function( o, p )
   {
      var a, b;
      if( o && p && typeof o === 'object' && typeof p === 'object' )
      {
         a = o[name];
         b = p[name];
         if( a === b ) 
         {
            return typeof minor === 'function' ? minor( o, p ) : 0;
         }
         if( typeof a === typeof b ) 
         {
            return a < b ? -1 : 1;
         }
         return typeof a < typeof b ? -1 : 1;
      }
      else 
      {
         throw new Error( "ERROR" );
      }
   }
}

/*******************************************************************************
@Description : 比较是否为json对象
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function isJson ( object ) 
{
   var isJson = object && typeof ( object ) == 'object' && Object.prototype.toString.call( object ).toLowerCase() == "[object object]";
   return isJson;
}

/*******************************************************************************
@Description : 比较2个对象是否相等
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function isObjEqual ( obj1, obj2 )
{
   if( typeof ( obj1 ) !== typeof ( obj2 ) ) return false;
   if( isJson( obj1 ) && isJson( obj2 ) )
   {
      var props1 = Object.getOwnPropertyNames( obj1 );
      var props2 = Object.getOwnPropertyNames( obj2 );
      if( props1.length !== props2.length ) return false;
      for( var i = 0; i < props1.length; i++ )
      {
         if( props1[i] !== props2[i] ) return false;
      }
   }

   for( var key in obj1 )
   {
      var oA = obj1[key];
      var oB = obj2[key];
      if( typeof ( oA ) !== typeof ( oB ) ) return false;
      if( isJson( oA ) && isJson( oB ) ) return isObjEqual( oA, oB );
      if( oA.hasOwnProperty( '$numberLong' ) && oB.hasOwnProperty( '$numberLong' ) && oA.$numberLong !== oB.$numberLong ) return false;
      if( oA.hasOwnProperty( '$decimal' ) && oB.hasOwnProperty( '$decimal' ) && oA.$decimal !== oB.$decimal ) return false;
      if( !oA.hasOwnProperty( '$numberLong' ) && !oB.hasOwnProperty( '$numberLong' )
         && !oA.hasOwnProperty( '$decimal' ) && !oB.hasOwnProperty( '$decimal' )
         && oA !== oB ) return false;
   }
   return true;
}

/*******************************************************************************
@Description : json对象按照属性值排序
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function sortJsonKeys ( obj )
{
   var tmp = {};
   Object.keys( obj ).sort().forEach( function( k ) { tmp[k] = obj[k] } );
   return tmp;
}

/*******************************************************************************
@Description : 比较查询返回的结果（游标）与预期结果(数组)是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

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

   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/*******************************************************************************
@Description : 获取coord组内所有节点名
@return: array,例如：["localhost:11810","localhost:11820"]
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function getCoordNodeNames ( db )
{
   var nodeNames = new Array();
   if( commIsStandalone( db ) )
   {
      return nodeNames;
   }
   var rg = db.getCoordRG();

   var details = rg.getDetail();
   while( details.next() )
   {
      var groups = details.current().toObj().Group;
      for( var i = 0; i < groups.length; i++ )
      {
         var hostName = groups[i].HostName;
         var service = groups[i].Service;
         for( var j = 0; j < service.length; j++ )
         {
            if( service[j].Type === 0 )
            {
               var serviceName = service[j].Name;
               break;
            }
         }
         nodeNames.push( hostName + ":" + serviceName );
      }
   }
   return nodeNames;
}

/*******************************************************************************
@Description : 获取集合的全局ID
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function getCLID ( db, csName, clName )
{
   var uniqueID = db.snapshot( 8, { Name: csName + "." + clName } ).next().toObj().UniqueID;
   return uniqueID;
}


/*******************************************************************************
@Description : 校验集合自增字段属性
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkAutoIncrementonCL ( db, csName, clName, expArr )
{
   for( var i = 0; i < expArr.length; i++ )
   {
      if( expArr[i].Generated == undefined ) { expArr[i].Generated = "default"; }
   }

   var autoIncrementArr = db.snapshot( 8, { Name: csName + "." + clName } ).next().toObj().AutoIncrement;
   assert.equal( autoIncrementArr.length, expArr.length );
   autoIncrementArr.sort( compare( "Field" ) );
   expArr.sort( compare( "Field" ) );
   for( var i = 0; i < autoIncrementArr.length; i++ )
   {
      var tmpActObj = sortJsonKeys( autoIncrementArr[i] );
      delete tmpActObj.SequenceID;
      var tmpExpObj = sortJsonKeys( expArr[i] );
      assert.equal( tmpActObj, tmpExpObj );
   }

}

/*******************************************************************************
@Description : 校验集合sequence属性
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkSequence ( db, sequenceName, expObj )
{
   if( expObj.Increment == undefined ) { expObj.Increment = 1; }
   if( expObj.StartValue == undefined ) { expObj.StartValue = 1; }
   if( expObj.MinValue == undefined ) { expObj.MinValue = 1; }
   if( expObj.MaxValue == undefined ) { expObj.MaxValue = { $numberLong: "9223372036854775807" }; }
   if( expObj.CacheSize == undefined ) { expObj.CacheSize = 1000; }
   if( expObj.AcquireSize == undefined ) { expObj.AcquireSize = 1000; }
   if( expObj.Cycled == undefined ) { expObj.Cycled = false; }
   if( expObj.CurrentValue == undefined ) { expObj.CurrentValue = 1; }

   var selObj = { Increment: null, StartValue: null, MinValue: null, MaxValue: null, CacheSize: null, AcquireSize: null, Cycled: null, CurrentValue: null };
   var sequenceObj = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName }, selObj ).next().toObj();

   var tmpActObj = sortJsonKeys( sequenceObj );
   var tmpExpObj = sortJsonKeys( expObj );

   assert.equal( tmpActObj, tmpExpObj );
}

/*******************************************************************************
@Description : 校验从节点返回记录数
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkCountFromNode ( db, groupName, csName, clName, expCount )
{
   var rg = db.getRG( groupName );
   var data = rg.getMaster().connect();
   var dataCL = data.getCS( csName ).getCL( clName );
   var count = dataCL.count();
   assert.equal( count, expCount );
}

/*******************************************************************************
@Description : 插入其他不支持类型的记录
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function insertOtherTypeDatas ( dbcl, arr )
{
   for( var i = 0; i < arr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.insert( arr[i] );
      } );
   }
}

/*******************************************************************************
@Description : 指定options参数为不支持类型创建自增字段
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function create ( dbcl, options, isLegal )
{
   try
   {
      dbcl.createAutoIncrement( options );
      if( !isLegal )
      {
         throw new Error( "NEED_ERROR" );
      }
   }
   catch( e )
   {
      if( !isLegal )
      {
         if( e.message != SDB_INVALIDARG )
         {
            throw e;
         }
      } else
      {
         throw e;
      }
   }
}

/*******************************************************************************
@Description : 插入数据并返回预期结果
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function insertAndGetExpList ( cl, increment_1, increment_2, currentValue_1, currentValue_2, expList, insertCount )
{
   for( var i = 0; i < 3; i++ )
   {
      cl.insert( { a: i } );
      currentValue_1 = currentValue_1 + increment_1;
      currentValue_2 = currentValue_2 + increment_2;
      expList.push( { a: i, id1: currentValue_1, id2: currentValue_2 } );
      insertCount.count++;
   }
   expList.sort( compare( "id1" ) );
   return expList;
}

/*******************************************************************************
@Description : 检查自增字段的lastGenerateID值
@return: 
@Modify list : 2020-05-08 liuxiaoxuan
*******************************************************************************/
function checkLastGenerateID ( actLastGenerateID, expLastGenerateID )
{
   if( expLastGenerateID !== actLastGenerateID )
   {
      throw new Error( "actual LastGenerateID: " + actLastGenerateID + ", expect LastGenerateID: " + expLastGenerateID );
   }
}
