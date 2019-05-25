//@ sourceURL=other/Index.js
// --------------------- Data.Overview.Index ---------------------
var _DataOverviewIndex = {} ;

//获取集合列表的详细信息
_DataOverviewIndex.getCLInfo = function( $scope, SdbRest, index, moduleName, moduleMode, clusterName ){
   var sql ;
   if( moduleMode == 'standalone' )
   {
      sql = 'SELECT t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalDataPages, t1.Details.TotalIndexPages, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS t1' ;
   }
   else
   {
      /*sql = 'SELECT t2.Name as FullName, t1.Details.ID, t1.Details.LogicalID, t1.Details.Sequence, t1.Details.GroupName, t1.Details.Status, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalDataPages, t1.Details.TotalIndexPages, t1.Details.TotalLobPages, t1.Details.TotalDataFreeSpace/1048576, t1.Details.TotalIndexFreeSpace/1048576, t2.IsMainCL, t2.MainCLName, t2.ShardingKey, t2.ShardingType FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1 RIGHT OUTER JOIN $SNAPSHOT_CATA AS t2 ON t1.Name = t2.Name' ;*/
      sql = 'SELECT t1.Name, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalDataPages, t1.Details.TotalIndexPages, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1' ;
   }
   //合并cl数据
   function mergedData( clList, cataList ){
      var newClList = []
      $.each( clList, function( index, clInfo ){
         var hasFind = false ;
         $.each( newClList, function( index3, newInfo ){
            if( newInfo['Name'] == clInfo['Name'] )
            {
               newInfo['TotalRecords'] += clInfo['TotalRecords'] ;
               hasFind = true ;
            }
         } ) ;
         if( hasFind == false )
         {
            newClList.push( clInfo ) ;
         }
      } ) ;
      $.each( cataList, function( index, cataInfo ){
         if( cataInfo['IsMainCL'] == true )
         {
            newClList.push( cataInfo ) ;
         }
      } ) ;
      return newClList ;
   }
   //查询成功执行的
   function success( data ){
      if( data.length == 1 && data[0]['Name'] == null ) data = [] ;
      $scope.QueryModule[index]['status1'] = $scope.autoLanguage( '良好' )  ;
      $scope.QueryModule[index]['status2'] = $scope.autoLanguage( '业务运行正常' )  ;
      $scope.QueryModule[index]['errno'] = 0 ;
      $scope.QueryModule[index]['description'] = '' ;
      var sumRecords = 0 ;
      var clList = data ;
      var lobNum = 0 ;
      var dataNum = 0 ;
      var indexNum = 0 ;
      $.each( clList, function( index2, clInfo ){
         if( !isNaN( clInfo['TotalRecords'] ) )
         {
            sumRecords += clInfo['TotalRecords'] ;
         }
         if( !isNaN( clInfo['TotalLobPages'] ) )
         {
            lobNum += clInfo['TotalLobPages'] ;
         }
         if( !isNaN( clInfo['TotalDataPages'] ) )
         {
            dataNum += clInfo['TotalDataPages'] ;
         }
         if( !isNaN( clInfo['TotalIndexPages'] ) )
         {
            indexNum += clInfo['TotalIndexPages'] ;
         }
      } ) ;
      var sum = lobNum + dataNum + indexNum ;
      var lobPercent = 0 ;
      var dataPercent = 0 ;
      var indexPercent = 0 ;
      if( sum > 0 )
      {
         lobPercent = ( lobNum / sum ).toFixed( 2 ) ;
         dataPercent = ( dataNum / sum ).toFixed( 2 ) ;
         indexPercent = ( indexNum / sum ).toFixed( 2 ) ;
         $scope.QueryModule[index]['info'] = [] ;
         if( lobPercent > 0 )
         {
            $scope.QueryModule[index]['info'].push( { name: 'Lob', color: '#68DEAB', percent: lobPercent } ) ;
         }
         if( dataPercent > 0 )
         {
            $scope.QueryModule[index]['info'].push( { name: 'Data', color: '#84BAE7', percent: dataPercent } ) ;
         }
         if( indexPercent > 0 )
         {
            $scope.QueryModule[index]['info'].push( { name: 'Index', color: '#EDCC96', percent: indexPercent } ) ;
         }
      }
      else
      {
         $scope.QueryModule[index]['info'] = [ { name: 'Empty', color: '#DCDCDC', percent: 1 } ] ;
      }
      $scope.QueryModule[index]['chart'] = {} ;
      $scope.QueryModule[index]['chart']['options'] = window.SdbSacManagerConf.recordEchart ;
      if( $scope.lastRecord[index] >= 0 && sumRecords - $scope.lastRecord[index] >= 0 )
      {
         $scope.QueryModule[index]['chart']['value'] = [ [ 0, sumRecords - $scope.lastRecord[index], true, false ] ] ;
      }
      else
      {
         $scope.QueryModule[index]['chart']['value'] = [ [ 0, 0, true, false ] ] ;
      }
      $scope.lastRecord[index] = sumRecords ;
      $scope.QueryModule[index]['detail'] = sprintf( $scope.autoLanguage( '一共 ? 个集合， ? 条记录' ), clList.length, sumRecords ) ;
      $scope.$apply() ;
      if( $scope.Url.Module == 'Data' && $scope.Url.Action == 'Overview' && $scope.Url.Method == 'Index' )
      {
         setTimeout( function(){
            _DataOverviewIndex.getCLInfo( $scope, SdbRest, index, moduleName, moduleMode, clusterName )
         }, 5000 ) ;
      }
   }
   //获取集合列表
   SdbRest.Exec2( clusterName, moduleName, sql, {
      'success': function( data ){
         if( moduleMode == 'standalone' )
         {
            success( data ) ;
         }
         else
         {
            sql = 'SELECT * FROM $SNAPSHOT_CATA WHERE IsMainCL=true or MainCLName>"" or ShardingType>""' ;
            SdbRest.Exec2( clusterName, moduleName, sql, {
               'success': function( data2 ){
                  var newData = mergedData( data, data2 ) ;
                  success( newData ) ;
               },
               'failed': function( errorInfo ){
                  $scope.QueryModule[index]['status1'] = $scope.autoLanguage( '错误' )  ;
                  $scope.QueryModule[index]['status2'] = $scope.autoLanguage( '业务运行错误' )  ;
                  $scope.QueryModule[index]['detail'] = '' ;
                  $scope.QueryModule[index]['errno'] = errorInfo['errno'] ;
                  $scope.QueryModule[index]['description'] = sprintf( $scope.autoLanguage( '错误码: ?, ?。' ), errorInfo['errno'], errorInfo['description'] ) ;
                  setTimeout( function(){
                     _DataOverviewIndex.getCLInfo( $scope, SdbRest, index, moduleName, moduleMode, clusterName )
                  }, 5000 ) ;
               }
            }, {
               'showLoading': false
            } ) ;
         }
      },
      'failed': function( errorInfo ){
         $scope.QueryModule[index]['status1'] = $scope.autoLanguage( '错误' )  ;
         $scope.QueryModule[index]['status2'] = $scope.autoLanguage( '业务运行错误' )  ;
         $scope.QueryModule[index]['detail'] = '' ;
         $scope.QueryModule[index]['errno'] = errorInfo['errno'] ;
         $scope.QueryModule[index]['description'] = sprintf( $scope.autoLanguage( '错误码: ?, ?。' ), errorInfo['errno'], errorInfo['description'] ) ;
         setTimeout( function(){
            _DataOverviewIndex.getCLInfo( $scope, SdbRest, index, moduleName, moduleMode, clusterName )
         }, 5000 ) ;
      }
   }, {
      'showLoading': false
   } ) ;
}

