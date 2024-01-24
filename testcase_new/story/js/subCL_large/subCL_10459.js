/**************************************
 * @Description: 覆盖多分区键
 * @author: ouyangzhongnan 
 * @Date: 2016-11-23
 * @coverTestcace:  (主表为多分区情况)
 *       seqDB-10458:查询数据在不同组上的不同子表中，查询并排序
 *       seqDB-10459:查询数据在不同组上的相同子表中，查询并排序
 *       seqDB-10460:查询数据在相同组上的不同子表中，查询并排序
 *       seqDB-10461:查询数据在相同组上的相同子表中，查询并排序             
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_querySort_10459.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

main( test );
function test ()
{
   var mainCl = null;
   var data = [];
   var mainClName = CHANGEDPREFIX + "_maincl_10459";
   var subClNames = [
      CHANGEDPREFIX + "_subcl_10459_0",
      CHANGEDPREFIX + "_subcl_10459_1",
      CHANGEDPREFIX + "_subcl_10459_2",
      CHANGEDPREFIX + "_subcl_10459_3",
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

   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[2], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[3], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );

   //create maincl for range split
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { IsMainCL: true, ShardingKey: { a: 1, b: -1, c: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   mainCl = commCreateCL( db, COMMCSNAME, mainClName, mainCLOption, true, true );
   var subCLOptions = [
      { ReplSize: 0, Compressed: true },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1, b: 1 }, ShardingType: "hash", Partition: 16 },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1, b: 1 }, ShardingType: "range" },
      { ReplSize: 0, Compressed: true, ShardingKey: { a: 1, b: 1 }, ShardingType: "hash", Partition: 16 }
   ];

   //create subcl
   commCreateCL( db, COMMCSNAME, subClNames[0], subCLOptions[0], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[1], subCLOptions[1], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[2], subCLOptions[2], true, true );
   commCreateCL( db, COMMCSNAME, subClNames[3], subCLOptions[3], true, true );

   //attach subcl
   attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0, b: 400, c: 0 }, UpBound: { a: 100, b: 300, c: 100 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100, b: 300, c: 100 }, UpBound: { a: 200, b: 200, c: 200 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[2], { LowBound: { a: 200, b: 200, c: 200 }, UpBound: { a: 300, b: 100, c: 300 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[3], { LowBound: { a: 300, b: 100, c: 300 }, UpBound: { a: 400, b: 0, c: 400 } } );

   //init data
   var aVal;
   var bVal;
   var cVal;
   for( var i = 0; i < 1000; i++ ) 
   {
      //添加混乱的数据
      aVal = parseInt( Math.random() * 1000 + 1 ) % 400 + 1;
      bVal = parseInt( Math.random() * 1000 + 1 ) % 400 + 1;
      cVal = parseInt( ( aVal + bVal ) / 2 );
      data.push( { a: aVal, b: bVal, c: cVal, d: i } );
   }

   mainCl.insert( data );

   //split
   ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
   ClSplitOneTimes( COMMCSNAME, subClNames[2], 50, null );
   ClSplitOneTimes( COMMCSNAME, subClNames[3], 50, null );

   var flag = true;//定义一个标识，用于判断对错
   /**=查询所有数据并排序，排序字段包含所有分区键=*/
   //正序 and 逆序
   var sortOptions = { a: 1, b: -1, c: 1 };
   var cursor = mainCl.find().sort( sortOptions );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareObj = cursor.next().toObj();
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareObj = cursor.next().toObj();
         if( compareJSONObj( compareObj, toCompareObj, sortOptions ) < 0 ) 
         {
            flag = false;
            break;
         }
         compareObj = toCompareObj;
      }
   }

   if( !flag ) 
   {
      throw new Error( "sort by " + JSON.stringify( sortOptions ) + " is false" );
   }

   /**=查询所有数据并排序，排序字段包含部分分区键=*/
   //正序 and 逆序
   var sortOptions = { a: 1, c: -1 };
   var cursor = mainCl.find().sort( sortOptions );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareObj = cursor.next().toObj();
      for( var i = 1; i < cursor.count(); i++ )
      {
         var toCompareObj = cursor.next().toObj();
         if( compareJSONObj( compareObj, toCompareObj, sortOptions ) < 0 )
         {
            flag = false;
            break;
         }
         compareObj = toCompareObj;
      }
   }

   if( !flag ) 
   {
      throw new Error( "sort by " + JSON.stringify( sortOptions ) + " is false" );
   }

   /**=查询所有数据并排序，排序字段不包含分区键=*/
   //正序
   var sortOptions = { d: 1 };
   var cursor = mainCl.find().sort( sortOptions );
   var flag = true;
   if( cursor.count() != data.length )
   {
      flag = false;
   }
   else 
   {
      var compareObj = cursor.next().toObj();
      for( var i = 1; i < cursor.count(); i++ )
      {
         var toCompareObj = cursor.next().toObj();
         if( compareJSONObj( compareObj, toCompareObj, sortOptions ) < 0 ) 
         {
            flag = false;
            break;
         }
         compareObj = toCompareObj;
      }
   }

   if( !flag ) 
   {
      throw new Error( "sort by " + JSON.stringify( sortOptions ) + " is false" );
   }

   //逆序
   var sortOptions = { d: -1 };
   var cursor = mainCl.find().sort( sortOptions );
   if( cursor.count() != data.length ) 
   {
      flag = false;
   }
   else 
   {
      var compareObj = cursor.next().toObj();
      for( var i = 1; i < cursor.count(); i++ ) 
      {
         var toCompareObj = cursor.next().toObj();
         if( compareJSONObj( compareObj, toCompareObj, sortOptions ) < 0 ) 
         {
            flag = false;
            break;
         }
         compareObj = toCompareObj;
      }
   }

   if( !flag ) 
   {
      throw new Error( "sort by " + JSON.stringify( sortOptions ) + " is false" );
   }

   //drop subcl and maincl
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}