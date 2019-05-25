//@ sourceURL=other/Record.js
// --------------------- Data.Operate.Record ---------------------
var _DataOperateRecord = {} ;

//打开 索引详细 的窗口
_DataOperateRecord.getIndexInfo = function( $scope, SdbRest ){
   var data = { 'cmd': 'list indexes', 'collectionname': $scope.fullName } ;
   SdbRest.DataOperation( data, {
      'success': function( indexList ){
         $scope.indexList.push( { 'key': $scope.autoLanguage( '无' ), 'value': 0 } ) ;
         $scope.indexList.push( { 'key': $scope.autoLanguage( '表扫描' ), 'value': 1 } ) ;
         $.each( indexList, function( index, indexInfo ){
            $scope.indexList.push( { 'key': indexInfo['IndexDef']['name'], 'value': indexInfo['IndexDef']['name'] } ) ;
         } ) ;
      },
      'failed': function( errorInfo ){
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            exec() ;
            return true ;
         }, $scope.autoLanguage( '获取索引信息失败' ) ) ;
      }
   } ) ;
}

//初始化
_DataOperateRecord.init = function( $scope ){
   //是不是非条件查询
   $scope.isNotFilter = true ;
   //记录集
   $scope.records = [] ;
   //每页最大记录数
   $scope.limit = 30 ;
   //当前页的字段
   $scope.fieldList = [] ;
   //当前页
   $scope.setCurrent = 1 ;
   $scope.current = 1 ;
   //总页数
   $scope.total = 0 ;
   //显示记录总数内容
   $scope.recordTotal = '' ;
   //查询的条件
   $scope.queryFilter = { 'name': $scope.fullName, 'returnnum': $scope.limit, 'skip': 0 } ;
   $scope.GridData = { 'title': [], 'body': [], 'tool': {}, 'options': { 'grid': {} } } ;
   $scope.indexList = [] ;
}

//查询
_DataOperateRecord.queryRecord = function( $scope, SdbRest, SdbFunction, data, type, showSuccess ){
   if( typeof( data['filter'] ) != 'undefined' || typeof( data['selector'] ) != 'undefined' ||
       typeof( data['sort'] ) != 'undefined' || typeof( data['hint'] ) != 'undefined' )
   {
      $scope.isNotFilter = false ;
   }
   data['cmd'] = 'query' ;
   var errJson = [] ;
   SdbRest.DataOperation( data, {
      'success': function( json ){
         $scope.records = json ;
         $scope.ErrRecord = errJson ;
         //获取所有字段
         $.each( $scope.records, function( index, record ){
            $scope.fieldList = SdbFunction.getJsonKeys( record, 0, $scope.fieldList ) ;
         } ) ;
         if( showSuccess != false )
         {
            var start = 0 ;
            var end = 0 ;
            if( $scope.records.length > 0 )
            {
               start = data['skip'] + 1 ;
               end = data['skip'] + $scope.records.length ;
            }
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询成功，显示 ? - ?，总计 ? 条记录' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, start, end, $scope.records.length ) ;
            $scope.execRc = true ;
         }
         $scope.recordTotal = '' ;
         $scope.queryFilter = data ;
      },
      'failed': function( errorInfo ){
         $scope.records = [] ;
         $scope.fieldList = [] ;
         $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
         $scope.execRc = false ;
      },
      'error': function(){
         $scope.records = [] ;
         $scope.fieldList = [] ;
         //_IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
      },
      'complete': function(){
         $scope.$apply() ;
         $scope.show( type ) ;
      }
   }, {}, errJson ) ;
   if( $scope.isNotFilter )
   {
      var newdata = { 'cmd': 'get count', 'name': $scope.fullName } ;
      SdbRest.DataOperation( newdata, {
         'success': function( countData ){
            $scope.recordTotal = sprintf( $scope.autoLanguage( '一共 ? 条记录。' ), countData[0]['Total'] ) ;
            $scope.total = parseInt( countData[0]['Total'] / $scope.limit ) ;
            if( countData[0]['Total'] % $scope.limit > 1 )
            {
               ++$scope.total ;
            }
            if( $scope.total == 0 )
            {
               $scope.total = 1 ;
            }
            $scope.$apply() ;
         }
      } ) ;
   }
}

