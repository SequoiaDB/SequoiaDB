//@ sourceURL=other/Index.js
// --------------------- Data.Operate.Index ---------------------
var _DataOperateIndex = {} ;

//初始化
_DataOperateIndex.init = function( $scope, moduleName, moduleMode ){
   //cl列表信息
   $scope.clsInfo = [] ;
   //每一页的最大记录数
   $scope.limit = 30 ;
   //显示的当前页
   $scope.setCurrent = 1 ;
   //真实的当前页
   $scope.current = 1 ;
   //总页数
   $scope.total = 0 ;
   //业务名
   $scope.moduleName = moduleName ;
   //业务类型
   $scope.moduleMode = moduleMode ;
}

//显示指定页
_DataOperateIndex.showPage = function( $scope, $compile, pageNum ){
   var newClList = [] ;
   var startOfNum = $scope.limit * ( pageNum - 1 ) ;
   var endOfNum = $scope.limit * pageNum ;
   endOfNum = ( endOfNum > $scope.clsInfo.length ? $scope.clsInfo.length : endOfNum ) ;
   for( var i = startOfNum; i < endOfNum ; ++i )
   {
      newClList.push( $scope.clsInfo[i] ) ;
   }
   var gridData = {
      'title': [],
      'body': [],
      'tool': {
         'position': 'bottom',
         'left': [
            { 'html': $compile( '<i class="fa fa-refresh" ng-click="getCLList()"></i>' )( $scope ).attr( 'data-desc', $scope.autoLanguage( '刷新' ) ) },
            { 'html': $compile( '<i class="fa fa-play fa-flip-horizontal" ng-show="current > 1" ng-click="previous()"></i>' )( $scope ) },
            { 'html': $compile( '<input style="width:100px;" ng-change="checkCurrent()" ng-model="setCurrent" ng-keypress="gotoPate($event)">' )( $scope ) },
            { 'html': $compile( '<span>/<span>' )( $scope ) },
            { 'html': $compile( '<span ng-bind="total"></span>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-play" ng-show="current < total" ng-click="nextPage()"></i>' )( $scope ) }
         ],
         'right': [
            { 'text': sprintf( $scope.autoLanguage( '一共 ? 个集合。' ), $scope.clsInfo.length ) }
         ]
      },
      'options': {
         'order': { 'active': true },
         'grid': {
            'tdModel': 'fixed',
            'tdHeight': '19px',
            'gridModel': 'fixed'
         }
      }
   } ;
   var gridTitle = [] ;
   if( $scope.moduleMode == 'standalone' )
   {
      gridTitle = [
         "",
         $scope.autoLanguage( '集合空间' ),
         $scope.autoLanguage( '集合' ),
         $scope.autoLanguage( '记录数' ),
         $scope.autoLanguage( 'Lob数' )
      ] ;
   }
   else if( $scope.moduleMode == 'distribution' )
   {
      gridTitle = [
         "",
         $scope.autoLanguage( '集合空间' ),
         $scope.autoLanguage( '集合' ),
         $scope.autoLanguage( '分区类型' ),
         $scope.autoLanguage( '记录数' ),
         $scope.autoLanguage( 'Lob数' )
      ] ;
   }
   $.each( gridTitle, function( index, titleText ){
      gridData['title'].push( { 'text': titleText } ) ;
   } ) ;
   $.each( newClList, function( index, clInfo ){
      var shardingType ;
      var shardingTypeDesc ;
      if( $scope.moduleMode == 'distribution' )
      {
         if( clInfo['IsMainCL'] == true )
         {
            shardingType = $scope.autoLanguage( '垂直' ) ;
            shardingTypeDesc = $scope.autoLanguage( '垂直分区' ) ;
         }
         else
         {
            if( clInfo['ShardingType'] == 'range' )
            {
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平范围分区' ) ;
            }
            else if( clInfo['ShardingType'] == 'hash' )
            {
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平散列分区' ) ;
            }
            else
            {
               shardingType = $scope.autoLanguage( '普通' ) ;
               shardingTypeDesc = $scope.autoLanguage( '普通' ) ;
            }
         }
      }
      var fullName = clInfo['Name'].split( '.' ) ;
      var csName = fullName[0] ;
      var clName = fullName[1] ;
      var newEle = $compile( '<a class="linkButton" ng-click="gotoRecord(' + ( index + $scope.limit * ( pageNum - 1 ) ) + ')"></a>' )( $scope ).text( clName ) ;
      var newEle2 = null ;
      var newEle3 = null ;
      if( clInfo['TotalRecords'] != null )
      {
         newEle2 = $compile( '<a class="linkButton" ng-click="gotoRecord(' + ( index + $scope.limit * ( pageNum - 1 ) ) + ')"></a>' )( $scope ).text( clInfo['TotalRecords'] ) ;
      }
      if( clInfo['IsMainCL'] != true )
      {
         newEle3 = $compile( '<a class="linkButton" ng-click="gotoLob(' + ( index + $scope.limit * ( pageNum - 1 ) ) + ')"></a>' )( $scope ).text( '-' ) ;
      }
      if( $scope.moduleMode == 'standalone' )
      {
         gridData['body'].push( [
            { 'text': index + 1 },
            { 'text': csName },
            { 'html': newEle },
            { 'html': newEle2 },
            { 'html': newEle3 }
         ] ) ;
      }
      else if( $scope.moduleMode == 'distribution' )
      {
         var shardingTypeEle = $( '<span></span>' ).attr( 'data-desc', shardingTypeDesc ).addClass( 'badge badge-info' ).text( shardingType ) ;
         gridData['body'].push( [
            { 'text': index + 1 },
            { 'text': csName },
            { 'html': newEle },
            { 'html': shardingTypeEle },
            { 'html': newEle2 },
            { 'html': newEle3 }
         ] ) ;
      }
   } ) ;
   $scope.clGridData = gridData ;
}

