/******************************************************************************
 * @Description   : seqDB-22834 :: 创建包含多个有效地址的数据源
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.09
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22834";
   var csName = "cs_22834";
   var clName = "cl_22834";
   var srcCSName = "datasrcCS_22834";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );
   var datasrcUrls = getCoordUrl( datasrcDB );
   db.createDataSource( dataSrcName, datasrcUrls.toString(), userName, passwd );
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   datasrcDB.dropCS( srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}


