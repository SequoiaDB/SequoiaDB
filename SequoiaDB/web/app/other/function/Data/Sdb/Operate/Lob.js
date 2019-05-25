// --------------------- Data.Operate.Lob ---------------------
var _DataOperateLob = {} ;

//查询指定范围的Lob
_DataOperateLob.queryLobs = function( $scope, $compile, SdbFunction, lobs, start, end ){
   $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询成功，显示 ? - ?，总计 ? 条记录' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, start, end, lobs.length ) ;
   $scope.execRc = true ;
   var gridData = {
      'title': [],
      'body': [],
      'tool':{
         'position': 'bottom',
         'left':
            [],
         'right': [
            { 'html': $compile( '<span ng-bind="recordTotal"></span>' )( $scope ) }
         ]
      },
      'options': {
         'order': { 'active': true },
         'grid': { 'tool': true, 'gridModel': 'fixed', titleWidth: [ '60px', '250px', 40, 30, 30 ] } 
      }
   } ;
   if( $scope.isNotFilter )
   {
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play fa-flip-horizontal" ng-show="current > 1" ng-click="previous()"></i>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<input style="width:100px;" ng-change="checkCurrent()" ng-model="setCurrent" ng-keypress="gotoPate($event)">' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span>/<span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span ng-bind="total"></span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play" ng-show="current < total" ng-click="nextPage()"></i>' )( $scope ) } ) ;
   }
   var keyList = [ '', 'Oid', 'CreateTime' ] ;
   $.each( lobs, function( index, record ){
         keyList = SdbFunction.getJsonKeys( record, 5, keyList ) ;
   } ) ;
   $.each( keyList, function( index, key ){
      gridData['title'].push( { 'text': key } );
   } ) ;
   $.each( lobs, function( index, record ){
      var id = index + 1 ;
      var line = SdbFunction.getJsonValues( record, keyList, [] ) ;
      var newRow = [] ;
      newRow[0] = { 'text': id } ;
      //构造一个删除按钮
      // var removeIcon = $( '<i></i>' ).addClass( 'fa fa-remove' ).text( ' ' + $scope.autoLanguage( '删除' ) ) ;
      // var removeBtn = $compile( '<a ng-click="LobDelete(' + ( index + start - 1 ) + ')"></div>' )( $scope ).addClass( 'linkButton' ).append( removeIcon ) ;
      // newRow[1] = { 'html': removeBtn } ;
      newRow[1] = { 'html': $compile( '<a ng-click="showLobInfo(\'' + record['Oid']['$oid'] + '\')"></a>' )( $scope ).addClass( 'linkButton' ).text( line[1] ) } ;
      newRow[2] = { 'text': line[2] } ;
      newRow[3] = { 'text': line[3] } ;
      newRow[4] = { 'text': line[4] } ;
      gridData['body'].push( newRow ) ;
   } ) ;
   $scope.lobGridData = gridData ;
}

//显示指定页的Lob
_DataOperateLob.showPage = function( $scope, $compile, SdbFunction, pageNum ){
   var newLobs = [] ;
   var startOfNum = $scope.limit * ( pageNum - 1 ) ;
   var endOfNum = $scope.limit * pageNum ;
   var start = 0 ;
   var end = 0 ;
   endOfNum = ( endOfNum > $scope.lobContent.length ? $scope.lobContent.length : endOfNum ) ;
   for( var i = startOfNum; i < endOfNum ; ++i )
   {
      newLobs.push( $scope.lobContent[i] ) ;
   }
   if( newLobs.length > 0 )
   {
      start = startOfNum + 1 ;
      end = endOfNum ;
   }
   _DataOperateLob.queryLobs( $scope, $compile, SdbFunction, newLobs, start, end ) ;
}

//查询所有Lob
_DataOperateLob.queryAll = function( $scope, $compile, SdbFunction, SdbRest ){
   var data = { 'cmd': 'list lobs', 'name': $scope.fullName } ;
   SdbRest.DataOperation( data, function( records ){
      $scope.lobContent = records ;
      $scope.recordTotal = sprintf( $scope.autoLanguage( '一共 ? 条记录。' ), $scope.lobContent.length ) ;
      $scope.total = parseInt( $scope.lobContent.length / $scope.limit ) ;
      if( $scope.lobContent.length % $scope.limit > 0 )
      {
         ++$scope.total ;
      }
      $scope.setCurrent = 1 ;
      $scope.current = 1 ;
      $scope.isNotFilter = true ;
      _DataOperateLob.showPage( $scope, $compile, SdbFunction, 1 ) ;
   }, function( errorInfo ){
      $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
      $scope.execRc = false ;
   }, function(){
      //_IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
   } ) ;
}