//构造json
_DataOperateRecord.buildJsonGrid = function( $scope, $compile )
{
   $scope.showType = 1 ;
   var gridData = {
      'title': [ { 'text': '#' }, { 'text': '' }, { 'text': 'Record' } ],
      'body': [],
      'tool': {
         'position': 'bottom',
         'left': [
            { 'html': $compile( '<i class="fa fa-font"  ng-class="{ active: showType == 1 }" ng-click="show(1)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-list" ng-class="{ active: showType == 2 }" ng-click="show(2)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-table"  ng-class="{ active: showType == 3 }" ng-click="show(3)"></i>' )( $scope ) }
         ],
         'right': [
            { 'html': $compile( '<span ng-bind="recordTotal"></span>' )( $scope ) }
         ]
      },
      'options': {
         'grid': { 'tdModel': 'auto', 'gridModel': 'fixed', 'titleWidth': [ '60px', '80px', 100 ] }
      }
   } ;
   if( $scope.isNotFilter )
   {
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play fa-flip-horizontal" ng-show="current > 1" ng-click="previous()"></i>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<input style="width:100px;" ng-change="checkCurrent()" ng-model="setCurrent" ng-keypress="gotoPate($event)">' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span>/<span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span ng-bind="total"></span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play" ng-show="current < total" ng-click="nextPage()"></i>' )( $scope ) } ) ;
   }
   $.each( $scope.records, function( index, record ){
      var tmpJson = JSON.stringify( record, null, 3 ) ;
      var editBtn = $compile( '<a ng-click="Edit(' + index + ')"></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-edit"></i>' ).attr( 'data-desc', $scope.autoLanguage( '编辑' ) ) ) ;
      var copyBtn = $compile( '<a ng-click="Insert(' + index + ')"></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-copy"></i>' ).attr( 'data-desc', $scope.autoLanguage( '复制' ) ) ) ;
      var deleteBtn = $compile( '<a ng-click="DeleteRecord(' + index + ')" ></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-remove"></i>' ).attr( 'data-desc', $scope.autoLanguage( '删除' ) ) ) ;
      if( $scope.ErrRecord[index] == false )
      {
         gridData['body'].push( [
            { 'text': index + 1 },
            { 'html': $compile( '<span></span>' )( $scope ).append( editBtn ).append( '&nbsp;&nbsp;' ).append( copyBtn ).append( '&nbsp;&nbsp;' ).append( deleteBtn ), 'ellipsis': false },
            { 'text': tmpJson, 'ellipsis': false }
         ] ) ;
      }
      else
      {
         gridData['body'].push( [
            { 'html': $compile( '<span>' + ( index + 1 ) + '&nbsp;<i class="fa fa-warning" style="color:#FF8804;" data-desc="' + $scope.autoLanguage( '解释失败的记录' ) + '"></i></span>' )( $scope ) },
            { 'html': $compile( '<span></span>' )( $scope ).append( editBtn ).append( '&nbsp;&nbsp;' ).append( copyBtn ).append( '&nbsp;&nbsp;' ).append( deleteBtn ), 'ellipsis': false },
            { 'text': tmpJson, 'ellipsis': false }
         ] ) ;
      }
   } ) ;
   $scope.GridData = gridData ;
}