//页面跳转
_DataOperateIndex.gotoRecord = function( $scope, $location, SdbFunction, listIndex ){
   var fullName = $scope.clsInfo[listIndex]['Name'] ;
   var csName = fullName.split( '.' )[0] ;
   var clName = fullName.split( '.' )[1] ;
   var clType = '' ;
   if( $scope.clsInfo[listIndex]['IsMainCL'] == true )
   {
      clType = 'main' ;
   }
   SdbFunction.LocalData( 'SdbCsName', csName ) ;
   SdbFunction.LocalData( 'SdbClName', clName ) ;
   SdbFunction.LocalData( 'SdbClType', clType ) ;
   $location.path( 'Data/SDB-Operate/Record' ).search( { 'r': new Date().getTime() } ) ;
}

//lob页面跳转
_DataOperateIndex.gotoLob = function( $scope, $location, SdbFunction, listIndex ){
   var fullName = $scope.clsInfo[listIndex]['Name'] ;
   var csName = fullName.split( '.' )[0] ;
   var clName = fullName.split( '.' )[1] ;
   var clType = '' ;
   if( $scope.clsInfo[listIndex]['IsMainCL'] == true )
   {
      clType = 'main' ;
   }
   SdbFunction.LocalData( 'SdbCsName', csName ) ;
   SdbFunction.LocalData( 'SdbClName', clName ) ;
   SdbFunction.LocalData( 'SdbClType', clType ) ;
   $location.path( 'Data/SDB-Operate/Lobs' ).search( { 'r': new Date().getTime() } ) ;
}

//上一页
_DataOperateIndex.previous = function( $scope, $compile ){
   --$scope.current ;
   $scope.setCurrent = $scope.current ;
   _DataOperateIndex.showPage( $scope, $compile, $scope.current ) ;
}

//下一页
_DataOperateIndex.nextPage = function( $scope, $compile ){
   ++$scope.current ;
   $scope.setCurrent = $scope.current ;
   _DataOperateIndex.showPage( $scope, $compile, $scope.current ) ;
}

//跳转到指定页
_DataOperateIndex.gotoPate = function( $scope, $compile, event ){
   if( event.keyCode == 13 )
   {
      if( $scope.setCurrent.length == 0 )
      {
         $scope.setCurrent = 1 ;
      }
      $scope.current = $scope.setCurrent ;
      _DataOperateIndex.showPage( $scope, $compile, $scope.current ) ;
   }
}

//检查输入的页数格式
_DataOperateIndex.checkCurrent = function( $scope ){
   if( $scope.setCurrent.length == 0 )
   {
   }
   else if( isNaN( $scope.setCurrent ) || parseInt( $scope.setCurrent ) != $scope.setCurrent )
   {
      $scope.setCurrent = parseInt( $scope.setCurrent ) ;
      if( isNaN( $scope.setCurrent ) )
      {
         $scope.setCurrent = 1 ;
      }
   }
   else if( $scope.setCurrent > $scope.total )
   {
      $scope.setCurrent = $scope.total ;
   }
   else if( $scope.setCurrent <= 0 )
   {
      $scope.setCurrent = 1 ;
   }
}

