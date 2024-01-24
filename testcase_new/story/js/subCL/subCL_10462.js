/**************************************
 * @author: ouyangzhongnan 
 * @coverTestcace: 
 *       seqDB-10462:hint指定索引查询
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_queryByHint_10462.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

var mainCl = null;
var subCls = [];
var data = [];
var dataA50B50;
var dataA150B50;
var indexName = CHANGEDPREFIX + "abIndex";
var mainClName = CHANGEDPREFIX + "maincl_10462";
var subClNames = [
   CHANGEDPREFIX + "subcl_10462_0",
   CHANGEDPREFIX + "subcl_10462_1"
];


main( test );
function test ()
{

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

   // drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
   //create maincl and subcl,attach subcl to maincl
   db.setSessionAttr( { PreferedInstance: "M" } );
   mainCl = commCreateCL( db, COMMCSNAME, mainClName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true }, true, true );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0] ) );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[1], { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, Partition: 16 }, true, true ) );
   //attach subcl
   attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   mainCl.createIndex( indexName, { a: 1, b: 1 }, true );
   initData();
   queryByHint( { a: 50, b: 50 }, dataA50B50 );
   queryByHint( { a: 150, b: 50 }, dataA150B50 );
   end();
}

function end ()
{
   // drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}

/**
 * [initData 初始化查询数据]
 */
function initData ()
{
   for( var i = 0; i < 200; i++ )
   {
      data.push( { a: i, b: i % 100, c: i } );
      if( i === 50 ) dataA50B50 = { a: i, b: i % 100, c: i };
      if( i === 150 ) dataA150B50 = { a: i, b: i % 100, c: i };
   }
   mainCl.insert( data );
   //split cl
   ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
}

/**
 * [queryByHint 按索引查询]
 */
function queryByHint ( queryCond, res )
{
   //先判断是不是走索引
   var flag_0 = true;
   var explainArr = mainCl.find( queryCond ).hint( { "": indexName } ).explain().toArray();
   var explainObjArr = [];
   for( var i = 0; i < explainArr.length; i++ )
   {
      explainObjArr.push( JSON.parse( explainArr[i] ) );
   }
   for( var i = 0; i < explainObjArr.length; i++ )
   {
      for( var j = 0; j < explainObjArr[i].SubCollections.length; j++ )
      {
         if( explainObjArr[i].SubCollections[j].ScanType !== "ixscan"
            || explainObjArr[i].SubCollections[j].IndexName !== indexName )
         {
            flag_0 = false;
            break;
         }
      }
   }

   //查询出的结果是否正确
   var flag_1 = true;
   var query_res = mainCl.find( queryCond ).next().toObj();
   delete query_res["_id"];

   var actA = query_res["a"];
   var actB = query_res["b"];
   var tmpStr = JSON.stringify( queryCond );
   var tmpObj = eval( '(' + tmpStr + ')' );
   var expA = tmpObj["a"];
   var expB = tmpObj["b"];
   if( actA !== expA || actB !== expB ) flag_1 = false;

   if( !( flag_0 && flag_1 ) )
   {
      throw new Error( "query result is false or not by index" );
   }
}