//构造树
_DataOperateRecord.buildTreeGrid = function( $scope, $compile )
{
   $scope.showType = 2 ;
   var gridData = {
      'title': [
         { 'text': 'Key' }, { 'text': 'Value' }, { 'text': 'Type' }
      ],
      'body': [],
      'tool': {
         'position': 'bottom',
         'left': [
            { 'html': $compile( '<i class="fa fa-font" ng-class="{ active: showType == 1 }" ng-click="show(1)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-list" ng-class="{ active: showType == 2 }" ng-click="show(2)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-table" ng-class="{ active: showType == 3 }" ng-click="show(3)"></i>' )( $scope ) }
         ],
         'right': [
            { 'html': $compile( '<span ng-bind="recordTotal"></span>' )( $scope ) }
         ]
      },
      'options': {
         'grid': { 'tdModel': 'dynamic', 'gridModel': 'fixed' },
         'event': {
            'onResize': function( column, line, width, height ){
               if( column == 0 )
               {
                  var id = line ;
                  gridData['data'][id]['width'] = parseInt( width ) ;
               }
            }
         }
      },
      'data': []
   } ;
   if( $scope.isNotFilter )
   {
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play fa-flip-horizontal" ng-show="current > 1" ng-click="previous()"></i>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<input type="text" style="width:100px;" ng-change="checkCurrent()" ng-model="setCurrent" ng-keypress="gotoPate($event)">' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span>/<span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span ng-bind="total"></span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play" ng-show="current < total" ng-click="nextPage()"></i>' )( $scope ) } ) ;
   }
   $.each( $scope.records, function( index, record ){
      var index2 = gridData['data'].length ;
      var line = json2Array( record, 0, true ) ;
      gridData['data'].push( { 'Json': line, 'index': index + 1, width: 100 } ) ;
      gridData['body'].push( [
         { 'html': '<div tree-key para="data.data[' + index2 + ']"></div>' },
         { 'html': '<div tree-value para="data.data[' + index2 + ']"></div>' },
         { 'html': '<div tree-type para="data.data[' + index2 + ']"></div>' }
      ] ) ;
   } ) ;
   $scope.GridData = gridData ;

}

//构造表格
_DataOperateRecord.buildTableGrid = function( $scope, $compile, SdbFunction )
{
   $scope.showType = 3 ;
   var gridData = {
      'title': [],
      'body': [],
      'tool': {
         'position': 'bottom',
         'left': [
            { 'html': $compile( '<i class="fa fa-font"  ng-class="{ active: showType == 1 }" ng-click="show(1)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-list"  ng-class="{ active: showType == 2 }" ng-click="show(2)"></i>' )( $scope ) },
            { 'html': $compile( '<i class="fa fa-table" ng-class="{ active: showType == 3 }" ng-click="show(3)"></i>' )( $scope ) }
         ],
         'right': [
            { 'html': $compile( '<span ng-bind="recordTotal"></span>' )( $scope ) }
         ]
      },
      'options': {
         'grid': { 'tdModel': 'fixed', 'gridModel': 'fixed', 'tdHeight': '19px', 'titleWidth': [ '60px' ] }
      }
   } ;
   if( $scope.isNotFilter )
   {
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( {} ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play fa-flip-horizontal" ng-show="current > 1" ng-click="previous()"></i>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<input style="width:100px;" ng-change="checkCurrent()" ng-model="setCurrent" ng-keypress="gotoPate($event)">' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span>/<span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<span ng-bind="total"></span>' )( $scope ) } ) ;
      gridData.tool.left.push( { 'html': $compile( '<i class="fa fa-play" ng-show="current < total" ng-click="nextPage()"></i>' )( $scope ) } ) ;
   }
   var keyList = [] ;
   //取得json的所有键
   $.each( $scope.records, function( index, record ){
      keyList = SdbFunction.getJsonKeys( record, 0, keyList ) ;
   } ) ;
   keyList.unshift( '#' ) ;
   //计算列宽
   var titleWidth = 100 / ( keyList.length - 1 ) ;
   $.each( keyList, function( index, key ){
      //填写标题
      gridData['title'].push( { 'text': key } ) ;
      if( index > 0 )
      {
         //设置宽度
         gridData.options.grid.titleWidth.push( titleWidth ) ;
      }
   } ) ;
   //取得json的所有值
   $.each( $scope.records, function( index, record ){
      var line = [] ;
      line = SdbFunction.getJsonValues( record, keyList, line ) ;
      line[0] = index + 1 ;
      var newRow = [] ;
      $.each( line, function( index, value ){
         newRow.push( { 'text': value } ) ;
      } ) ;
      gridData['body'].push( newRow ) ;
   } ) ;
   $scope.GridData = gridData ;
}

//创建插入操作弹窗
_DataOperateRecord.createInsertModel = function( $scope, SdbRest, SdbFunction, recordIndex ){
   $scope.Components.Modal.icon = 'fa-plus' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '插入' ) ;
   $scope.Components.Modal.isShow = true ;
   if( typeof( recordIndex ) == 'undefined' )
   {
      $scope.Components.Modal.jsonEdit = { Json: {}, Height: 0 } ;
   }
   else
   {
      $scope.Components.Modal.icon = 'fa-copy' ;
      $scope.Components.Modal.title = $scope.autoLanguage( '复制' ) ;
      var newObject = jQuery.extend( true, {}, $scope.records[recordIndex] ) ;
      delete newObject['_id'] ;
      $scope.Components.Modal.jsonEdit = { Json: newObject, Height: 0 } ;
   }
   $scope.Components.Modal.Context = '<div json-edit para="data.jsonEdit"></div>' ;
   $scope.Components.Modal.ok = function(){
      var str = JSON.stringify( $scope.Components.Modal.jsonEdit.Callback.getJson() ) ;
      var data = { 'cmd': 'insert', 'name': $scope.fullName, 'insertor': str } ;
      SdbRest.DataOperation( data, {
         'success': function( json ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 插入记录成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
            $scope.execRc = true ;
            _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
         },
         'failed': function( errorInfo ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 插入记录失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
            $scope.execRc = false ;
         },
         'complete': function(){
            //关闭弹窗
            $scope.Components.Modal.isShow = false ;
            $scope.$apply() ;
         }
      } ) ;
      return false ;
   }
   $scope.Components.Modal.onResize = function( width, height ){
      $scope.Components.Modal.jsonEdit.Height = height ;
   }
}