//获取集合列表
_DataOperateIndex.getCLList = function( $scope, $compile, SdbRest, moduleName, moduleMode ){
   var sql ;
   if( moduleMode == 'standalone' )
   {
      sql = 'SELECT T1.Name, T1.IsMainCL, T1.MainCLName, T1.ShardingType, T1.Details.TotalRecords as TotalRecords, T1.Details.Indexes AS TotalIndexes FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS T1' ;
   }  
   else
   {
      /*sql = 'SELECT T4.Name, T4.IsMainCL, T4.MainCLName, T4.ShardingType, T3.TotalRecords, T3.TotalIndexes FROM (SELECT T2.Name, sum(T2.SUM1) AS TotalRecords, sum(T2.SUM2) AS TotalIndexes FROM (SELECT T1.Details.TotalRecords AS SUM1, T1.Details.Indexes AS SUM2, T1.Name FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS T1 GROUP BY T1.Details.GroupName) AS T2 GROUP BY T2.Name) AS T3 RIGHT OUTER JOIN $SNAPSHOT_CATA AS T4 ON T3.Name = T4.Name GROUP BY T4.Name' ;*/
      sql = 'SELECT t1.Name, t1.Details.TotalLob, t1.Details.TotalRecords FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1' ;
   }

   //合并cl数据
   function mergedData( clList, cataList ){
      var newClList = []
      $.each( clList, function( index, clInfo ){
         var hasFind = false ;
         $.each( cataList, function( index2, cataInfo ){
            if( clInfo['Name'] == cataInfo['Name'] && typeof( cataInfo['MainCLName'] ) == 'string' )
            {
               hasFind = true ;
               return false ;
            }
         } ) ;
         if( hasFind == false )
         {
            hasFind = false ;
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
         }
      } ) ;
      $.each( cataList, function( index, cataInfo ){
         if( cataInfo['IsMainCL'] == true )
         {
            cataInfo['TotalRecords'] = 0 ;
            $.each( cataInfo['CataInfo'], function( index2, rangeInfo ){
               $.each( clList, function( index3, clInfo ){
                  if( clInfo['Name'] == rangeInfo['SubCLName'] )
                  {
                     cataInfo['TotalRecords'] += clInfo['TotalRecords'] ;
                  }
               } ) ;
            } ) ;

            newClList.push( cataInfo ) ;
         }
         else if( typeof( cataInfo['ShardingType'] ) == 'string' )
         {
            $.each( newClList, function( index2, newInfo ){
               if( newInfo['Name'] == cataInfo['Name'] )
               {
                  newInfo['ShardingType'] = cataInfo['ShardingType'] ;
                  newInfo['ShardingKey'] = cataInfo['ShardingKey'] ;
                  return false;
               }
            } ) ;
         }
      } ) ;
      return newClList ;
   }

   function success( data )
   {
      if( data.length == 1 && data[0]['Name'] == null ) data = [] ;
      $scope.execRc = true ;
      $scope.clsInfo = data ;
      $scope.total = parseInt( $scope.clsInfo.length / $scope.limit ) ;
      if( $scope.clsInfo.length % $scope.limit > 0 )
      {
         ++$scope.total ;
      }
      $scope.current = 1 ;
      $scope.setCurrent = 1 ;
      _DataOperateIndex.showPage( $scope, $compile, 1 ) ;
   }

   //获取集合列表
   SdbRest.Exec( sql, {
      'success': function( clList ){
         if( moduleMode == 'standalone' )
         {
            success( clList ) ;
         }
         else
         {
            sql = 'select * from $SNAPSHOT_CATA where IsMainCL=true or MainCLName>"" or ShardingType>""' ;
            SdbRest.Exec( sql, {
               'success': function( cataList ){
                  var newList = mergedData( clList, cataList ) ;
                  success( newList ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
      },
      'failed': function( errorInfo ){
         _DataOperateIndex.showPage( $scope, $compile, 1 ) ;
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            _DataOperateIndex.getCLList( $scope, $compile, SdbRest, moduleName, moduleMode ) ;
            return true ;
         } ) ;
      }
   } ) ;
}