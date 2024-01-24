/*******************************************************************************
*@Description: findandremove basic testcases
*@Modify list:
*   2014-3-4 wenjing wang  Init
*******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var dataGroupNum = 0;

if( "undefined" == typeof ( errMsg ) )
{
   var errMsg =
   {
      FISRTPARAM: "SdbQuery.update(): the 1st param should be non-empty object",
      SECONDPARAM: "SdbQuery.update(): the 2nd param should be boolean",
      WITHCOUNT: "count() cannot be executed with update() or remove()"
   }
}

// 装载多条数据
function loadMultipleDoc ( cl, number )
{
   if( undefined == number )
   {
      number = 1000;
   }

   var j = 0;
   var subcldocnum = 5;
   if( 0 != dataGroupNum )
   {
      subcldocnum = subcldocnum * commGetGroupsNum( db )
   }

   for( var i = 0; i < number; ++i )
   {
      if( i % subcldocnum == 0 &&
         i != 0 )
      {
         j = j + 1;
      }
      cl.insert( { _id: i, a: i, date: 20150000 + ( j % 4 + 1 ) * 100 + i % 28 + 1 } )
   }
}

function buildoption ( ismaincl, groupname )
{
   var option = new Object();
   if( undefined != groupname )
   {
      option.Group = groupname;
   }

   option.ShardingType = "range";
   var shardingkey = new Object();
   if( undefined != ismaincl &&
      true == ismaincl )
   {
      option.IsMainCL = ismaincl
      shardingkey.date = 1;
   }
   else
   {
      shardingkey._id = 1;
   }

   option.ShardingKey = shardingkey;
   return option;
}

// 切分表
function splittable ( db, cl, clname )
{
   var fullname = COMMCSNAME + "." + clname;
   var srcgroups = commGetCLGroups( db, fullname );
   if( srcgroups.length > 1 )
   {
      return srcgroups.length;
   }

   var datagroups = commGetGroups( db, true );
   dataGroupNum = datagroups.length;
   var startid = 0;
   var endid = 5;
   for( var i = 0; i < datagroups.length; ++i )
   {
      if( datagroups[i][0].GroupName != srcgroups[0] )
      {
         var taskId = cl.splitAsync( srcgroups[0], datagroups[i][0].GroupName, { _id: startid }, { _id: endid } );
         startid = endid;
         endid += 5;
         db.waitTasks( taskId );
      }
   }
   return datagroups.length;
}

//检查删除结果
function checkRemoveResult ( cl, arr )
{
   for( var i = 0; i < arr.length; ++i )
   {
      var doc = eval( "( " + arr[i] + " )" );
      recordnumber = cl.find( { _id: doc["_id"], b: 1 } ).count();
      if( 0 != parseInt( recordnumber ) )
      {
         return false;
      }
   }

   return true;
}

//检查更新结果
function checkUpdateResult ( cl, arr )
{
   for( var i = 0; i < arr.length; ++i )
   {
      var doc = eval( "( " + arr[i] + " )" );
      recordnumber = cl.find( { _id: doc["_id"], b: 1 } ).count();
      if( 1 != parseInt( recordnumber ) )
      {
         return false;
      }
   }

   return true;
}

// 创建子表，并且挂到相应主表下
function init ( db, maincl, clName, isneedsplit, groupname )
{
   var ismaincl = false;
   var start = 20150101;
   dataGroupNum = 0; // 不切分的情况下，不考虑多个组，在这重置

   for( var i = 0; i < 4; ++i )
   {
      var subclname = clName + "sub";
      subclname = subclname + i;
      var subcl = commCreateCL( db, COMMCSNAME, subclname, buildoption( ismaincl, groupname ), true );
      var fullname = COMMCSNAME + "." + subclname;
      maincl.attachCL( fullname, { LowBound: { date: start + ( i * 100 ) }, UpBound: { date: start + ( ( i + 1 ) * 100 ) } } );

      if( undefined != isneedsplit &&
         true == isneedsplit )
      {
         splittable( db, subcl, subclname );
      }
   }
}

// 结束时，释放所有子表
function fini ( db, clName )
{
   for( var i = 0; i < 4; ++i )
   {
      var subclname = clName + "sub"
      subclname = subclname + i;
      commDropCL( db, COMMCSNAME, subclname );
   }
}