//创建编辑操作弹窗
_DataOperateRecord.createEditModel = function( $scope, SdbRest, SdbFunction, recordIndex ){
   var newObject = jQuery.extend( true, {}, $scope.records[recordIndex] ) ;
   delete newObject['_id'] ;
   $scope.Components.Modal.icon = 'fa-edit' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '编辑记录' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.jsonEdit = { Json: newObject, Height: 0 } ;
   $scope.Components.Modal.Context = '<div class="alert alert-warning" style="padding:10px;"><span style="color:#FF8804;font-size:150%;"><i class="fa fa-exclamation-triangle"></i></span>&nbsp;' + $scope.autoLanguage( '更新操作中会忽略对分区键的修改。' ) + '</div><div style="margin-top:10px;" json-edit para="data.jsonEdit"></div>' ;
   $scope.Components.Modal.ok = function(){
      var filter = JSON.stringify( { '_id': $scope.records[recordIndex]['_id'] } ) ;
      var newRecord = $scope.Components.Modal.jsonEdit.Callback.getJson() ;
      var updator = JSON.stringify( { '$replace': newRecord } ) ;
      var data = { 'cmd': 'update', 'name': $scope.fullName, 'updator': updator, 'filter': filter } ;
      SdbRest.DataOperation( data, {
         'success': function( json ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
            $scope.execRc = true ;
            _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
         },
         'failed': function( errorInfo ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
            $scope.execRc = false ;
         }, 
         'complete': function(){
            //关闭弹窗
            $scope.Components.Modal.isShow = false ;
            $scope.$apply() ;
         }
      } ) ;
      return false ;
   }
   $scope.Components.Modal.onResize = function( width, height ){
      $scope.Components.Modal.jsonEdit.Height = height - 55 ;
      $scope.$apply() ;
   }
}

