/**************************************
 * @Description: 覆盖单分区键
 * @author: ouyangzhongnan 
 * @Date: 2016-11-23
 * @coverTestcace:  (主表为单分区情况)
 *       seqDB-10458:查询数据在不同组上的不同子表中，查询并排序
 *       seqDB-10459:查询数据在不同组上的相同子表中，查询并排序
 *       seqDB-10460:查询数据在相同组上的不同子表中，查询并排序
 *       seqDB-10461:查询数据在相同组上的相同子表中，查询并排序             
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_querySort_10458.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

main( test );
function test ()
{

   var mainCl = null;
   var data = [];
   var mainClName = CHANGEDPREFIX + "maincl_10458";
   var subClNames = [
      CHANGEDPREFIX + "subcl_10458_0",
      CHANGEDPREFIX + "subcl_10458_1",
      CHANGEDPREFIX + "subcl_10458_2",
      CHANGEDPREFIX + "subcl_10458_3"
   ];

   //check test environment before split
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   /* =====================================*/
   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[2], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[3], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );

   //create maincl for range split
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { IsMainCL: true, ShardingKey: { a: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   mainCl = commCreateCL( db, COMMCSNAME, mainClName, mainCLOption, true, true );
   var subCLOptions = [
      { ReplSize: 0, Compressed: true },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1 }, ShardingType: "range" },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 16 },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1 }, ShardingType: "range" }
   ];

   //create subcl
   commCreateCL( db, COMMCSNAME, subClNames[0], subCLOptions[0], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[1], subCLOptions[1], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[2], subCLOptions[2], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[3], subCLOptions[3], true, true );

   //attach subcl
   attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 400 }, UpBound: { a: 300 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 300 }, UpBound: { a: 200 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[2], { LowBound: { a: 200 }, UpBound: { a: 100 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[3], { LowBound: { a: 100 }, UpBound: { a: 0 } } );

   //init data
   var aVal;
   for( var i = 0; i < 1000; i++ )
   {
      aVal = i % 400 + 1;
      data.push( { name: "name" + i, a: aVal, b: i } );
   }
   mainCl.insert( data );
   //split cl
   ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
   ClSplitOneTimes( COMMCSNAME, subClNames[2], 50, null );
   ClSplitOneTimes( COMMCSNAME, subClNames[3], 50, null );

   var flag = true;//定义一个标识，用于判断结果对错
   /*==查询所有数据并排序，排序字段包含全部分区键==*/
   //正序
   var cursor = mainCl.find().sort( { a: 1 } );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareNum = cursor.next().toObj().a;
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareNum = cursor.next().toObj().a;
         if( compareNum > toCompareNum ) 
         {
            flag = false;
            break;
         }
         else 
         {
            compareNum = toCompareNum;
         }
      }
   }

   assert.equal( flag, true );

   //逆序
   flag = true;
   var cursor = mainCl.find().sort( { a: -1 } );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareNum = cursor.next().toObj().a;
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareNum = cursor.next().toObj().a;
         if( compareNum < toCompareNum ) 
         {
            flag = false;
            break;
         }
         else 
         {
            compareNum = toCompareNum;
         }
      }
   }

   assert.equal( flag, true );

   /*=查询所有数据并排序，排序字段不包含分区键=*/
   //正序
   flag = true;
   var cursor = mainCl.find().sort( { b: 1 } );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareNum = cursor.next().toObj().b;
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareNum = cursor.next().toObj().b;
         if( compareNum > toCompareNum ) 
         {
            flag = false;
            break;
         }
         else 
         {
            compareNum = toCompareNum;
         }
      }
   }

   assert.equal( flag, true );

   //逆序
   flag = true;
   var cursor = mainCl.find().sort( { b: -1 } );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareNum = cursor.next().toObj().b;
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareNum = cursor.next().toObj().b;
         if( compareNum < toCompareNum ) 
         {
            flag = false;
            break;
         }
         else 
         {
            compareNum = toCompareNum;
         }
      }
   }
   assert.equal( flag, true );

   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}