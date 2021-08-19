//@ sourceURL=Data.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.SQL.Data.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql' || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      var moduleInfo = SdbFunction.LocalData( 'SdbModuleInfo' ) ;
      if( moduleInfo == null )
      {
         $location.path( 'Data/SQL-Metadata/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }
      try{
         moduleInfo = JSON.parse( moduleInfo ) ;
      }catch( e ){
         $location.path( 'Data/SQL-Metadata/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }
      var dbUser = moduleInfo['User'] ;
      var dbPwd  = moduleInfo['Pwd'] ;
      var dbName = moduleInfo['DbName'] ;
      var tbName = moduleInfo['TbName'] ;
      $scope.dbHost = moduleInfo['Host'] ;
      $scope.dbPort = moduleInfo['Port'] ;
      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName ) ;

      //数据库 + 表名
      $scope.fullName = dbName + '.' + tbName ;
      //执行结果
      $scope.execRc = true ;
      //执行结果描述
      $scope.execResult = '' ;
      //当前的sql语句
      $scope.sqlcmd = '' ;
      //执行结果的内容
      $scope.result = [] ;
      //最多保留多少行内容
      var maxLine = 9999 ;
      //字段表
      var fieldList = [] ;

      //数据库操作
      var sequoiasqlOperate = function( db, user, pwd, sql, success ){
         var state = { 'status': 0 } ;
         var data = { 'cmd': 'ssql exec', 'DbName': db, 'User': user, 'Passwd': pwd, 'Sql': sql, 'ResultFormat': 'pretty' } ;
         SdbRest.SequoiaSQL( data, function( taskInfo, isEnd ){
            success( taskInfo, isEnd ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               sequoiasqlOperate( db, user, pwd, sql, success ) ;
               return true ;
            } ) ;
         }, function(){
            //_IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      $scope.explain = function(){
         var sql = trim( $scope.sqlcmd ) ;
         if( /^explain/i.test( sql ) == true )
         {
            return ;
         }
         $scope.sqlcmd = 'EXPLAIN ' + $scope.sqlcmd ;
      }
      
      //更新结果
      var updateResult = function( str ){
         $scope.result.push( str ) ;
         if( $scope.result.length > maxLine )
         {
            $scope.result.shift() ;
         }
      }

      //执行按钮
      $scope.execSQL = function(){
         var state = { 'status': 0 } ;
         var sql = $scope.sqlcmd ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               updateResult( taskInfo[i]['Value'] ) ;
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
            }
            if( isEnd == true )
            {
               if( state['rc'] == false )
               {
                  $scope.execRc = false ;
                  $scope.execResult = sprintf( '? ?', timeFormat( new Date(), 'hh:mm:ss' ), state['result'] ) ;
                  updateResult( "\r\n" ) ;
               }
               else
               {
                  $scope.execRc = true ;
                  $scope.execResult = sprintf( $scope.autoLanguage( '? 执行SQL成功。' ), timeFormat( new Date(), 'hh:mm:ss' ) ) ;
                  updateResult( "\r\n" ) ;
               }
            }
         } ) ;
      }

      //默认加载页面马上查询一次
      $scope.sqlcmd = 'SELECT * FROM ' + tbName + ' LIMIT 30' ;
      $scope.execSQL() ;

      var selectGridData = [] ;
      //创建查询窗口
      $scope.Query = function(){
         $scope.Components.Modal.icon = 'fa-search' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '查询' ) ;
         $scope.Components.Modal.isScroll = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            type: 'grid',
            gridTitle: [
               { 'text': 'Column' },
               { 'text': 'Type' },
               { 'text': 'Operator' },
               { 'text': 'Value' }
            ],
            grid: { 'tdModel': 'auto', 'gridModel': 'fixed', titleWidth: [ 30, 20, '100px', 50 ] },
            inputList: $.extend( true, {}, selectGridData )
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function( data ){
            var value = $scope.Components.Modal.form.getValue() ;
            var isFirst = true ;
            var sql = 'SELECT * FROM ' + tbName ;
            var operate = '=' ;
            var length = value.length ;
            for( var i = 0; i < length; ++i )
            {
               var field   = value[i][0] ;
               var operate = value[i][2] ;
               var param   = value[i][3] ;
               if( typeof( param ) == 'string' && param.length > 0 )
               {
                  if( isFirst )
                  {
                     sql += ' WHERE ' ;
                     isFirst = false ;
                  }
                  else
                  {
                     sql += ' AND ' ;
                  }
                  switch( operate )
                  {
                  case '=':
                  case '>':
                  case '>=':
                  case '<':
                  case '<=':
                  case '!=':
                     sql += fieldList[i] + operate + sqlEscape(param) ;
                     break ;
                  case 'LIKE':
                  case 'NOT LIKE':
                     sql += fieldList[i] + ' ' + operate + ' ' + sqlEscape(param) ;
                     break ;
                  case 'BETWEEN':
                  case 'NOT BETWEEN':
                     var paramArr = param.split( ',', 2 ) ;
                     if( paramArr.length > 0 )
                     {
                        sql += fieldList[i] + ' ' + operate + ' ' + sqlEscape(paramArr[0]) + ' AND ' + sqlEscape(paramArr[1]) ;
                     }
                     break ;
                  case 'IS NULL':
                  case 'IS NOT NULL':
                     sql += fieldList[i] + ' ' + operate ;
                     break ;
                  case 'IN':
                  case 'NOT IN':
                     var tmp = trim( param ) ;
                     if( tmp.charAt(0) == '(' && tmp.charAt(tmp.length - 1) == ')' )
                     {
                        sql += fieldList[i] + ' ' + operate + ' ' + tmp ;
                     }
                     else
                     {
                        var paramArr = param.split( ',' ) ;
                        if( paramArr.length > 0 )
                        {
                           sql += fieldList[i] + ' ' + operate + ' (' ;
                           $.each( paramArr, function( index, subPara ){
                              if( index > 0 )
                              {
                                 sql += ',' ;
                              }
                              sql += sqlEscape( subPara ) ;
                           } ) ;
                           sql += ')' ;
                        }
                     }
                     break ;
                  }
               }
            }
            $scope.sqlcmd = sql ;
            $scope.execSQL() ;
            return true ;
         }
         $scope.Components.Modal.onResize = function( width, height ){
            var timer = setInterval( function(){
               if( typeof( $scope.Components.Modal.form.grid.onResize ) == 'function' )
               {
                  $scope.Components.Modal.form.grid.onResize( width, height ) ;
                  clearInterval( timer ) ;
               }
            }, 10 ) ;
         }
      }

      var insertGridData = [] ;
      //创建插入窗口
      $scope.Insert = function(){
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '插入' ) ;
         $scope.Components.Modal.isScroll = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            type: 'grid',
            gridTitle: [
               { 'text': 'Column' },
               { 'text': 'Type' },
               { 'text': 'NULL' },
               { 'text': 'Value' }
            ],
            grid: { 'tdModel': 'auto', 'gridModel': 'fixed', titleWidth: [ 30, 20, '100px', 50 ] },
            inputList: $.extend( true, {}, insertGridData )
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function( data ){
            var value = $scope.Components.Modal.form.getValue() ;
            var fields = '' ;
            var params = '' ;
            var sql = 'INSERT INTO ' + tbName + ' ' ;
            var length = value.length ;
            for( var i = 0; i < length; ++i )
            {
               if( value[i][3] == null )
               {
                  continue ;
               }
               if( i > 0 )
               {
                  fields += ',' ;
                  params += ',' ;
               }
               fields += value[i][0] ;
               if( value[i][2] == false )
               {
                  params += sqlEscape( value[i][3] ) ;
               }
               else
               {
                  params += 'NULL' ;
               }
            }
            fields = '(' + fields + ')' ;
            params = 'VALUES (' + params + ')' ;
            sql += fields + ' ' + params ;
            $scope.sqlcmd = sql ;
            $scope.execSQL() ;
            return true ;
         }
         $scope.Components.Modal.onResize = function( width, height ){
            var timer = setInterval( function(){
               if( typeof( $scope.Components.Modal.form.grid.onResize ) == 'function' )
               {
                  $scope.Components.Modal.form.grid.onResize( width, height ) ;
                  clearInterval( timer ) ;
               }
            }, 10 ) ;
         }
      }
      
      //获取表结构
      var state = { 'status': 0 } ;
      var sql = '\\d+ ' + tbName ;
      sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
         var length = taskInfo.length ;
         for( var i = 0, k = 0; i < length; ++i )
         {
            state = parseSSQL( taskInfo[i]['Value'], state ) ;
            if( state['status'] == 4 )
            {
               fieldList.push( state['value'][0] ) ;
               var selectLine = [
                  { 'type': 'textual', 'value': state['value'][0] },
                  { 'type': 'textual', 'value': state['value'][1] },
                  { 'type': 'select', 'value': '=', 'valid': [
                        { 'key': '=', 'value': '=' },
                        { 'key': '>', 'value': '>' },
                        { 'key': '>=', 'value': '>=' },
                        { 'key': '<', 'value': '<' },
                        { 'key': '<=', 'value': '<=' },
                        { 'key': '!=', 'value': '!=' },
                        { 'key': 'LIKE', 'value': 'LIKE' },
                        { 'key': 'NOT LIKE', 'value': 'NOT LIKE' },
                        { 'key': 'IN(...)', 'value': 'IN' },
                        { 'key': 'NOT IN(...)', 'value': 'NOT IN' },
                        { 'key': 'BETWEEN', 'value': 'BETWEEN' },
                        { 'key': 'NOT BETWEEN', 'value': 'NOT BETWEEN' },
                        { 'key': 'IS NULL', 'value': 'IS NULL' },
                        { 'key': 'IS NOT NULL', 'value': 'IS NOT NULL' }
                     ] },
                  { 'type': 'string' }
               ]
               var insertLine = [
                  { 'type': 'textual', 'value': state['value'][0] },
                  { 'type': 'textual', 'value': state['value'][1] },
                  { 'type': 'checkbox', 'value': false },
                  { 'type': 'string' }
               ] ;
               selectGridData.push( selectLine ) ;
               insertGridData.push( insertLine ) ;
               ++k ;
            }
            if( state['status'] == 7 )
            {
               break ;
            }
         }
         if( isEnd == true && state['rc'] == false )
         {
            _IndexPublic.createRetryModel( $scope, null, function(){
               $scope.queryTableStruct() ;
               return true ;
            }, $scope.autoLanguage( '获取数据库列表失败' ), state['result'] ) ;
         }
      } ) ;

   } ) ;
}());