//创建查询操作弹窗
_DataOperateRecord.createQueryModel = function( $scope, SdbRest, SdbFunction ){
   $scope.Components.Modal.icon = 'fa-search' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '查询' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         _operate.condition( $scope, "查询条件" ),
         _operate.selector( $scope ),
         _operate.sort( $scope ),
         _operate.hint( $scope ),
         _operate.returnNum( $scope ),
         _operate.skip( $scope )
      ]
   } ;
   $scope.Components.Modal.form2 = {
      inputList: [
         _operate.returnNum( $scope ),
         _operate.skip( $scope )
      ]
   } ;
   $scope.Components.Modal.tab = 1 ;
   $scope.Components.Modal.filter = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.selector = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.sort = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.hint = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.Context = '\
<div class="underlineTab" style="padding-bottom:20px;">\
   <ul class="left">\
      <li ng-class="{active:data.tab == 1}">\
         <a ng-click="data.tab = 1">' + $scope.autoLanguage( '快速' ) + '</a>\
      </li>\
      <li ng-class="{active:data.tab == 2}">\
         <a ng-click="data.tab = 2">' + $scope.autoLanguage( '高级' ) + '</a>\
      </li>\
   </ul>\
</div>\
<div ng-show="data.tab == 1" form-create para="data.form"></div>\
<table ng-show="data.tab == 2" class="table loosen">\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '查询条件' ) + '</td>\
      <td><div json-edit para="data.filter"></div></td>\
   </tr>\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '选择字段' ) + '</td>\
      <td><div json-edit para="data.selector"></div></td>\
   </tr>\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '排序字段' ) + '</td>\
      <td><div json-edit para="data.sort"></div></td>\
   </tr>\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '扫描方式' ) + '</td>\
      <td><div json-edit para="data.hint"></div></td>\
   </tr>\
   <tr>\
      <td colspan="2"><div form-create para="data.form2"></div></td>\
   </tr>\
</table>' ;
   $scope.Components.Modal.ok = function(){
      if( $scope.Components.Modal.tab == 1 )
      {
         var isAllClear = $scope.Components.Modal.form.check() ;
         if( isAllClear )
         {
            function modalValue2Query( valueJson )
            {
               var filter = parseConditionValue( valueJson['filter'] ) ;
               var selector = parseSelectorValue( valueJson['selector'] ) ;
               var sort = parseSortValue( valueJson['sort'] ) ;
               var hint = parseHintValue( valueJson['hint'] ) ;
               var returnnum = valueJson['returnnum'] ;
               var skip = valueJson['skip'] ;
               //组装
               var returnJson = {} ;
               if( $.isEmptyObject( filter ) == false )
               {
                  returnJson['filter'] = JSON.stringify( filter ) ;
               }
               if( $.isEmptyObject( selector ) == false )
               {
                  returnJson['selector'] = JSON.stringify( selector ) ;
               }
               if( $.isEmptyObject( sort ) == false )
               {
                  returnJson['sort'] = JSON.stringify( sort ) ;
               }
               if( $.isEmptyObject( hint ) == false )
               {
                  returnJson['hint'] = JSON.stringify( hint ) ;
               }
               returnJson['returnnum'] = returnnum ;
               returnJson['skip'] = skip ;
               return returnJson ;
            }
            var value = $scope.Components.Modal.form.getValue() ;
            var data = modalValue2Query( value ) ;
            data['name'] = $scope.fullName ;
            _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, data, $scope.showType ) ;
         }
         return isAllClear ;
      }
      else
      {
         var isAllClear = $scope.Components.Modal.form2.check() ;
         if( isAllClear )
         {
            var filter = $scope.Components.Modal.filter.Callback.getJson() ;
            var selector = $scope.Components.Modal.selector.Callback.getJson() ;
            var sort = $scope.Components.Modal.sort.Callback.getJson() ;
            var hint = $scope.Components.Modal.hint.Callback.getJson() ;
            var data = $scope.Components.Modal.form2.getValue() ;
            if( $.isEmptyObject( filter ) == false )
            {
               data['filter'] = JSON.stringify( filter ) ;
            }
            if( $.isEmptyObject( selector ) == false )
            {
               data['selector'] = JSON.stringify( selector ) ;
            }
            if( $.isEmptyObject( sort ) == false )
            {
               data['sort'] = JSON.stringify( sort ) ;
            }
            if( $.isEmptyObject( hint ) == false )
            {
               data['hint'] = JSON.stringify( hint ) ;
            }
            data['name'] = $scope.fullName ;
            _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, data, $scope.showType ) ;
         }
         return isAllClear ;
      }
   }
   $scope.Components.Modal.onResize = function( width, height ){
      height = height - 30 ;
      height = parseInt( height / 4 ) ;
      if( height < 160 ) height = 160 ;
      $scope.Components.Modal.filter.Height = height ;
      $scope.Components.Modal.selector.Height = height ;
      $scope.Components.Modal.sort.Height = height ;
      $scope.Components.Modal.hint.Height = height ;
   }
}