//上一页
_DataOperateLob.previous = function( $scope, $compile, SdbFunction ){
   --$scope.current ;
   $scope.setCurrent = $scope.current ;
   _DataOperateLob.showPage( $scope, $compile, SdbFunction, $scope.current ) ;
}

//下一页
_DataOperateLob.nextPage = function( $scope, $compile, SdbFunction ){
   ++$scope.current ;
   $scope.setCurrent = $scope.current ;
   _DataOperateLob.showPage( $scope, $compile, SdbFunction, $scope.current ) ;
}

//检查输入的页数格式
_DataOperateLob.checkCurrent = function( $scope ){
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

//跳转到指定页
_DataOperateLob.gotoPate = function( $scope, $compile, SdbFunction, event ){
   if( event.keyCode == 13 )
   {
      if( $scope.setCurrent.length == 0 )
      {
         $scope.setCurrent = 1 ;
      }
      $scope.current = $scope.setCurrent ;
      _DataOperateLob.showPage( $scope, $compile, SdbFunction, $scope.current ) ;
   }
}

//删除Lob记录
_DataOperateLob.LobDelete = function( $scope, SdbRest, index ){
   var oid = $scope.lobContent[index]['Oid']['$oid'] ;

   _IndexPublic.createRetryModel( $scope, null, function(){
      var data = { 'cmd': 'delete lob', 'name': $scope.fullName, 'oid': oid } ;
      SdbRest.DataOperation( data, function( json ){
         $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
         $scope.execRc = true ;
         $scope.queryAll() ;
         _DataOperateLob.showPage( $scope, $compile, SdbFunction, $scope.current ) ;
      }, function( errorInfo ){
         $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
         $scope.execRc = false ;
      }, function(){
         //_IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
      }, function(){
         //关闭弹窗
         $scope.Components.Modal.isShow = false ;
         $scope.$apply() ;
      } ) ;
      return true ;
   }, $scope.autoLanguage( '要删除这条记录吗？' ), 'Oid : ' + oid, $scope.autoLanguage( '是的，删除' ) ) ;
}

//查询lob
_DataOperateLob.LobQuery = function( $scope, $compile, SdbFunction ){
   $scope.Components.Modal.icon = 'fa-search' ;
   $scope.Components.Modal.title = $scope.autoLanguage( 'Lob查询' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "oid",
            "webName": "Lob Oid",
            "required": true,
            "type": "string",
            "value": "",
            "valid": {
               "min": 24,
               "max": 24
            }
         }
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function( data ){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var newLobs = [] ;
         var value = $scope.Components.Modal.form.getValue() ;
         $.each( $scope.lobContent, function( index, lobInfo ){
            if( lobInfo['Oid']['$oid'] == value['oid'] )
            {
               newLobs = [ lobInfo ] ;
               return false ;
            }
         } ) ;
         if( newLobs.length > 0 )
         {
            $scope.isNotFilter = false ;
            _DataOperateLob.queryLobs( $scope, $compile, SdbFunction, newLobs, 1, 1 ) ;
         }
         else
         {
            _DataOperateLob.queryLobs( $scope, $compile, SdbFunction, newLobs, 0, 0 ) ;
         }
         $scope.recordTotal = '';

      }
      return isAllClear ;
   } 
}

//显示lob的详细信息
_DataOperateLob.showLobInfo = function( $scope, oid ){
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.icon = '' ;
   $scope.Components.Modal.title = $scope.autoLanguage( 'Lob信息' ) ;
   $scope.Components.Modal.Grid = {} ;
   $.each( $scope.lobContent, function( index, lob ){
      if( lob['Oid']['$oid'] == oid )
      {
         $scope.Components.Modal.Grid = lob ;
         return false ;
      }
   } ) ;
   $scope.Components.Modal.Context = '\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr>\
<td>Oid</td>\
<td>{{data.Grid[\'Oid\'][\'$oid\']}}</td>\
</tr>\
<tr ng-repeat="(key, value) in data.Grid track by $index" ng-if="key != \'Oid\'">\
<td>{{key}}</td>\
<td ng-if="key != \'CreateTime\'">{{value}}</td>\
<td ng-if="key == \'CreateTime\'">{{value[\'$timestamp\']}}</td>\
</tr>\
</table>' ;
   $scope.Components.Modal.noOK = true ;
}