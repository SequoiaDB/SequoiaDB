/******************************************************************************
 * @Description   : seqDB-23295:指定SdbSnapshotOption参数查询数据源
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23295_";
   var dataSrcName1 = "datasrc23295_1";
   var limitNum = 10;
   var skipNum = 5;

   for( var i = 0; i < 30; i++ )
   {
      clearDataSource( "nocs", dataSrcName + i + "" );
      db.createDataSource( dataSrcName + i + "", datasrcUrl, userName, passwd );
   }

   // cond
   var option = new SdbSnapshotOption().cond( { Name: dataSrcName1 } );
   var actualResults = db.listDataSources( { Name: dataSrcName1 } );
   listDataSourcesAndCheck( option, actualResults );

   // cond、sel
   var option = new SdbSnapshotOption().cond( { Name: dataSrcName1 } ).sel( { DSVersion: "" } );
   var actualResults = db.listDataSources( { Name: dataSrcName1 }, { DSVersion: "" } );
   listDataSourcesAndCheck( option, actualResults );

   // cond、sort 
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sort( { Name: 1 } );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, {}, { Name: 1 } );
   listDataSourcesAndCheck( option, actualResults );

   // cond、sort、skip
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sort( { Name: -1 } ).skip( skipNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, {}, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, skipNum );

   // cond、sort、limit
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sort( { Name: -1 } ).limit( limitNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, {}, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, 0, limitNum + 1 );

   // cond、sel、sort、skip
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sel( { Name: "" } ).sort( { Name: -1 } ).skip( skipNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, { Name: "" }, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, skipNum );

   // cond、sel、sort、limit
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sel( { Name: "" } ).sort( { Name: -1 } ).limit( limitNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, { Name: "" }, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, 0, limitNum + 1 );

   // cond、sort、limit、skip
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sort( { Name: -1 } ).limit( limitNum ).skip( skipNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, {}, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, skipNum, limitNum + skipNum + 1 );

   // cond、sel、sort、limit、skip
   var option = new SdbSnapshotOption().cond( { Name: { $regex: "datasrc23295_*" } } ).sel( { Name: "" } ).sort( { Name: -1 } ).limit( limitNum ).skip( skipNum );
   var actualResults = db.listDataSources( { Name: { $regex: "datasrc23295_*" } }, { Name: "" }, { Name: -1 } );
   listDataSourcesAndCheck( option, actualResults, skipNum, limitNum + skipNum + 1 );

   for( var i = 0; i < 30; i++ )
   {
      var dataSrcName2 = dataSrcName + i;
      clearDataSource( "nocs", dataSrcName2 );
   }
}

function listDataSourcesAndCheck ( option, actualResults, minnum, maxnum )
{
   var expectResult = [];
   var num = 0;
   var cursor = db.list( SDB_LIST_DATASOURCES, option );
   while( actualResults.next() )
   {
      num++;
      if( isRangeIn( num, minnum, maxnum ) )
      {
         var actualResultsInfo = actualResults.current().toObj();
         expectResult.push( actualResultsInfo );
      }
   }
   actualResults.close();
   commCompareResults( cursor, expectResult, false );
}

function isRangeIn ( num, minnum, maxnum )
{
   if( minnum == undefined ) { minnum = 0; }
   if( maxnum == undefined ) { maxnum = 31; }
   if( num < maxnum && num > minnum )
   {
      return true;
   }
   return false;
}