//创建更新操作弹窗
_DataOperateRecord.createUpdateModel = function( $scope, SdbRest, SdbFunction ){
   $scope.Components.Modal.icon = 'fa-edit' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '更新' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         _operate.condition( $scope, "查询条件" ),
         _operate.updator( $scope )
      ]
   } ;
   $scope.Components.Modal.tab = 1 ;
   $scope.Components.Modal.rule = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.filter = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.Context = '\
<div class="alert alert-warning" style="padding:10px;">\
   <span style="color:#FF8804;font-size:150%;"><i class="fa fa-exclamation-triangle"></i></span>&nbsp;' + $scope.autoLanguage( '更新操作中会忽略对分区键的修改。' ) + '\
</div>\
<div class="underlineTab" style="padding-bottom:20px;margin-top:10px;">\
   <ul class="left">\
      <li ng-class="{active:data.tab == 1}">\
         <a ng-click="data.tab = 1">' + $scope.autoLanguage( '快速' ) + '</a>\
      </li>\
      <li ng-class="{active:data.tab == 2}">\
         <a ng-click="data.tab = 2">' + $scope.autoLanguage( '高级' ) + '</a>\
      </li>\
   </ul>\
</div>\
<div ng-show="data.tab == 1" form-create para="data.form"></div>\
<table ng-show="data.tab == 2" class="table loosen">\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '匹配条件' ) + '</td>\
      <td><div json-edit para="data.filter"></div></td>\
   </tr>\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '更新操作' ) + '</td>\
      <td><div json-edit para="data.rule"></div></td>\
   </tr>\
</table>' ;
   $scope.Components.Modal.ok = function(){
      if( $scope.Components.Modal.tab == 1 )
      {
         var isAllClear = $scope.Components.Modal.form.check() ;
         if( isAllClear )
         {
            function modalValue2Update( valueJson )
            {
               var updator = {} ;
               var filter = parseConditionValue( valueJson['filter'] ) ;
               var updator = parseUpdatorValue( valueJson['updator'] ) ;
               //组装
               var returnJson = {} ;
               if( $.isEmptyObject( filter ) == false )
               {
                  returnJson['filter'] = JSON.stringify( filter ) ;
               }
               if( $.isEmptyObject( updator ) == false )
               {
                  returnJson['updator'] = JSON.stringify( updator ) ;
               }
               return returnJson ;
            }
            var value = $scope.Components.Modal.form.getValue() ;
            var data = modalValue2Update( value ) ;
            data['cmd'] = 'update' ;
            data['name'] = $scope.fullName ;
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
                  $scope.execRc = true ;
                  _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
               },
               'failed': function( errorInfo ){
                  $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                  $scope.execRc = false ;
               },
               'complete': function(){
                  //关闭弹窗
                  $scope.Components.Modal.isShow = false ;
                  $scope.$apply() ;
               }
            } ) ;
         }
         return false ;
      }
      else
      {
         var data = {} ;
         var filter = $scope.Components.Modal.filter.Callback.getJson() ;
         var rule   = $scope.Components.Modal.rule.Callback.getJson() ;
         if( $.isEmptyObject( filter ) == false )
         {
            data['filter'] = JSON.stringify( filter ) ;
         }
         if( $.isEmptyObject( rule ) == false )
         {
            data['updator'] = JSON.stringify( rule ) ;
         }
         data['cmd'] = 'update' ;
         data['name'] = $scope.fullName ;
         SdbRest.DataOperation( data, {
            'success': function( json ){
               $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
               $scope.execRc = true ;
               _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
            },
            'failed': function( errorInfo ){
               $scope.execResult = sprintf( $scope.autoLanguage( '? ? 更新失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
               $scope.execRc = false ;
            },
            'complete': function(){
               //关闭弹窗
               $scope.Components.Modal.isShow = false ;
               $scope.$apply() ;
            }
         } ) ;
      }
   }
   $scope.Components.Modal.onResize = function( width, height ){
      height = height - 30 ;
      height = parseInt( height / 2 ) ;
      if( height < 160 ) height = 200 ;
      $scope.Components.Modal.filter.Height = height ;
      $scope.Components.Modal.rule.Height = height ;
   }
}