//获取业务列表
_DataOverviewIndex.getModuleList = function( $scope, SdbRest, clusterName ){
   var data = { 'cmd': 'query business', 'filter': JSON.stringify( { 'ClusterName' : clusterName } ) } ;
   SdbRest.OmOperation( data, {
      'success': function( json ){
         $.each( json, function( index ){
            var i = index ;
            if( index > 3 ) i = index % 4 ;
            if( i == 0 ) json[index]['color'] = 'green' ;
            if( i == 1 ) json[index]['color'] = 'yellow' ;
            if( i == 2 ) json[index]['color'] = 'blue' ;
            if( i == 3 ) json[index]['color'] = 'violet' ;
            json[index]['detail'] = $scope.autoLanguage( '正在加载...' ) ;
            json[index]['errno'] = 0 ;
            json[index]['description'] = '' ;
            json[index]['status1'] = '' ;
            json[index]['status2'] = '' ;
            json[index]['info'] = [] ;
            $scope.lastRecord.push( -1 ) ;
         } ) ;
         $scope.QueryModule = json ;
         $.each( $scope.QueryModule, function( index, moduleInfo ){
            moduleInfo['WebDeployMod'] = moduleInfo.DeployMod ;
            $.each( $scope.moduleTemplate, function( index2, templateInfo ){
               if( moduleInfo.DeployMod == templateInfo.DeployMod )
               {
                  moduleInfo['WebDeployMod'] = templateInfo.WebName ;
                  return false ;
               
               }
            } ) ;
            _DataOverviewIndex.getCLInfo( $scope, SdbRest,index, moduleInfo.BusinessName, moduleInfo.DeployMod, clusterName ) ;
         } ) ;
         $scope.$apply() ;
      },
      'failed': function( errorInfo ){
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '获取业务列表失败。' ) ) ;
      }
   }, {
      'showLoading': false
   } ) ;
}

//获取业务模板
_DataOverviewIndex.getModuleTemplate = function( $scope, SdbRest, clusterName ){
   var data = { 'cmd': 'get business template', 'BusinessType': 'sequoiadb' } ;
   SdbRest.OmOperation( data, {
      'success': function( json ){
         $scope.moduleTemplate = json ;
         _DataOverviewIndex.getModuleList( $scope, SdbRest, clusterName ) ;
      }, 
      'failed': function( errorInfo ){
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '获取业务列表失败。' ) ) ;
      }
   } ) ;
}

//跳转到数据库页面
_DataOverviewIndex.gotoDatabase = function( $scope, $location, SdbFunction, moduleIndex ){
   var moduleName = $scope.QueryModule[moduleIndex].BusinessName ;
   var moduleMode = $scope.QueryModule[moduleIndex].DeployMod ;
   SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
   SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
   $location.path( 'Data/Database/Index' ).search( { 'r': new Date().getTime() } ) ;
}