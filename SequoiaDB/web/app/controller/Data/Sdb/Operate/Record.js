//@ sourceURL=Record.js
"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.Operate.Record.Ctrl', function( $scope, $compile, $location, Loading, SdbRest, SdbFunction, SdbSwap, SdbSignal ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName + ', Mode: ' + moduleMode ) ;

      var csName = SdbFunction.LocalData( 'SdbCsName' ) ;
      var clName = SdbFunction.LocalData( 'SdbClName' ) ;
      var clType = SdbFunction.LocalData( 'SdbClType' ) ;
      if( csName == null || clName == null || clType == null )
      {
         $location.path( 'Data/SDB-Operate/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      SdbSwap.fullName = csName + '.' + clName ;
      SdbSwap.clType   = clType ;

      //初始化
      //索引列表
      var indexList = [] ;
      //表格模式: 1字符串  2树状  3表格
      $scope.TableType = 1 ;
      //每页最大记录数
      SdbSwap.limit = 30 ;
      //字符串记录
      SdbSwap.records = [] ;
      //json对象记录
      SdbSwap.records2 = [] ;
      SdbSwap.isNotFilter = true ;
      SdbSwap.queryFilter = { 'name': SdbSwap.fullName, 'returnnum': SdbSwap.limit, 'skip': 0 } ;
      //查询窗口属性
      $scope.QueryWindows = {
         'config': [],
         'tabType': 1,
         'data': {
            'filter': { Json: {}, Height: 0 },
            'selector': { Json: {}, Height: 0 },
            'sort': { Json: {}, Height: 0 },
            'hint': { Json: {}, Height: 0 }
         },
         'callback': {}
      } ;
      //编辑窗口属性
      $scope.EditWindows = {
         'data': { Json: {}, Height: 0 },
         'config': {},
         'callback': {}
      } ;
      //插入窗口属性
      $scope.InsertWindows = {
         'data': { Json: {}, Height: 0 },
         'config': {},
         'callback': {}
      } ;
      //查看JSON窗口属性
      $scope.ViewWindows = {
         'data': {},
         'dataCallback': {},
         'config': {},
         'callback': {}
      } ;
      //更新窗口属性
      $scope.UpdateWindows = {
         'data': {
            'rule': { Json: {}, Height: 0 },
            'filter': { Json: {}, Height: 0 }
         },
         'tabType': 1,
         'config': {},
         'callback': {}
      } ;
      //删除窗口属性
      $scope.DeleteWindows = {
         'data': {
            'filter': { Json: {}, Height: 0 }
         },
         'tabType': 1,
         'config': {},
         'callback': {}
      } ;

      //查询
      SdbSwap.queryRecord = function( data, showSuccess ){
         if( typeof( data['filter'] ) != 'undefined' || typeof( data['selector'] ) != 'undefined' ||
             typeof( data['sort'] ) != 'undefined' || typeof( data['hint'] ) != 'undefined' )
         {
            SdbSwap.isNotFilter = false ;
         }

         data['cmd'] = 'query' ;
         var strJson = [] ;
         var errJson = [] ;
         var options = { 'parseJson': false } ;
         SdbRest.DataOperation( data, {
            'success': function( json ){
               SdbSwap.records = json ;
               $scope.ErrRecord = options['errJson'] ;

               if( showSuccess != false )
               {
                  var start = 0 ;
                  var end   = 0 ;

                  if( SdbSwap.records.length > 0 )
                  {
                     start = data['skip'] + 1 ;
                     end   = data['skip'] + SdbSwap.records.length ;
                  }

                  var str = sprintf( $scope.autoLanguage( '? ? 执行查询成功，显示 ? - ?，总计 ? 条记录' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName,
                                     start, end,
                                     SdbSwap.records.length ) ;

                  SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
               }

               SdbSignal.commit( 'update_records', SdbSwap.records ) ;
               
               SdbSwap.queryFilter = data ;
               if( SdbSwap.isNotFilter == false )
               {
                  SdbSignal.commit( 'update_total', json.length ) ;
               }
            },
            'failed': function( errorInfo ){
               SdbSwap.records = [] ;
               var str = sprintf( $scope.autoLanguage( '? ? 执行查询失败，错误码: ?，?. ?' ),
                                  timeFormat( new Date(), 'hh:mm:ss' ),
                                  SdbSwap.fullName,
                                  errorInfo['errno'],
                                  errorInfo['description'],
                                  errorInfo['detail'] ) ;
               SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
            },
            'error': function(){
               SdbSwap.records = [] ;
            },
            'complete': function(){
            }
         }, options ) ;

         if( SdbSwap.isNotFilter )
         {
            var newdata = { 'cmd': 'get count', 'name': SdbSwap.fullName } ;
            SdbRest.DataOperation( newdata, {
               'success': function( countData ){
                  SdbSignal.commit( 'update_total', countData[0]['Total'] ) ;
               }
            } ) ;
         }
      }

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
         else
         {
            returnJson['filter'] = {} ;
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

      //查询所有
      $scope.queryAll = function(){
         SdbSwap.isNotFilter = true ;
         SdbSwap.queryFilter = { 'name': SdbSwap.fullName, 'returnnum': 30, 'skip': 0 } ;
         SdbSignal.commit( 'goto_page', 1 ) ;
      }

      //显示查询窗口
      $scope.ShowQueryWindows = function(){
         $scope.QueryWindows.tabType = 1 ;
         $scope.QueryWindows['config'][0] = {
            inputList: [
               _operate.condition( $scope, '查询条件' ),
               _operate.selector( $scope ),
               _operate.sort( $scope ),
               _operate.hint( $scope, indexList ),
               _operate.returnNum( $scope ),
               _operate.skip( $scope )
            ]
         } ;
         $scope.QueryWindows['config'][1] = {
            inputList: [
               _operate.returnNum( $scope ),
               _operate.skip( $scope )
            ]
         } ;
         $scope.QueryWindows['callback']['SetTitle']( $scope.autoLanguage( '查询' ) ) ;
         $scope.QueryWindows['callback']['SetIcon']( 'fa-search' ) ;
         $scope.QueryWindows['callback']['Open']() ;

         $scope.QueryWindows['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            if( $scope.QueryWindows.tabType == 1 )
            {
               var isAllClear = $scope.QueryWindows['config'][0].check() ;
               if( isAllClear )
               {
                  var value = $scope.QueryWindows['config'][0].getValue() ;
                  var data = modalValue2Query( value ) ;

                  SdbSwap.limit = value['returnnum'] ;
                  data['name'] = SdbSwap.fullName ;

                  SdbSwap.queryFilter = data ;
                  SdbSignal.commit( 'goto_page', 1 ) ;
               }
               return isAllClear ;
            }
            else
            {
               var isAllClear = $scope.QueryWindows['config'][1].check() ;
               if( isAllClear )
               {
                  var filter = $scope.QueryWindows['data']['filter'].Callback.getJson() ;
                  var selector = $scope.QueryWindows['data']['selector'].Callback.getJson() ;
                  var sort = $scope.QueryWindows['data']['sort'].Callback.getJson() ;
                  var hint = $scope.QueryWindows['data']['hint'].Callback.getJson() ;
                  var data = $scope.QueryWindows['config'][1].getValue() ;

                  if( $.isEmptyObject( filter ) == false )
                  {
                     data['filter'] = JSON.stringify( filter ) ;
                  }
                  else
                  {
                     data['filter'] = '{}' ;
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

                  data['name'] = SdbSwap.fullName ;
                  SdbSwap.limit = data['returnnum'] ;
                  SdbSwap.queryFilter = data ;
                  SdbSignal.commit( 'goto_page', 1 ) ;
               }
               return isAllClear ;
            }
         } ) ;

         $scope.QueryWindows['callback']['SetResize']( function( left, top, width, height ){
            height = height - 88 ;
            height = parseInt( height / 4 ) ;
            if( height < 160 ) height = 160 ;
            $scope.QueryWindows.data.filter.Height = height ;
            $scope.QueryWindows.data.selector.Height = height ;
            $scope.QueryWindows.data.sort.Height = height ;
            $scope.QueryWindows.data.hint.Height = height ;
         } ) ;
      }

      //显示编辑窗口
      $scope.ShowEdit = function( recordIndex ){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         if( SdbSwap.records2[recordIndex] == null )
         {
            SdbSwap.records2[recordIndex] = JSON.parse( SdbSwap.records[recordIndex] ) ;
         }

         var newObject = jQuery.extend( true, {}, SdbSwap.records2[recordIndex] ) ;
         var _id = newObject['_id'] ;
         delete newObject['_id'] ;
         $scope.EditWindows['data']['Json'] = newObject ;

         $scope.EditWindows['callback']['SetTitle']( $scope.autoLanguage( '编辑记录' ) ) ;
         $scope.EditWindows['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.EditWindows['callback']['Open']() ;

         $scope.EditWindows['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var filter = JSON.stringify( { '_id': _id } ) ;
            var newRecord = $scope.EditWindows.data['Callback'].getJson() ;
            var updator = JSON.stringify( { '$replace': newRecord } ) ;
            var data = { 'cmd': 'update', 'name': SdbSwap.fullName, 'updator': updator, 'filter': filter } ;

            SdbRest.DataOperation( data, {
               'success': function( json ){
                  var str = sprintf( $scope.autoLanguage( '? ? 更新成功' ), timeFormat( new Date(), 'hh:mm:ss' ), SdbSwap.fullName ) ;
                  SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                  SdbSwap.queryRecord( SdbSwap.queryFilter, false ) ;
               },
               'failed': function( errorInfo ){
                  var str = sprintf( $scope.autoLanguage( '? ? 更新失败，错误码: ?，?. ?' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName,
                                     errorInfo['errno'],
                                     errorInfo['description'],
                                     errorInfo['detail'] ) ;
                  SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
               },
               'complete': function(){
                  $scope.EditWindows['callback']['Close']() ;
               }
            } ) ;
            return false ;
         } ) ;

         $scope.EditWindows['callback']['SetResize']( function( left, top, width, height ){
            $scope.EditWindows.data.Height = height - 181 ;
         } ) ;
      }

      //显示插入窗口
      $scope.ShowInsert = function( recordIndex ){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         if( isNaN( recordIndex ) )
         {
            $scope.InsertWindows['callback']['SetTitle']( $scope.autoLanguage( '插入' ) ) ;
            $scope.InsertWindows['callback']['SetIcon']( 'fa-plus' ) ;
            $scope.InsertWindows['data']['Json'] = {} ;
         }
         else
         {
            $scope.InsertWindows['callback']['SetTitle']( $scope.autoLanguage( '复制' ) ) ;
            $scope.InsertWindows['callback']['SetIcon']( 'fa-copy' ) ;

            if( SdbSwap.records2[recordIndex] == null )
            {
               SdbSwap.records2[recordIndex] = JSON.parse( SdbSwap.records[recordIndex] ) ;
            }

            var newObject = jQuery.extend( true, {}, SdbSwap.records2[recordIndex] ) ;
            delete newObject['_id'] ;
            $scope.InsertWindows['data']['Json'] = newObject ;
         }

         $scope.InsertWindows['callback']['Open']() ;

         $scope.InsertWindows['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var str = JSON.stringify( $scope.InsertWindows['data'].Callback.getJson() ) ;
            var data = { 'cmd': 'insert', 'name': SdbSwap.fullName, 'insertor': str } ;
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  var str = sprintf( $scope.autoLanguage( '? ? 插入记录成功' ), timeFormat( new Date(), 'hh:mm:ss' ), SdbSwap.fullName ) ;
                  SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                  SdbSwap.queryRecord( SdbSwap.queryFilter, false ) ;
               },
               'failed': function( errorInfo ){
                  var str = sprintf( $scope.autoLanguage( '? ? 插入记录失败，错误码: ?，?. ?' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName,
                                     errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                  SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
               },
               'complete': function(){
                  $scope.InsertWindows['callback']['Close']() ;
               }
            } ) ;
            return false ;
         } ) ;

         $scope.InsertWindows['callback']['SetResize']( function( left, top, width, height ){
            $scope.InsertWindows.data.Height = height - 138 ;
         } ) ;
      }

      //显示更新窗口
      $scope.ShowUpdate = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         $scope.UpdateWindows['tabType'] = 1 ;

         $scope.UpdateWindows['config'] = {
            inputList: [
               _operate.condition( $scope, "查询条件" ),
               _operate.updator( $scope )
            ]
         } ;
         $scope.UpdateWindows['callback']['SetTitle']( $scope.autoLanguage( '更新' ) ) ;
         $scope.UpdateWindows['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.UpdateWindows['callback']['Open']() ;

         $scope.UpdateWindows['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var data ;

            if( $scope.UpdateWindows['tabType'] == 1 )
            {
               var isAllClear = $scope.UpdateWindows['config'].check() ;
               if( !isAllClear )
               {
                  return false ;
               }
               var value = $scope.UpdateWindows['config'].getValue() ;
               data = modalValue2Update( value ) ;
               data['cmd'] = 'update' ;
               data['name'] = SdbSwap.fullName ;
            }
            else
            {
               data = {} ;
               var filter = $scope.UpdateWindows['data'].filter.Callback.getJson() ;
               var rule   = $scope.UpdateWindows['data'].rule.Callback.getJson() ;
               if( $.isEmptyObject( filter ) == false )
               {
                  data['filter'] = JSON.stringify( filter ) ;
               }
               if( $.isEmptyObject( rule ) == false )
               {
                  data['updator'] = JSON.stringify( rule ) ;
               }
               data['cmd'] = 'update' ;
               data['name'] = SdbSwap.fullName ;
            }
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  var str = sprintf( $scope.autoLanguage( '? ? 更新成功' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName ) ;
                  SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                  SdbSignal.commit( 'goto_page', 1 ) ;
               },
               'failed': function( errorInfo ){
                  var str = sprintf( $scope.autoLanguage( '? ? 更新失败，错误码: ?，?. ?' ),
                                       timeFormat( new Date(), 'hh:mm:ss' ),
                                       SdbSwap.fullName,
                                       errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                  SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
               },
               'complete': function(){
                  $scope.UpdateWindows['callback']['Close']() ;
               }
            } ) ;
         } ) ;

         $scope.UpdateWindows['callback']['SetResize']( function( left, top, width, height ){
            height = height - 93 ;
            height = parseInt( height / 2 ) ;
            if( height < 160 ) height = 200 ;
            $scope.UpdateWindows['data']['filter'].Height = height ;
            $scope.UpdateWindows['data']['rule'].Height = height ;
         } ) ;
      }

      //创建删除弹窗
      $scope.ShowDelete = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         $scope.DeleteWindows['tabType'] = 1 ;

         $scope.DeleteWindows['config'] = {
            inputList: [
               _operate.condition( $scope, "删除条件" )
            ]
         } ;
         $scope.DeleteWindows['callback']['SetTitle']( $scope.autoLanguage( '删除' ) ) ;
         $scope.DeleteWindows['callback']['SetIcon']( 'fa-trash' ) ;
         $scope.DeleteWindows['callback']['Open']() ;

         $scope.DeleteWindows['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var data ;
            if( $scope.DeleteWindows['tabType'] == 1 )
            {
               var isAllClear = $scope.DeleteWindows['config'].check() ;
               if( !isAllClear )
               {
                  return false ;
               }
               var value = $scope.DeleteWindows['config'].getValue() ;
               data = modalValue2Delete( value ) ;
            }
            else
            {
               data = {} ;
               var filter = $scope.DeleteWindows['data'].filter.Callback.getJson() ;
               if( $.isEmptyObject( filter ) == false )
               {
                  data['deletor'] = JSON.stringify( filter ) ;
               }
            }
            var exec = function() {
               data['cmd'] = 'delete' ;
               data['name'] = SdbSwap.fullName ;
               SdbRest.DataOperation( data, {
                  'success': function( json ){
                     var str = sprintf( $scope.autoLanguage( '? ? 删除成功' ),
                                          timeFormat( new Date(), 'hh:mm:ss' ),
                                          SdbSwap.fullName ) ;
                     SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                     SdbSignal.commit( 'goto_page', 1 ) ;
                  },
                  'failed': function( errorInfo ){
                     var str = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ),
                                          timeFormat( new Date(), 'hh:mm:ss' ),
                                          SdbSwap.fullName,
                                          errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
                     SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
                  },
                  'complete': function(){
                     $scope.DeleteWindows['callback']['Close']() ;
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
         } ) ;

         $scope.DeleteWindows['callback']['SetResize']( function( left, top, width, height ){
            height = height - 216 ;
            if( height < 160 ) height = 160 ;
            $scope.DeleteWindows['data']['filter'].Height = height ;
         } ) ;
      }

      //创建删除记录提示
      $scope.DeleteRecord = function( recordIndex ){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         if( SdbSwap.records2[recordIndex] == null )
         {
            SdbSwap.records2[recordIndex] = JSON.parse( SdbSwap.records[recordIndex] ) ;
         }

         var _id = SdbSwap.records2[recordIndex]['_id']['$oid'] ;

         _IndexPublic.createRetryModel( $scope, null, function(){
            var deletor = JSON.stringify( { '_id': { '$oid': _id } } ) ;
            var data = { 'cmd': 'delete', 'name': SdbSwap.fullName, 'deletor': deletor } ;
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  var str = sprintf( $scope.autoLanguage( '? ? 删除成功' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName ) ;
                  SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                  SdbSwap.queryRecord( SdbSwap.queryFilter, $scope.TableType, false ) ;
               },
               'failed': function( errorInfo ){
                  var str = sprintf( $scope.autoLanguage( '? ? 删除失败，错误码: ?，?. ?' ),
                                     timeFormat( new Date(), 'hh:mm:ss' ),
                                     SdbSwap.fullName,
                                     errorInfo['errno'],
                                     errorInfo['description'],
                                     errorInfo['detail'] ) ;
                  SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
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

      SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;

      SdbSignal.on( 'update_table_type', function( tableType ){
         $scope.TableType = tableType ;
      } ) ;

      //获取索引列表
      var getIndexInfo = function(){
         var data = { 'cmd': 'list indexes', 'collectionname': SdbSwap.fullName } ;
         SdbRest.DataOperation( data, {
            'success': function( indexs ){
               indexList.push( { 'key': $scope.autoLanguage( '无' ), 'value': 0 } ) ;
               indexList.push( { 'key': $scope.autoLanguage( '表扫描' ), 'value': 1 } ) ;
               $.each( indexs, function( index, indexInfo ){
                  indexList.push( { 'key': indexInfo['IndexDef']['name'], 'value': indexInfo['IndexDef']['name'] } ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getIndexInfo() ;
                  return true ;
               }, $scope.autoLanguage( '获取索引信息失败' ) ) ;
            } 
         } ) ;
      }

      //显示方式
      $scope.show = function( type, isRender ){
         if( type == 1 )
         {
            //构造json
            $scope.showType = 1 ;
            if( isRender == true || $scope.GridData1['title'].length == 0 )
            {
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
               $scope.GridData1 = gridData ;
            }
            else
            {
               setTimeout( function(){
                  $scope.GridData1.onResize() ;
                  $scope.$apply() ;
               } ) ;
            }
         }
         else if( type == 2 )
         {
            $scope.showType = 2 ;
            if( isRender == true || $scope.GridData2['title'].length == 0 )
            {
               $scope.jsonTree = [] ;
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
                             $scope.jsonTree[id]['width'] = parseInt( width ) ;
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
                  var index2 =$scope.jsonTree.length ;
                  var line = json2Array( record, 0, true ) ;
                  $scope.jsonTree.push( { 'Json': line, 'index': index + 1, width: 100 } ) ;
                  gridData['body'].push( [
                     { 'html':  $compile( '<div tree-key para="jsonTree[' + index2 + ']"></div>' )( $scope ) },
                     { 'html':  $compile( '<div tree-value para="jsonTree[' + index2 + ']"></div>' )( $scope ) },
                     { 'html':  $compile( '<div tree-type para="jsonTree[' + index2 + ']"></div>' )( $scope ) }
                  ] ) ;
               } ) ;
               $scope.GridData2 = gridData ;
            }
            else
            {
               setTimeout( function(){
                  $scope.GridData2.onResize() ;
                  $scope.$apply() ;
               } ) ;
            }
         }
         else
         {
            $scope.showType = 3 ;
            if( isRender == true || $scope.GridData3['title'].length == 0 )
            {
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
               $.each( $scope.ShowKey2, function( index, showKeyInfo ){
                  if( showKeyInfo['show'] == true )
                  {
                     keyList.push( showKeyInfo['key'] ) ;
                  }
               } ) ;
               $scope.ShowKeyList = keyList ;
               keyList.unshift( '#' ) ;
               //计算列宽
               var titleWidth = 100 / ( ( keyList.length > 20 ? 20 : keyList.length ) - 1 ) ;
               $.each( keyList, function( index, key ){
                  if( index >= 20 )
                  {
                     return false ;
                  }
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
                     if( index >= 20 )
                     {
                        return false ;
                     }
                     newRow.push( { 'text': value } ) ;
                  } ) ;
                  gridData['body'].push( newRow ) ;
               } ) ;
               $scope.GridData3 = gridData ;
            }
            else
            {
               setTimeout( function(){
                  $scope.GridData3.onResize() ;
                  $scope.$apply() ;
               } ) ;
            }
         }
      }

      getIndexInfo() ;
   } ) ;

   //分页栏
   sacApp.controllerProvider.register( 'Data.Operate.Record.Tab.Ctrl', function( $scope, SdbSwap ){
   
      $scope.FullName = SdbSwap.fullName ;
      $scope.CLType   = SdbSwap.clType ;

   } ) ;

   //操作栏
   sacApp.controllerProvider.register( 'Data.Operate.Record.OperateBtn.Ctrl', function( $scope, SdbSwap, SdbSignal, SdbFunction ){

      var isFirst = true ;
      var showFieldList = [] ;

      //表格 显示列 下拉菜单
      $scope.FieldDropdown = {
         'config': [],
         'callback': {}
      } ;

      SdbSignal.on( 'update_fields', function( records ){

         var fieldList = [] ;

         $.each( SdbSwap.records2, function( index, record ){
            fieldList = SdbFunction.getJsonKeys( record, 0, fieldList ) ;
         } ) ;

         $scope.FieldDropdown['config'] = [] ;

         $.each( fieldList, function( index, field ){
            var isShow ;

            if( isFirst )
            {
               isShow = index < 10 ? true : false ;
               if( isShow )
               {
                  showFieldList.push( field ) ;
               }
            }
            else
            {
               isShow = showFieldList.indexOf( field ) >= 0 ? true : false ;
            }

            $scope.FieldDropdown['config'].push( { 'key': field, 'field': field, 'show': isShow } ) ;
         } ) ;

         SdbSignal.commit( 'update_table3_key', showFieldList ) ;

         isFirst = false ;

      } ) ;

      //打开 显示列 下拉菜单
      $scope.ShowKeyList = function( event ){
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 显示列
      $scope.SaveField = function(){
         showFieldList = [] ;
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            if( fieldInfo['show'] )
            {
               showFieldList.push( fieldInfo['field'] ) ;
            }
         } ) ;
         SdbSignal.commit( 'update_table3_key', showFieldList ) ;
      }
   } ) ;

   //结果栏
   sacApp.controllerProvider.register( 'Data.Operate.Record.Result.Ctrl', function( $scope, SdbSwap, SdbSignal ){

      SdbSignal.on( 'update_result', function( result ){
         $scope.ExecRc = result['rc'] ;
         $scope.ExecResult = result['result'] ;
      }, false ) ;

   } ) ;

   //表格栏
   sacApp.controllerProvider.register( 'Data.Operate.Record.Grid.Ctrl', function( $scope, $timeout, SdbRest, SdbSwap, SdbSignal, SdbFunction ){
      var isFirst = true ;
      $scope.RecordNum = 0 ;

      $scope.Records = [] ;
      $scope.RecordTable = {
         'title': {},
         'options': {},
         'callback': {}
      } ;

      var records1 = [] ;
      var recordTable1 = {
         'title': {
            '#': '#',
            '-': '',
            'Record': 'Record'
         },
         'options': {
            'mode': 'dynamic',
            'trim': false,
            'width': {
               '#': '60px',
               '-': '80px',
               'Record': '100%' 
            },
            'max': 30
         }
      } ;

      var records2 = [] ;
      var recordTable2 = {
         'title': {
            'key':   'key',
            'value': 'value',
            'type':  'type'
         },
         'options': {
            'mode': 'dynamic',
            'trim': false,
            'width': {
               'key':   '33%',
               'value': '33%',
               'type':  '34%' 
            },
            'max': 30
         }
      } ;

      var records3 = [] ;
      var recordTable3 = {
         'title': { '#2': '#' },
         'options': {
            'mode': 'dynamic',
            'trim': true,
            'width': { '#': '100px' },
            'max': 30,
            'isRenderHide': false
         }
      } ;

      $scope.RecordTable['title'] = recordTable1['title'] ;
      $scope.RecordTable['options'] = recordTable1['options'] ;
     
      //上一页
      function previousPage()
      {
         SdbSwap.queryFilter['skip'] -= SdbSwap.limit ;
         SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;
      }

      //下一页
      function nextPage()
      {
         SdbSwap.queryFilter['skip'] += SdbSwap.limit ;
         SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;
      }

      //第一页
      function firstPage()
      {
         SdbSwap.queryFilter['skip'] = 0 ;
         SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;
      }

      //最后一页
      function lastPage()
      {
         var totalPage ;

         totalPage = $scope.RecordTable['callback']['GetSumPageNum']() ;

         SdbSwap.queryFilter['skip'] = ( totalPage - 1 ) * SdbSwap.limit ;
         SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;
      }

      //跳转到指定页
      var gotoPage = function( pageNum ){
         SdbSwap.queryFilter['skip'] = ( pageNum - 1 ) * SdbSwap.limit ;
         SdbSwap.queryRecord( SdbSwap.queryFilter, true ) ;
      }

      SdbSignal.on( 'goto_page', function( num ){
         $scope.RecordTable['callback']['Jump']( num ) ;
      } ) ;

      SdbSignal.on( 'update_records', function( records ){

         $scope.RecordNum = records.length ;
         SdbSwap.records1 = records ;
         SdbSwap.records2 = [] ;

         if( $scope.TableType == 1 )
         {
            $scope.Records = [] ;

            //把过长的记录缩短
            $.each( SdbSwap.records1, function( index, record ){
               if( record.length > 1024 )
               {
                  $scope.Records.push( { 'Record': record.substring( 0, 1024 ) + '...' } ) ;
               }
               else
               {
                  $scope.Records.push( { 'Record': record } ) ;
               }
               //填空
               SdbSwap.records2.push( null ) ;
            } ) ;

            $scope.RecordTable['title'] = recordTable1['title'] ;
            $scope.RecordTable['options'] = recordTable1['options'] ;
         }
         else if( $scope.TableType == 2 )
         {
            $scope.Records = [] ;

            $.each( records, function( index, record ){
               var recordObj = JSON.parse( record ) ;
               SdbSwap.records2.push( recordObj ) ;

               var line = json2Array( recordObj, 0, true ) ;
               $scope.Records.push( { 'json': line, 'index': index + 1 } ) ;
            } ) ;

            $scope.RecordTable['title'] = recordTable2['title'] ;
            $scope.RecordTable['options'] = recordTable2['options'] ;
         }
         else if( $scope.TableType == 3 )
         {
            $.each( records, function( index, record ){
               SdbSwap.records2[index] = JSON.parse( record ) ;
            } ) ;

            $scope.RecordTable['title'] = recordTable3['title'] ;
            $scope.RecordTable['options'] = recordTable3['options'] ;

            SdbSignal.commit( 'update_fields', SdbSwap.records2 ) ;
         }

         if( isFirst )
         {
            initTable( $scope.RecordTable['callback'] ) ;
            isFirst = false ;
         }

      } ) ;

      SdbSignal.on( 'update_table3_key', function( fieldList ){
         $scope.RecordTable['title'] = { '#': '#' } ;

         $.each( fieldList, function( index, field ){
            $scope.RecordTable['title'][field] = field ;
         } ) ;

         $scope.Records = [] ;

         $.each( SdbSwap.records2, function( index, record ){
            var newRecord = SdbFunction.filterJson( record, fieldList ) ;

            $scope.Records.push( newRecord ) ;
         } ) ;
      } ) ;

      SdbSignal.on( 'update_total', function( total ){
         $timeout( function(){
            if( SdbSwap.isNotFilter )
            {
               //无任何条件的查询
               recordTable1['options']['max'] = 30 ;
               recordTable2['options']['max'] = 30 ;
               recordTable3['options']['max'] = 30 ;
               $scope.RecordTable['options']['max'] = 30 ;

               $scope.RecordTable['callback']['SetToolText']( null ) ;
            }
            else
            {
               recordTable1['options']['max'] = 100 ;
               recordTable2['options']['max'] = 100 ;
               recordTable3['options']['max'] = 100 ;
               $scope.RecordTable['options']['max'] = 100 ;

               $scope.RecordTable['callback']['SetToolText']( '' ) ;
            }
            $scope.RecordTable['callback']['SetTotalNum']( total ) ;
         }, 100 ) ;
      } ) ;

      function initTable( callback )
      {
         $timeout( function(){
            var active = { 'style': { 'color': '#09D1A4' } } ;
            var unactive = { 'style': { 'color': '#555' } } ;

            callback['SetToolPageButton']( 'first', firstPage ) ;
            callback['SetToolPageButton']( 'previous', previousPage ) ;
            callback['SetToolPageButton']( 'next', nextPage ) ;
            callback['SetToolPageButton']( 'last', lastPage ) ;
            callback['SetToolPageButton']( 'jump', gotoPage ) ;

            callback['AddToolButton']( 'fa-font', { 'position': 'left', 'style': { 'color': '#09D1A4' } }, function(){

               SdbSignal.commit( 'update_table_type', 1 ) ;

               callback['SetToolButton']( 'fa-font', active ) ;
               callback['SetToolButton']( 'fa-list', unactive ) ;
               callback['SetToolButton']( 'fa-table', unactive ) ;

               $scope.Records = [] ;

               //把过长的记录缩短
               $.each( SdbSwap.records1, function( index, record ){
                  if( record.length > 1024 )
                  {
                     $scope.Records.push( { 'Record': record.substring( 0, 1024 ) + '...' } ) ;
                  }
                  else
                  {
                     $scope.Records.push( { 'Record': record } ) ;
                  }
               } ) ;

               $scope.RecordTable['title'] = recordTable1['title'] ;
               $scope.RecordTable['options'] = recordTable1['options'] ;
            } ) ;

            callback['AddToolButton']( 'fa-list', { 'position': 'left' }, function(){

               SdbSignal.commit( 'update_table_type', 2 ) ;

               callback['SetToolButton']( 'fa-font', unactive ) ;
               callback['SetToolButton']( 'fa-list', active ) ;
               callback['SetToolButton']( 'fa-table', unactive ) ;

               $.each( SdbSwap.records2, function( index, record ){
                  if( record === null )
                  {
                     SdbSwap.records2[index] = JSON.parse( SdbSwap.records1[index] ) ;
                  }
               } ) ;

               $scope.Records = [] ;
               $.each( SdbSwap.records2, function( index, record ){
                  var line = json2Array( record, 0, true ) ;
                  $scope.Records.push( { 'json': line, 'index': index + 1 } ) ;
               } ) ;

               $scope.RecordTable['title'] = recordTable2['title'] ;
               $scope.RecordTable['options'] = recordTable2['options'] ;

               $timeout( function(){
                  $scope.bindResize() ;
               } ) ;

            } ) ;

            callback['AddToolButton']( 'fa-table', { 'position': 'left' }, function(){

               SdbSignal.commit( 'update_table_type', 3 ) ;

               callback['SetToolButton']( 'fa-font', unactive ) ;
               callback['SetToolButton']( 'fa-list', unactive ) ;
               callback['SetToolButton']( 'fa-table', active ) ;

               $.each( SdbSwap.records2, function( index, record ){
                  if( record === null )
                  {
                     SdbSwap.records2[index] = JSON.parse( SdbSwap.records1[index] ) ;
                  }
               } ) ;

               $scope.RecordTable['title'] = recordTable3['title'] ;
               $scope.RecordTable['options'] = recordTable3['options'] ;

               SdbSignal.commit( 'update_fields', SdbSwap.records2 ) ;
            } ) ;
         }, 100 ) ;
      }

      //显示记录窗口
      $scope.ShowView = function( recordIndex ){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Data' ) == false )
         {
            return ;
         }

         $scope.ViewWindows['callback']['SetTitle']( $scope.autoLanguage( '记录' ) ) ;

         if( SdbSwap.records2[recordIndex] == null )
         {
            SdbSwap.records2[recordIndex] = JSON.parse( SdbSwap.records[recordIndex] ) ;
         }

         $scope.ViewWindows['data'] = SdbSwap.records2[recordIndex] ;

         $scope.ViewWindows['callback']['Open']() ;

         $scope.ViewWindows['callback']['SetResize']( function( left, top, width, height ){
            if( typeof( $scope.ViewWindows['dataCallback']['Resize'] ) == 'function' )
            {
               $scope.ViewWindows['dataCallback']['Resize']() ;
            }
         } ) ;

      }

   } ) ;

}());