//创建删除操作弹窗
_DataOperateRecord.createDeleteModel = function( $scope, SdbRest, SdbFunction ){
   $scope.Components.Modal.icon = 'fa-remove' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '删除' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         _operate.condition( $scope, "删除条件" )
      ]
   } ;
   $scope.Components.Modal.tab = 1 ;
   $scope.Components.Modal.filter = { Json: {}, Height: 0 } ;
   $scope.Components.Modal.Context = '\
<div class="underlineTab" style="padding-bottom:20px;margin-top:10px;">\
   <ul class="left">\
      <li ng-class="{active:data.tab == 1}">\
         <a ng-click="data.tab = 1">' + $scope.autoLanguage( '快速' ) + '</a>\
      </li>\
      <li ng-class="{active:data.tab == 2}">\
         <a ng-click="data.tab = 2">' + $scope.autoLanguage( '高级' ) + '</a>\
      </li>\
   </ul>\
</div>\
<div ng-show="data.tab == 1" form-create para="data.form"></div>\
<table ng-show="data.tab == 2" class="table loosen">\
   <tr>\
      <td style="width:130px;vertical-align:top;">' + $scope.autoLanguage( '匹配条件' ) + '</td>\
      <td><div json-edit para="data.filter"></div></td>\
   </tr>\
</table>' ;
   $scope.Components.Modal.ok = function(){
      if( $scope.Components.Modal.tab == 1 )
      {
         var isAllClear = $scope.Components.Modal.form.check() ;
         if( isAllClear )
         {
            function modalValue2Delete( valueJson )
            {
               var filter = parseConditionValue( valueJson['filter'] ) ;
               //组装
               var returnJson = {} ;
               if( $.isEmptyObject( filter ) == false )
               {
                  returnJson['deletor'] = JSON.stringify( filter ) ;
               }
               return returnJson ;
            }
            var value = $scope.Components.Modal.form.getValue() ;
            var data = modalValue2Delete( value ) ;
            var exec = function(){
               data['cmd'] = 'delete' ;
               data['name'] = $scope.fullName ;
               SdbRest.DataOperation( data, {
                  'success': function( json ){
                     $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
                     $scope.execRc = true ;
                     _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
                  },
                  'failed': function( errorInfo ){
                     $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                     $scope.execRc = false ;
                  },
                  'complete': function(){
                     //关闭弹窗
                     $scope.Components.Modal.isShow = false ;
                     $scope.$apply() ;
                  }
               } ) ;
            }
            if( isEmpty( data ) )
            {
               _IndexPublic.createInfoModel( $scope, $scope.autoLanguage( "执行当前的操作会删除所有记录！要继续吗？" ), $scope.autoLanguage( '继续' ), function(){
                  exec() ;
               } ) ;
            }
            else
            {
               exec() ;
            }
         }
         return false ;
      }
      else
      {
         var data = {} ;
         var filter = $scope.Components.Modal.filter.Callback.getJson() ;
         if( $.isEmptyObject( filter ) == false )
         {
            data['deletor'] = JSON.stringify( filter ) ;
         }
         var exec = function(){
            data['cmd'] = 'delete' ;
            data['name'] = $scope.fullName ;
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
                  $scope.execRc = true ;
                  _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
               },
               'failed': function( errorInfo ){
                  $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                  $scope.execRc = false ;
               },
               'complete': function(){
                  //关闭弹窗
                  $scope.Components.Modal.isShow = false ;
                  $scope.$apply() ;
               }
            } ) ;
         }
         if( isEmpty( data ) )
         {
            _IndexPublic.createInfoModel( $scope, $scope.autoLanguage( "执行当前的操作会删除所有记录！要继续吗？" ), $scope.autoLanguage( '继续' ), function(){
               exec() ;
            } ) ;
         }
         else
         {
            exec() ;
         }
      }
   }
   $scope.Components.Modal.onResize = function( width, height ){
      height = height - 90 ;
      if( height < 160 ) height = 160 ;
      $scope.Components.Modal.filter.Height = height ;
   }
}

//创建删除记录操作弹窗
_DataOperateRecord.createDeleteRecordModel = function( $scope, SdbRest, SdbFunction, recordIndex ){
   var _id = $scope.records[recordIndex]['_id']['$oid'] ;
   _IndexPublic.createRetryModel( $scope, null, function(){
      var deletor = JSON.stringify( { '_id': { '$oid': _id } } ) ;
      var data = { 'cmd': 'delete', 'name': $scope.fullName, 'deletor': deletor } ;
      SdbRest.DataOperation( data, {
         'success': function( json ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除成功' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName ) ;
            $scope.execRc = true ;
            _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType, false ) ;
         },
         'failed': function( errorInfo ){
            $scope.execResult = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
            $scope.execRc = false ;
         },
         'complete': function(){
            //关闭弹窗
            $scope.Components.Modal.isShow = false ;
            $scope.$apply() ;
         }
      } ) ;
      return true ;
   }, $scope.autoLanguage( '要删除这条记录吗？' ), '_id : ' + _id, $scope.autoLanguage( '是的，删除' ) ) ;
}

//查询所有
_DataOperateRecord.queryAll = function( $scope, SdbRest, SdbFunction ){
   $scope.isNotFilter = true ;
   $scope.setCurrent = 1 ;
   $scope.current = 1 ;
   var skipNum = ( $scope.current - 1 ) * $scope.limit ;
   $scope.queryFilter = { 'name': $scope.fullName, 'returnnum': $scope.limit, 'skip': skipNum } ;
   _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType ) ;
}

//上一页
_DataOperateRecord.previous = function( $scope, SdbRest, SdbFunction ){
   --$scope.current ;
   $scope.setCurrent = $scope.current ;
   var skipNum = ( $scope.current - 1 ) * $scope.limit ;
   $scope.queryFilter['skip'] = skipNum ;
   _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType ) ;
}

//下一页
_DataOperateRecord.nextPage = function( $scope, SdbRest, SdbFunction ){
   ++$scope.current ;
   $scope.setCurrent = $scope.current ;
   var skipNum = ( $scope.current - 1 ) * $scope.limit ;
   $scope.queryFilter['skip'] = skipNum ;
   _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType ) ;
}

//跳转到指定页
_DataOperateRecord.gotoPate = function( $scope, SdbRest, SdbFunction, event ){
   if( event.keyCode == 13 )
   {
      if( $scope.setCurrent.length == 0 )
      {
         $scope.setCurrent = 1 ;
      }
      $scope.current = $scope.setCurrent ;
      var skipNum = ( $scope.current - 1 ) * $scope.limit ;
      $scope.queryFilter['skip'] = skipNum ;
      _DataOperateRecord.queryRecord( $scope, SdbRest, SdbFunction, $scope.queryFilter, $scope.showType ) ;
   }
}

//检查输入的页数格式
_DataOperateRecord.checkCurrent = function( $scope ){
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

//显示方式
_DataOperateRecord.show = function( $scope, $compile, SdbFunction, type ){
   if( type == 1 )
   {
      _DataOperateRecord.buildJsonGrid( $scope, $compile ) ;
   }
   else if( type == 2 )
   {
      _DataOperateRecord.buildTreeGrid( $scope, $compile ) ;
   }
   else
   {
      _DataOperateRecord.buildTableGrid( $scope, $compile, SdbFunction ) ;
   }
}