//@ sourceURL=Data.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Data.Ctrl', function( $scope, $location, SdbSignal, SdbSwap, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      SdbSwap.dbName = SdbFunction.LocalData( 'PgsqlDbName' ) ;
      SdbSwap.tbName = SdbFunction.LocalData( 'PgsqlTbName' ) ;
      SdbSwap.tbType = SdbFunction.LocalData( 'PgsqlTbType' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-postgresql' || $scope.ModuleName == null || SdbSwap.dbName == null || SdbSwap.tbName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      SdbSwap.execSql = '' ;

      SdbSwap.dataList = [] ;

      SdbSwap.dataLengthList = [] ;

      //执行记录
      SdbSwap.execHistory = [] ;

      //数据库 + 表名
      $scope.FullName = SdbSwap.dbName + '.' + SdbSwap.tbName ;

      //执行sql 
      //noEditSql 为true时不修改sql输入框
      SdbSignal.on( 'exec_sql', function( data ){
         var sql = data['sql'] ;
         var isUser = data['isUser'] ;
         var isUpdateSql = data['isUpdateSql'] ;
         var gotoFirst = data['gotoFirst'] ;
         var execSql = sql ;
         var isQuery = trim( sql ).toLowerCase().indexOf( 'select' ) == 0 ;

         if( isQuery )
         {
            if( gotoFirst )
            {
               SdbSwap.execSql = sql ;
               SdbSignal.commit( 'goto_page', 1 ) ;
               return ;
            }
            else
            {
               execSql = sprintf( 'select * from (?) as t1 limit 30 offset ?', sql, SdbSwap.offset ) ;
            }
         }
         
         if( isUser && isUpdateSql )
         {
            SdbSwap.execHistory.push( { 'execTime': timeFormat( new Date(), 'hh:mm:ss' ), 'execSql': sql } ) ;
            SdbSignal.commit( 'updateSqlCommand', sql ) ;
         }

         var data = { 'Sql': execSql, 'DbName': SdbSwap.dbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               if( isQuery )
               {
                  var count = result.length ;
                  SdbSwap.dataList = result ;
                  SdbSwap.dataLengthList = [] ;
                  var displayData = [] ;
                  displayData = $.extend( true, [], result ) ;
                  $.each( displayData, function( index, data ){
                     SdbSwap.dataLengthList.push( false ) ;
                     $.each( data, function( key, value ){
                        if( value != null )
                        {
                           if( value.toString().length > 1024 )
                           {
                              displayData[index][key] = value.substring( 0, 1024 ) + '...' ;
                              SdbSwap.dataLengthList[index] = true ;
                           }
                        }
                     } ) ;
                  } ) ;

                  if( isUpdateSql )
                  {
                     SdbSwap.lastQuerySql = sql ;
                  }

                  //设置表格显示
                  SdbSignal.commit( 'setTableData', displayData ) ;

                  //更新表格标题
                  SdbSignal.commit( 'setTableTitle', displayData ) ;

                  if( isUser )
                  {
                     var start = 0 ;
                     var end   = 0 ;

                     if( count > 0 )
                     {
                        start = SdbSwap.offset + 1 ;
                        end   = SdbSwap.offset + SdbSignal.commit( 'getTableLength' )[0] ;
                     }
                     //执行结果
                     var str = sprintf( $scope.pAutoLanguage( '? ? 执行查询成功，显示 ? - ? 记录' ),
                                        timeFormat( new Date(), 'hh:mm:ss' ),
                                        $scope.FullName,
                                        start, end ) ;
                     SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                  }

               }
               else
               {
                  if( isUser )
                  {
                     //执行结果
                     var str = sprintf( $scope.pAutoLanguage( '? ? 执行成功' ),
                                          timeFormat( new Date(), 'hh:mm:ss' ),
                                          $scope.FullName ) ;
                     SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;
                     SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': false } ) ;
                  }
               }
            },
            'failed': function( errorInfo ){
               var str = '' ;
               if( errorInfo['description'].length == 0 )
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['detail'] ) ;
               }
               else if( errorInfo['detail'].length == 0 )
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['description'] ) ;
               }
               else
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?. ?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['description'],
                                 errorInfo['detail'] ) ;
               }
               SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
            }
         } ) ;
      } ) ;

      //执行其他操作
      SdbSignal.on( 'execOperate', function( data ){
         var sql = data['sql'] ;
         var type = data['type'] ;
         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName } ;
         SdbSwap.execHistory.push( { 'execTime': timeFormat( new Date(), 'hh:mm:ss' ), 'execSql': sql } ) ;
         SdbSignal.commit( 'updateSqlCommand', sql ) ;

         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               //执行结果
               var str = sprintf( $scope.pAutoLanguage( '? ? 执行?成功' ),
                                  timeFormat( new Date(), 'hh:mm:ss' ),
                                  $scope.FullName,
                                  $scope.pAutoLanguage( type ) ) ;
               SdbSignal.commit( 'update_result', { 'rc': true, 'result': str } ) ;

               //查询数据
               SdbSignal.commit( 'goto_page', 2 ) ;
            },
            'failed': function( errorInfo ){
               var str = '' ;
               if( errorInfo['description'].length == 0 )
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['detail'] ) ;
               }
               else if( errorInfo['detail'].length == 0 )
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['description'] ) ;
               }
               else
               {
                  str = sprintf( $scope.pAutoLanguage( '? ? 执行失败，错误码: ?，?. ?' ),
                                 timeFormat( new Date(), 'hh:mm:ss' ),
                                 $scope.FullName,
                                 errorInfo['errno'],
                                 errorInfo['description'],
                                 errorInfo['detail'] ) ;
               }
               SdbSignal.commit( 'update_result', { 'rc': false, 'result': str } ) ;
            }
         } ) ;
      } ) ;

   } ) ;

   //操作 控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Data.Operate.Ctrl', function( $scope, $location, SdbSwap, SdbSignal, SdbRest ){
      
      //跳过记录数
      SdbSwap.offset = 0 ;
      
      //查询的记录数
      var count = 0 ;

      //插入弹窗表格数据
      var insertGridData = [] ;

      //字段表
      var fieldList = [] ;

      //最后一次查询sql
      SdbSwap.lastQuerySql = '' ;

      //插入弹窗错误提示
      $scope.InserError = '' ;

      //获取字段列表
      var queryTableStruct = function(){
         var sql = sprintf("SELECT \
                   ordinal_position, \
                   column_name, \
                   data_type, \
                   character_maximum_length, \
                   numeric_precision, \
                   numeric_scale, \
                   is_nullable, \
                   column_default \
                   FROM \
                   information_schema.columns \
                   WHERE \
                   table_name = '?'", SdbSwap.tbName ) ;
         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName, 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               fieldList = result ;
               $.each( fieldList, function( index, fieldInfo ){
                  disabled = fieldInfo['is_nullable'] == 'YES' ? false : true ;
                  var insertLine = {
                     'Column' : { 'type': 'textual', 'value': fieldInfo['column_name'] },
                     'Type'   : { 'type': 'textual', 'value': fieldInfo['data_type'] },
                     'NULL'   : { 'type': 'checkbox', 'value': false, 'disabled': disabled },
                     'Value'  : { 'type': 'string' }
                  } ;
                  insertGridData.push( insertLine ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryTableStruct() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      queryTableStruct() ;

      //查询数据
      var queryCtid = function( querySql, type, lastSql ){
         var data = { 'Sql': querySql, 'DbName': SdbSwap.dbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               var ctid = result[0]['ctid'] ;
               var sql = '' ;
               if( type == 'delete' )
               {
                  sql = sprintf( 'DELETE FROM ? WHERE ctid = ?', addQuotes( SdbSwap.tbName ), sqlEscape( ctid ) ) ;
                  type = '删除记录' ;
               }
               else if( type == 'update' )
               {
                  sql = sprintf( '? WHERE ctid = ?', lastSql, sqlEscape( ctid ) ) ;
                  type = '更新记录' ;
               }
               SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': type } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryData() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      $scope.QueryWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 查询弹窗
      $scope.ShowQueryWindow = function(){
         if( fieldList.length == 0 )
         {
            _IndexPublic.createInfoModel( $scope, $scope.pAutoLanguage( "当前数据表没有字段，无法进行该操作，是否前往表结构页面？" ), $scope.pAutoLanguage( '确定' ), function(){
               $location.path( '/Data/SequoiaSQL/PostgreSQL/Operate/Structure' ).search( { 'r': new Date().getTime() } ) ;
            } ) ;
         }
         else
         {
            var fieldsSelect = [] ;
            $.each( fieldList, function( index, fieldInfo ){
               fieldsSelect.push( { 'key': fieldInfo['column_name'], 'value': fieldInfo['column_name'] } ) ;
            } ) ;
            $scope.QueryWindow['config'] = {
               'inputList': [
                  {
                     "name": "filter",
                     "webName": $scope.pAutoLanguage( '查询条件' ),
                     "type": "group",
                     "child": [
                        {
                           "name": "model",
                           "webName": $scope.pAutoLanguage( "匹配模式" ),
                           "type": "select",
                           "value": "and",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "满足所有条件" ), "value": "and" },
                              { "key": $scope.pAutoLanguage( "满足任意条件" ), "value": "or" }
                           ]
                        },
                        {
                           "name": "condition",
                           "webName": $scope.pAutoLanguage( "匹配条件" ),
                           "type": "list",
                           "valid": {
                              "min": 0
                           },
                           "child": [
                              [
                                 {
                                    "name": "field",
                                    "type": "select",
                                    "value": '',
                                    "default": '',
                                    "valid": fieldsSelect
                                 },
                                 {
                                    "name": "logic",
                                    "type": "select",
                                    "value": ">",
                                    "default": ">",
                                    "valid": [
                                       { 'key': '=', 'value': '=' },
                                       { 'key': '>', 'value': '>' },
                                       { 'key': '>=', 'value': '>=' },
                                       { 'key': '<', 'value': '<' },
                                       { 'key': '<=', 'value': '<=' },
                                       { 'key': '<>', 'value': '<>' },
                                       { 'key': 'LIKE', 'value': 'LIKE' },
                                       { 'key': 'NOT LIKE', 'value': 'NOT LIKE' },
                                       { 'key': 'IN(...)', 'value': 'IN' },
                                       { 'key': 'NOT IN(...)', 'value': 'NOT IN' },
                                       { 'key': 'BETWEEN', 'value': 'BETWEEN' },
                                       { 'key': 'NOT BETWEEN', 'value': 'NOT BETWEEN' },
                                       { 'key': 'IS NULL', 'value': 'IS NULL' },
                                       { 'key': 'IS NOT NULL', 'value': 'IS NOT NULL' }
                                    ]
                                 },
                                 {
                                    "name": "value",
                                    "webName": $scope.pAutoLanguage( "值" ),
                                    "placeholder": $scope.pAutoLanguage( "值" ),
                                    "type": "string",
                                    "value": "",
                                    "valid": {
                                       "min": 1
                                    }
                                 }
                              ]
                           ]
                        }
                     ]
                  },
                  {
                     "name": "sort",
                     "webName": $scope.autoLanguage( "排序字段" ),
                     "type": "list",
                     "valid": {
                        "min": 0,
                        "max": 100
                     },
                     "child": [
                        [
                           {
                              "name": "field",
                              "type": "select",
                              "value": '',
                              "default": fieldsSelect[0]['key'],
                              "valid": fieldsSelect
                           },
                           {
                              "name": "order",
                              "type": "select",
                              "value": "ASC",
                              "valid": [
                                 { "key": $scope.autoLanguage( "升序" ), "value": "ASC" },
                                 { "key": $scope.autoLanguage( "降序" ), "value": "DESC" }
                              ]
                           }
                        ]
                     ]
                  },
                  {
                     "name": "returnnum",
                     "webName": $scope.pAutoLanguage( "返回记录数" ),
                     "required": true,
                     "type": "int",
                     "valid": {
                        "min": 1,
                        "max": 100
                     },
                     "value": 30
                  },
                  {
                     "name": "skip",
                     "webName": $scope.pAutoLanguage( "跳过记录数" ),
                     "required": true,
                     "type": "int",
                     "valid": {
                        "min": 0,
                        "max": 9007199254740991
                     },
                     "value": 0
                  },
                  {
                     "name": "execType",
                     "webName": $scope.pAutoLanguage( "执行模式" ),
                     "type": "select",
                     "value": 0,
                     "valid": [
                        { 'key': $scope.pAutoLanguage( "马上执行" ), 'value': 0 },
                        { 'key': $scope.pAutoLanguage( "仅生成SQL" ), 'value': 1 }
                     ]
                  }
               ]
            } ;
         
            $scope.QueryWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var isClear = $scope.QueryWindow['config'].check() ;
               if( isClear )
               {
                  var formValue = $scope.QueryWindow['config'].getValue() ;
                  var isFirst = true ;
                  var sql = sprintf( 'SELECT * FROM ?', addQuotes( SdbSwap.tbName ) ) ;
                  $.each( formValue['filter']['condition'], function( index, info ){
                     if( info['field'].length > 0 && ( info['value'].length > 0 || info['logic'] == 'IS NULL' || info['logic'] == 'IS NOT NULL' ) )
                     {
                        var field   = addQuotes( info['field'] ) ;
                        var operate = info['logic'] ;
                        var param   = info['value'] ;
                        if( isFirst == true )
                        {
                           sql += ' WHERE ' ;
                           isFirst = false ;
                        }
                        else
                        {
                           sql += sprintf( ' ? ', formValue['filter']['model'] ) ;
                        }
                        switch( operate )
                        {
                        case '=':
                        case '>':
                        case '>=':
                        case '<':
                        case '<=':
                        case '<>':
                        case 'LIKE':
                        case 'NOT LIKE':
                           sql += field + ' ' + operate + ' ' + sqlEscape( param ) ;
                           break ;
                        case 'BETWEEN':
                        case 'NOT BETWEEN':
                           var paramArr = param.split( ',', 2 ) ;
                           sql += field + ' ' + operate + ' ' + sqlEscape( paramArr[0] ) ;
                           if( paramArr.length > 1 )
                           {
                              sql += ' AND ' + sqlEscape( paramArr[1] ) ;
                           }
                           break ;
                        case 'IS NULL':
                        case 'IS NOT NULL':
                           sql += field + ' ' + operate ;
                           break ;
                        case 'IN':
                        case 'NOT IN':
                           var tmp = trim( param ) ;
                           if( tmp.charAt(0) == '(' && tmp.charAt(tmp.length - 1) == ')' )
                           {
                              sql += field + ' ' + operate + ' ' + tmp ;
                           }
                           else
                           {
                              var paramArr = param.split( ',' ) ;
                              if( paramArr.length > 0 )
                              {
                                 sql += field + ' ' + operate + ' (' ;
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
                  } ) ;
                  isFirst = true ;
                  $.each( formValue['sort'], function( index, info ){
                     if( info['field'].length > 0 )
                     {
                        if( isFirst == true )
                        {
                           sql += ' ORDER BY ' ;
                           isFirst = false ;
                        }
                        else
                        {
                           sql += ', ' ;
                        }
                        sql += sprintf( '? ?', addQuotes( info['field'] ), info['order'] ) ;
                     }
                  } ) ;
                  sql += sprintf( ' LIMIT ? OFFSET ?', formValue['returnnum'], formValue['skip'] ) ;

                  //执行
                  if( formValue['execType'] == 0 )
                  {
                     if( SdbSignal.commit( 'getTableLength' )[0] == 0 )
                     {
                        SdbSignal.commit( 'exec_sql', { 'sql': sql, 'isUser': true, 'isUpdateSql': true } ) ;
                     }
                     else
                     {
                        SdbSignal.commit( 'exec_sql', { 'sql': sql, 'isUser': true, 'isUpdateSql': true, 'gotoFirst': true } ) ;
                     }
                  }
                  else
                  {
                     SdbSignal.commit( 'updateSqlCommand', sql ) ;
                  }
                  $scope.QueryWindow['callback']['Close']() ;
               }
            } ) ;
            $scope.QueryWindow['callback']['SetIcon']( '' ) ;
            $scope.QueryWindow['callback']['SetTitle']( $scope.pAutoLanguage( '查询' ) ) ;
            $scope.QueryWindow['callback']['Open']() ;
         }
      }

      //更新弹窗
      $scope.UpdateWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 更新弹窗
      $scope.ShowUpdateWindow = function(){
         if( fieldList.length == 0 )
         {
            _IndexPublic.createInfoModel( $scope, $scope.pAutoLanguage( "当前数据表没有字段，无法进行该操作，是否前往表结构页面？" ), $scope.pAutoLanguage( '确定' ), function(){
               $location.path( '/Data/SequoiaSQL/PostgreSQL/Operate/Structure' ).search( { 'r': new Date().getTime() } ) ;
            } ) ;
         }
         else
         {
            var fieldsSelect = [] ;
            $.each( fieldList, function( index, fieldInfo ){
               fieldsSelect.push( { 'key': fieldInfo['column_name'], 'value': fieldInfo['column_name'] } ) ;
            } ) ;
            $scope.UpdateWindow['config'] = {
               'inputList': [
                  {
                     "name": "filter",
                     "webName": $scope.pAutoLanguage( '查询条件' ),
                     "type": "group",
                     "child": [
                        {
                           "name": "model",
                           "webName": $scope.pAutoLanguage( "匹配模式" ),
                           "type": "select",
                           "value": "and",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "满足所有条件" ), "value": "and" },
                              { "key": $scope.pAutoLanguage( "满足任意条件" ), "value": "or" }
                           ]
                        },
                        {
                           "name": "condition",
                           "webName": $scope.pAutoLanguage( "匹配条件" ),
                           "type": "list",
                           "valid": {
                              "min": 0
                           },
                           "child": [
                              [
                                 {
                                    "name": "field",
                                    "type": "select",
                                    "value": '',
                                    "default": fieldsSelect[0]['key'],
                                    "valid": fieldsSelect
                                 },
                                 {
                                    "name": "logic",
                                    "type": "select",
                                    "value": ">",
                                    "default": ">",
                                    "valid": [
                                       { 'key': '=', 'value': '=' },
                                       { 'key': '>', 'value': '>' },
                                       { 'key': '>=', 'value': '>=' },
                                       { 'key': '<', 'value': '<' },
                                       { 'key': '<=', 'value': '<=' },
                                       { 'key': '<>', 'value': '<>' },
                                       { 'key': 'LIKE', 'value': 'LIKE' },
                                       { 'key': 'NOT LIKE', 'value': 'NOT LIKE' },
                                       { 'key': 'IN(...)', 'value': 'IN' },
                                       { 'key': 'NOT IN(...)', 'value': 'NOT IN' },
                                       { 'key': 'BETWEEN', 'value': 'BETWEEN' },
                                       { 'key': 'NOT BETWEEN', 'value': 'NOT BETWEEN' },
                                       { 'key': 'IS NULL', 'value': 'IS NULL' },
                                       { 'key': 'IS NOT NULL', 'value': 'IS NOT NULL' }
                                    ]
                                 },
                                 {
                                    "name": "value",
                                    "webName": $scope.pAutoLanguage( "值" ),
                                    "placeholder": $scope.pAutoLanguage( "值" ),
                                    "type": "string",
                                    "value": "",
                                    "valid": {
                                       "min": 1
                                    }
                                 }
                              ]
                           ]
                        }
                     ]
                  },
                  {
                     "name": "updator",
                     "webName": $scope.pAutoLanguage( "更新操作" ),
                     "type": "list",
                     "desc": "",
                     "required": true,
                     "valid": {
                        "min": 1
                     },
                     "child": [
                        [
                           {
                              "name": "field",
                              "webName": $scope.pAutoLanguage( "字段名" ),
                              "placeholder": $scope.pAutoLanguage( "字段名" ),
                              "type": "string",
                              "value": "",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "value",
                              "webName": $scope.pAutoLanguage( "值" ),
                              "placeholder": $scope.pAutoLanguage( "值" ),
                              "type": "string",
                              "value": "",
                              "valid": {}
                           },
                           {
                              "name": "null",
                              "webName": $scope.pAutoLanguage( "空" ),
                              "type": "checkbox",
                              "value": false
                           }
                        ]
                     ]
                  },
                  {
                     "name": "execType",
                     "webName": $scope.pAutoLanguage( "执行模式" ),
                     "type": "select",
                     "value": 0,
                     "valid": [
                        { 'key': $scope.pAutoLanguage( "马上执行" ), 'value': 0 },
                        { 'key': $scope.pAutoLanguage( "仅生成SQL" ), 'value': 1 }
                     ]
                  }
               ]
            } ;
            $scope.UpdateWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var isClear = $scope.UpdateWindow['config'].check() ;
               if( isClear )
               {
                  var formValue = $scope.UpdateWindow['config'].getValue() ;
                  var sql = sprintf( 'UPDATE ? SET ', addQuotes( SdbSwap.tbName ) ) ;
                  $.each( formValue['updator'], function( index, fieldInfo ){
                     if( fieldInfo['null'] == true )
                     {
                        sql += addQuotes( fieldInfo['field'] ) + ' = NULL ' ;
                     }
                     else
                     {
                        sql += addQuotes( fieldInfo['field'] ) + ' = ' + sqlEscape( fieldInfo['value'] ) ;
                     }
                     if( index + 1 < formValue['updator'].length )
                     {
                        sql += ', ' ;
                     }
                  } ) ;
                  var isFirst = true ;
                  $.each( formValue['filter']['condition'], function( index, filterInfo ){
                     var field = addQuotes( filterInfo['field'] ) ;
                     if( filterInfo['field'].length > 0 && ( filterInfo['value'].length > 0 || filterInfo['logic'] == 'IS NULL' || filterInfo['logic'] == 'IS NOT NULL' ) )
                     {
                        if( isFirst )
                        {
                           sql += ' WHERE ' ;
                           isFirst = false ;
                        }
                        else
                        {
                           sql += sprintf( ' ? ', formValue['filter']['model'] ) ;
                        }

                        switch( filterInfo['logic'] )
                        {
                        case '=':
                        case '>':
                        case '>=':
                        case '<':
                        case '<=':
                        case '<>':
                        case 'LIKE':
                        case 'NOT LIKE':
                           sql += field + ' ' + filterInfo['logic'] + ' ' + sqlEscape( filterInfo['value'] ) ;
                           break ;
                        case 'BETWEEN':
                        case 'NOT BETWEEN':
                           var paramArr = filterInfo['value'].split( ',', 2 ) ;
                           sql += field + ' ' + filterInfo['logic'] + ' ' + sqlEscape( paramArr[0] ) ;
                           if( paramArr.length > 1 )
                           {
                              sql += ' AND ' + sqlEscape( paramArr[1] ) ;
                           }
                           break ;
                        case 'IS NULL':
                        case 'IS NOT NULL':
                           sql += field + ' ' + filterInfo['logic'] ;
                           break ;
                        case 'IN':
                        case 'NOT IN':
                           var tmp = trim( filterInfo['value'] ) ;
                           if( tmp.charAt(0) == '(' && tmp.charAt(tmp.length - 1) == ')' )
                           {
                              sql += field + ' ' + filterInfo['logic'] + ' ' + tmp ;
                           }
                           else
                           {
                              var paramArr = filterInfo['value'].split( ',' ) ;
                              if( paramArr.length > 0 )
                              {
                                 sql += field + ' ' + filterInfo['logic'] + ' (' ;
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
                  } ) ;

                  //执行
                  if( formValue['execType'] == 0 )
                  {
                     SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '更新记录' } ) ;
                  }
                  else
                  {
                     SdbSignal.commit( 'updateSqlCommand', sql ) ;
                  }
                  $scope.UpdateWindow['callback']['Close']() ;
               }
            } ) ;
            $scope.UpdateWindow['callback']['SetIcon']( '' ) ;
            $scope.UpdateWindow['callback']['SetTitle']( $scope.pAutoLanguage( '更新' ) ) ;
            $scope.UpdateWindow['callback']['Open']() ;
         }
      }
      
      //插入弹窗
      $scope.InsertWindow = {
         'config': {
            'type': 'table',
            'table': {
               'title': {
                  'Column' : 'Column',
                  'Type'   : 'Type',
                  'NULL'   : 'NULL',
                  'Value'  : 'Value'
               },
               'options': {
                  'width': {
                     'Column' : '30%',
                     'Type'   : '20%',
                     'NULL'   : '100px',
                     'Value'  : '50%'
                  }
               },
               'callback': {}
            }
         },
         'callback': {}
      } ;

      //打开 插入弹窗
      $scope.ShowInsertWindow = function(){
         if( fieldList.length == 0 )
         {
            _IndexPublic.createInfoModel( $scope, $scope.pAutoLanguage( "当前数据表没有字段，无法进行该操作，是否前往表结构页面？" ), $scope.pAutoLanguage( '确定' ), function(){
               $location.path( '/Data/SequoiaSQL/PostgreSQL/Operate/Structure' ).search( { 'r': new Date().getTime() } ) ;
            } ) ;
         }
         else
         {
            $scope.InsertWindow['config']['inputList'] = $.extend( true, [], insertGridData ) ;
            $scope.InsertWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定'), function(){
               $scope.ErrorTip = '' ;
               var value = $scope.InsertWindow['config'].getValue() ;
               var fields = '' ;
               var params = '' ;
               var isFirst = true ;
               var sql = sprintf( 'INSERT INTO ? ', addQuotes( SdbSwap.tbName ) ) ;
               var length = value.length ;
               for( var i = 0; i < length; ++i )
               {
                  var field   = addQuotes( value[i][0] ) ;
                  var operate = value[i][2] ;
                  var param   = value[i][3] ;
                  
                  if( isFirst )
                  {
                     isFirst = false ;
                  }
                  else
                  {
                     fields += ',' ;
                     params += ',' ;
                  }
                  fields += field ;
                  if( operate == false )
                  {
                     if( typeof( param ) == 'undefined' )
                     {
                        params += '\'\'' ;
                     }
                     else
                     {
                        params += sqlEscape( param ) ;
                     }
                  }
                  else
                  {
                     params += 'NULL' ;
                  }
               }
               fields = '(' + fields + ')' ;
               params = 'VALUES (' + params + ')' ;
               sql += fields + ' ' + params ;
               SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '插入记录' } ) ;
               $scope.InsertWindow['callback']['Close']() ;

            } ) ;
            $scope.InsertWindow['callback']['SetIcon']( '' ) ;
            $scope.InsertWindow['callback']['SetTitle']( $scope.pAutoLanguage( '插入' ) ) ;
            $scope.InsertWindow['callback']['Open']() ;
         }
      }

      //删除弹窗
      $scope.DeleteWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除弹窗
      $scope.ShowDeleteWindow = function(){
         if( fieldList.length == 0 )
         {
            _IndexPublic.createInfoModel( $scope, $scope.pAutoLanguage( "当前数据表没有字段，无法进行该操作，是否前往表结构页面？" ), $scope.pAutoLanguage( '确定' ), function(){
               $location.path( '/Data/SequoiaSQL/PostgreSQL/Operate/Structure' ).search( { 'r': new Date().getTime() } ) ;
            } ) ;
         }
         else
         {
            var fieldsSelect = [] ;
            $.each( fieldList, function( index, fieldInfo ){
               fieldsSelect.push( { 'key': fieldInfo['column_name'], 'value': fieldInfo['column_name'] } ) ;
            } ) ;
            $scope.DeleteWindow['config'] = {
               'inputList':[
                  {
                     "name": "filter",
                     "webName": $scope.pAutoLanguage( '删除条件' ),
                     "type": "group",
                     "child": [
                        {
                           "name": "model",
                           "webName": $scope.pAutoLanguage( "匹配模式" ),
                           "type": "select",
                           "value": "and",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "满足所有条件" ), "value": "and" },
                              { "key": $scope.pAutoLanguage( "满足任意条件" ), "value": "or" }
                           ]
                        },
                        {
                           "name": "condition",
                           "webName": $scope.pAutoLanguage( "匹配条件" ),
                           "type": "list",
                           "valid": {
                              "min": 0
                           },
                           "child": [
                              [
                                 {
                                    "name": "field",
                                    "type": "select",
                                    "value": '',
                                    "default": fieldsSelect[0]['key'],
                                    "valid": fieldsSelect
                                 },
                                 {
                                    "name": "logic",
                                    "type": "select",
                                    "value": ">",
                                    "default": ">",
                                    "valid": [
                                       { 'key': '=', 'value': '=' },
                                       { 'key': '>', 'value': '>' },
                                       { 'key': '>=', 'value': '>=' },
                                       { 'key': '<', 'value': '<' },
                                       { 'key': '<=', 'value': '<=' },
                                       { 'key': '<>', 'value': '<>' },
                                       { 'key': 'LIKE', 'value': 'LIKE' },
                                       { 'key': 'NOT LIKE', 'value': 'NOT LIKE' },
                                       { 'key': 'IN(...)', 'value': 'IN' },
                                       { 'key': 'NOT IN(...)', 'value': 'NOT IN' },
                                       { 'key': 'BETWEEN', 'value': 'BETWEEN' },
                                       { 'key': 'NOT BETWEEN', 'value': 'NOT BETWEEN' },
                                       { 'key': 'IS NULL', 'value': 'IS NULL' },
                                       { 'key': 'IS NOT NULL', 'value': 'IS NOT NULL' }
                                    ]
                                 },
                                 {
                                    "name": "value",
                                    "webName": $scope.pAutoLanguage( "值" ),
                                    "placeholder": $scope.pAutoLanguage( "值" ),
                                    "type": "string",
                                    "value": "",
                                    "valid": {
                                       "min": 1
                                    }
                                 }
                              ]
                           ]
                        }
                     ]
                  },
                  {
                     "name": "execType",
                     "webName": $scope.pAutoLanguage( "执行模式" ),
                     "type": "select",
                     "value": 0,
                     "valid": [
                        { 'key': $scope.pAutoLanguage( "马上执行" ), 'value': 0 },
                        { 'key': $scope.pAutoLanguage( "仅生成SQL" ), 'value': 1 }
                     ]
                  }
               ]
            } ;

            $scope.DeleteWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var isClear = $scope.DeleteWindow['config'].check() ;
               if( isClear )
               {
                  var formValue = $scope.DeleteWindow['config'].getValue() ;
                  var sql = sprintf( 'DELETE FROM ?' ,addQuotes( SdbSwap.tbName ) ) ;
                  var isFirst = true ;
                  var condition = '' ;
                  $.each( formValue['filter']['condition'], function( index, info ){
                     if( info['field'].length > 0 && ( info['value'].length > 0 || info['logic'] == 'IS NULL' || info['logic'] == 'IS NOT NULL' ) )
                     {
                        var field   = addQuotes( info['field'] ) ;
                        var operate = info['logic'] ;
                        var param   = info['value'] ;
                        if( isFirst == true )
                        {
                           condition += ' WHERE ' ;
                           isFirst = false ;
                        }
                        else
                        {
                           condition += sprintf( ' ? ', formValue['filter']['model'] ) ;
                        }
                        switch( operate )
                        {
                        case '=':
                        case '>':
                        case '>=':
                        case '<':
                        case '<=':
                        case '<>':
                        case 'LIKE':
                        case 'NOT LIKE':
                           condition += field + ' ' + operate + ' ' + sqlEscape( param ) ;
                           break ;
                        case 'BETWEEN':
                        case 'NOT BETWEEN':
                           var paramArr = param.split( ',', 2 ) ;
                           condition += field + ' ' + operate + ' ' + sqlEscape( paramArr[0] ) ;
                           if( paramArr.length > 1 )
                           {
                              condition += ' AND ' + sqlEscape( paramArr[1] ) ;
                           }
                           break ;
                        case 'IS NULL':
                        case 'IS NOT NULL':
                           condition += field + ' ' + operate ;
                           break ;
                        case 'IN':
                        case 'NOT IN':
                           var tmp = trim( param ) ;
                           if( tmp.charAt(0) == '(' && tmp.charAt(tmp.length - 1) == ')' )
                           {
                              condition += field + ' ' + operate + ' ' + tmp ;
                           }
                           else
                           {
                              var paramArr = param.split( ',' ) ;
                              if( paramArr.length > 0 )
                              {
                                 condition += field + ' ' + operate + ' (' ;
                                 $.each( paramArr, function( index, subPara ){
                                    if( index > 0 )
                                    {
                                       condition += ',' ;
                                    }
                                    condition += sqlEscape( subPara ) ;
                                 } ) ;
                                 condition += ')' ;
                              }
                           }
                           break ;
                        }
                     }
                  } ) ;
                  //$.each( formValue['filter']['condition'], function( index, filterInfo ){
                  //   if( isEmpty( filterInfo['field'] ) || isEmpty( filterInfo['value'] ) )
                  //   {
                  //      return ;
                  //   }
                  //   if( index == 0 )
                  //   {
                  //      condition += ' WHERE ' ;
                  //   }
                  //   condition += addQuotes( filterInfo['field'] ) + ' ' + filterInfo['logic'] + ' ' + sqlEscape( filterInfo['value'] ) ;
                  //   if( index + 1 < formValue['filter']['condition'].length )
                  //   {
                  //      condition += ' ' + formValue['filter']['model'] + ' ' ;
                  //   }
                  //} ) ;

                  sql += condition ;

                  //执行
                  if( formValue['execType'] == 0 )
                  {
                     if( condition.length < 1 )
                     {
                        _IndexPublic.createInfoModel( $scope, $scope.pAutoLanguage( "执行当前的操作会删除所有记录！要继续吗？" ), $scope.pAutoLanguage( '继续' ), function(){
                           SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '删除记录' } ) ;
                           $scope.DeleteWindow['callback']['Close']() ;
                        } ) ;
                     }
                     else
                     {
                        SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '删除记录' } ) ;
                        $scope.DeleteWindow['callback']['Close']() ;
                     }
                  }
                  else
                  {
                     SdbSignal.commit( 'updateSqlCommand', sql ) ;
                     $scope.DeleteWindow['callback']['Close']() ;
                  }
               }
            } ) ;
            $scope.DeleteWindow['callback']['SetIcon']( '' ) ;
            $scope.DeleteWindow['callback']['SetTitle']( $scope.pAutoLanguage( '删除' ) ) ;
            $scope.DeleteWindow['callback']['Open']() ;
         }
      }

      //编辑单条记录弹窗
      $scope.EditWindow = {
         'config': {
            'type': 'table',
            'table': {
               'title': {
                  'Column' : 'Column',
                  'Type'   : 'Type',
                  'Value'  : 'Value'
               },
               'options': {
                  'width': {
                     'Column' : '30%',
                     'Type'   : '20%',
                     'Value'  : '50%'
                  }
               },
               'callback': {}
            }
         },
         'callback': {}
      } ;

      //打开 编辑单条记录弹窗
      SdbSignal.on( 'showEditWindow', function( index ){
         var showWindow = function( index )
         {
            var data = SdbSwap.dataList[index] ;
            $scope.ErrorTip = '' ;
            $scope.EditWindow['config']['inputList'] = [] ;
         
            $.each( fieldList, function( index, fieldInfo ){
               var insertLine = {
                  'Column' : { 'type': 'textual', 'value': fieldInfo['column_name'] },
                  'Type'   : { 'type': 'textual', 'value': fieldInfo['data_type'] },
                  'Value'  : { 'type': 'string', 'value': data[fieldInfo['column_name']] }
               } ;
               $scope.EditWindow['config']['inputList'].push( insertLine ) ;
            } ) ;

            $scope.EditWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               $scope.ErrorTip = '' ;
               var formValue = $scope.EditWindow['config'].getValue() ;

               if( SdbSwap.tbType == 'table' )
               {
                  var setSql = sprintf( 'UPDATE ? SET ', addQuotes( SdbSwap.tbName ) ) ;
                  var tempNum = 1 ;
                  $.each( formValue, function( index, fieldInfo ){
                     if( fieldInfo[2] != data[fieldInfo[0]] )
                     {
                        if( tempNum > 1 )
                        {
                           setSql += ', ' ;
                        }
                        setSql += addQuotes( fieldInfo[0] ) + ' = ' + sqlEscape( fieldInfo[2] ) ;
                        ++ tempNum ;
                     }
                     else
                     {
                        return ;
                     }
                  } ) ;
                  if( tempNum == 1 )
                  {
                     $scope.ErrorTip = '请至少修改一个字段值。' ;
                     return ;
                  }

                  var querySql = sprintf( 'SELECT ctid FROM ? WHERE ', addQuotes( SdbSwap.tbName ) ) ;
                  var tempNum = 1 ;
                  $.each( data, function( key, value ){
                     if( tempNum > 1 )
                     {
                        querySql += ' AND ' ;
                     }
                     if( typeof( value ) == 'string' )
                     {
                        querySql += addQuotes( key ) + ' = ' + sqlEscape( value ) ;
                     }
                     else if( value === null )
                     {
                        querySql += addQuotes( key ) + ' IS NULL ' ;
                     }
                     else
                     {
                        querySql += addQuotes( key ) + ' = ' + value ;
                     }
                     ++ tempNum ;
                  } ) ;
                  querySql += ' LIMIT 1' ;
                  queryCtid( querySql, 'update', setSql ) ;

               }
               else
               {
                  var sql = sprintf( 'UPDATE ? SET ', addQuotes( SdbSwap.tbName ) ) ;
                  var tempNum = 1 ;
                  $.each( formValue, function( index, fieldInfo ){
                     if( fieldInfo[2] != data[fieldInfo[0]] )
                     {
                        if( tempNum > 1 )
                        {
                           sql += ', ' ;
                        }
                        sql += addQuotes( fieldInfo[0] ) + ' = ' + sqlEscape( fieldInfo[2] ) ;
                        ++ tempNum ;
                     }
                     else
                     {
                        return ;
                     }
                  } ) ;
                  if( tempNum == 1 )
                  {
                     $scope.ErrorTip = '请至少修改一个字段值。' ;
                     return ;
                  }
                  sql += ' WHERE ' ;
                  var tempNum = 1 ;
                  $.each( data, function( key, value ){
                     if( tempNum > 1 )
                     {
                        sql += ' AND ' ;
                     }
                     if( typeof( value ) == 'string' )
                     {
                        sql += addQuotes( key ) + ' = ' + sqlEscape( value ) ;
                     }
                     else if( value === null )
                     {
                        sql += addQuotes( key ) + ' IS NULL ' ;
                     }
                     else
                     {
                        sql += addQuotes( key ) + ' = ' + value ;
                     }
                     ++ tempNum ;
                  } ) ;
                  SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '更新记录' } ) ;
               }
               $scope.EditWindow['callback']['Close']() ;
            } ) ;
            $scope.EditWindow['callback']['SetIcon']( '' ) ;
            $scope.EditWindow['callback']['SetTitle']( $scope.pAutoLanguage( '编辑记录' ) ) ;
            $scope.EditWindow['callback']['Open']() ;
         }
         if( SdbSwap.dataLengthList[index] == true )
         {
            $scope.Components.Confirm.type = 2 ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '该条数据过大，执行该操作可能会造成浏览器卡顿，是否继续？' ) ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
            $scope.Components.Confirm.ok = function(){
               $scope.Components.Confirm.isShow = false ;
               showWindow( index ) ;
            }
         }
         else
         {
            showWindow( index ) ;
         }
         
      } ) ;

      //删除单条记录弹窗
      SdbSignal.on( 'showRemoveData', function( index ){
         var data = SdbSwap.dataList[index] ;
         var ctid = '' ;
         if( SdbSwap.tbType == 'table' )
         {
            var querySql = sprintf( 'SELECT ctid FROM ? WHERE ', addQuotes( SdbSwap.tbName ) ) ;
            var tempNum = 1 ;
            $.each( data, function( key, value ){
               if( tempNum > 1 )
               {
                  querySql += ' AND ' ;
               }
               if( typeof( value ) == 'string' )
               {
                  querySql += addQuotes( key ) + ' = ' + sqlEscape( value ) ;
               }
               else if( value === null )
               {
                  querySql += addQuotes( key ) + ' IS NULL ' ;
               }
               else
               {
                  querySql += addQuotes( key ) + ' = ' + value ;
               }
               ++ tempNum ;
            } ) ;
            querySql += ' LIMIT 1' ;
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除该记录？' ) ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 1 ;
            $scope.Components.Confirm.title = sprintf( $scope.pAutoLanguage( '是否确定删除该记录？' ) ) ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '如果该表没有主键或唯一索引，该操作可能造成删除多条相同记录。' ) ) ;
         }

         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            if( SdbSwap.tbType == 'table' )
            {
               queryCtid( querySql, 'delete' ) ;
            }
            else
            {
               var sql = sprintf( 'DELETE FROM ? WHERE ', addQuotes( SdbSwap.tbName ) ) ;
               var tempNum = 1 ;
               $.each( data, function( key, value ){
                  if( tempNum > 1 )
                  {
                     sql += ' AND ' ;
                  }
                  if( typeof( value ) == 'string' )
                  {
                     sql += addQuotes( key ) + ' = ' + sqlEscape( value ) ;
                  }
                  else if( value === null )
                  {
                     sql += addQuotes( key ) + ' IS NULL ' ;
                  }
                  else
                  {
                     sql += addQuotes( key ) + ' = ' + value ;
                  }
                  ++ tempNum ;
               } ) ;
               SdbSignal.commit( 'execOperate', { 'sql': sql, 'type': '删除记录' } ) ;
            }

            $scope.Components.Confirm.isShow = false ;
         }
      } ) ;

      //执行记录 弹窗
      $scope.ExecHistoryWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开执行记录 弹窗
      $scope.ShowExecHistory = function(){
         $scope.ExecHistoryWindow['config'] = SdbSwap.execHistory ;
         $scope.ExecHistoryWindow['callback']['SetIcon']( '' ) ;
         $scope.ExecHistoryWindow['callback']['SetTitle']( $scope.pAutoLanguage( '执行记录' ) ) ;
         $scope.ExecHistoryWindow['callback']['SetCloseButton']( $scope.pAutoLanguage( '关闭' ), function(){
            $scope.ExecHistoryWindow['callback']['Close']() ;
         } ) ;
         $scope.ExecHistoryWindow['callback']['Open']() ;
      }

      //选择字段 下拉菜单
      $scope.FieldDropdown = {
         'config': [],
         'callback': {}
      } ;

      //打开 选择字段
      $scope.ShowFieldList = function( event ){
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //获取选择字段下拉菜单的选项
      SdbSignal.on( 'getChooseFieldList', function( fields ){
         $scope.FieldDropdown['config'] = [] ;
         var isFirst = true ;
         var showFieldList = [] ;
         $.each( fields, function( index, field ){
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
      } ) ;

      //保存显示列
      $scope.SaveField = function(){
         showFieldList = [] ;
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            if( fieldInfo['show'] )
            {
               showFieldList.push( fieldInfo['field'] ) ;
            }
         } ) ;
         SdbSignal.commit( 'updateTableKey', showFieldList ) ;
      }

      //执行记录中更新sql输入框
      $scope.UpdateSqlCommand = function( sql ){
         SdbSignal.commit( 'updateSqlCommand', sql ) ;
      }

   } ) ;

   //结果栏
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Data.Result.Ctrl', function( $scope, SdbSignal ){

      SdbSignal.on( 'update_result', function( result ){
         $scope.ExecRc = result['rc'] ;
         $scope.ExecResult = result['result'] ;
      }, false ) ;

   } ) ;

   //sql语句输入框
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Data.InputBox.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      
      $scope.SqlCommand = sprintf( 'SELECT * FROM ?', addQuotes( SdbSwap.tbName ) ) ;
      
      $scope.ExecQuery = function(){
         if( SdbSignal.commit( 'getTableLength' )[0] == 0 )
         {
            SdbSignal.commit( 'exec_sql', { 'sql': $scope.SqlCommand, 'isUser': true, 'isUpdateSql': true } ) ;
         }
         else
         {
            SdbSignal.commit( 'exec_sql', { 'sql': $scope.SqlCommand, 'isUser': true, 'isUpdateSql': true, 'gotoFirst': true } ) ;
         }
      }

      SdbSignal.commit( 'exec_sql', { 'sql': $scope.SqlCommand, 'isUser': true, 'isUpdateSql': true } ) ;

      //修改输入框sql语句
      SdbSignal.on( 'updateSqlCommand', function( sql ){
         $scope.SqlCommand = sql ;
      } ) ;
   } ) ;

   //表格
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Data.Table.Ctrl', function( $scope, SdbFunction, SdbSwap, SdbSignal ){
      
      var isFirst = true ;
      //临时存查询结果，选择字段时使用
      var tempData = [] ;

      //调用跳转第一页时控制参数
      //0: isUpdateSql 和 gotoFirst 为false
      //1: isUpdateSql 为 true
      //2: isUser 为false
      SdbSwap.userQuery ;
      
      //表格
      $scope.GridTable = {
         'title': {
            "#": "#",
            "operate": ""
         },
         'body': [],
         'options': {
            'width': {
               "#": "30px",
               "operate": "50px"
            },
            'sort': {
            },
            'max': 30,
            'filter': {
            },
            'mode': 'adaption'
         },
         'callback': {}
      } ;

      //第一页
      var firstPage = function(){
         SdbSwap.offset = 0 ;
         SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': true } ) ;
      }
      
      //上一页
      var previousPage = function(){
         SdbSwap.offset -= 30 ;
         SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': true } ) ;
      }

      //下一页
      var nextPage = function(){
         SdbSwap.offset += 30 ;
         SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': true } ) ;
      }

      //最后一页
      var lastPage = function(){
         var totalPage = 0 ;
         totalPage = $scope.GridTable['callback']['GetSumPageNum']() ;
         SdbSwap.offset = ( totalPage - 1 ) * 30 ;
         SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': true } ) ;
      }

      //指定某页
      var gotoPage = function( pageNum ){
         SdbSwap.offset = ( pageNum - 1 ) * 30 ;
         if( SdbSwap.userQuery == 1 )
         {
            SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.execSql, 'isUser': true, 'isUpdateSql': true } ) ;
            SdbSwap.userQuery = 0 ;
         }
         else if( SdbSwap.userQuery == 2 )
         {
            SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': false } ) ;
         }
         else
         {
            SdbSignal.commit( 'exec_sql', { 'sql': SdbSwap.lastQuerySql, 'isUser': true } ) ;
         }
         
      }

      SdbSignal.on( 'goto_page', function( userQuery ){
         SdbSwap.userQuery = userQuery ;
         $scope.GridTable['callback']['Jump']( 1 ) ;
      } ) ;

      var initTable = function( callback ){
         callback['SetToolPageButton']( 'first', firstPage ) ;
         callback['SetToolPageButton']( 'previous', previousPage ) ;
         callback['SetToolPageButton']( 'next', nextPage ) ;
         callback['SetToolPageButton']( 'last', lastPage ) ;
         callback['SetToolPageButton']( 'jump', gotoPage ) ;
      }


      //获取表格字段
      SdbSignal.on( 'setTableTitle', function( result ){
         var fieldList = [] ;
         $scope.GridTable['title'] = {
            "#": "#",
            "operate": ""
         } ;
         $.each( result, function( index, record ){
            fieldList = SdbFunction.getJsonKeys( record, 0, fieldList ) ;
         } ) ;
         $.each( fieldList, function( index, field ){
            //默认最多显示10个字段
            if( index < 10 )
            {
               $scope.GridTable['title'][field] = field ;
            }
            else
            {
               $scope.GridTable['title'][field] = false ;
            }
         } ) ;

         //选择字段
         SdbSignal.commit( 'getChooseFieldList', fieldList ) ;
      } ) ;

      //更新表格key
      SdbSignal.on( 'updateTableKey', function( fields ){
         $scope.GridTable['title'] = {
            "#": "#",
            "operate": ""
         } ;
         $.each( fields, function( index, field ){
            $scope.GridTable['title'][field] = field ;
         } ) ;

         $scope.GridTable['body'] = [] ;
         $.each( tempData, function( index, record ){
            var newRecord = SdbFunction.filterJson( record, fields ) ;
            $scope.GridTable['body'].push( newRecord ) ;
         } ) ;
      } ) ;

      //表格内容
      SdbSignal.on( 'setTableData', function( result ){
         $scope.GridTable['body'] = [] ;
         $.each( result, function( index, dataInfo ){
            $scope.GridTable['body'].push( dataInfo ) ;
         } ) ;
         tempData = $.extend( true, [], $scope.GridTable['body'] ) ;
         if( isFirst )
         {
            //初始化表格翻页事件
            initTable( $scope.GridTable['callback'] ) ;
            isFirst = false ;
         }
      } ) ;

      //查看单条记录弹窗
      $scope.DataWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 单条记录 弹窗
      $scope.ShowDataWindow = function( index ){
         var showWindow = function( index ){
            var data = SdbSwap.dataList[index] ;
            $scope.DataWindow['config'] = data ;
            $scope.DataWindow['callback']['SetTitle']( $scope.pAutoLanguage( '记录' ) ) ;
            $scope.DataWindow['callback']['SetCloseButton']( $scope.pAutoLanguage( '关闭' ), function(){
               $scope.DataWindow['callback']['Close']() ;
            } ) ;
            $scope.DataWindow['callback']['Open']() ;
         }

         if( SdbSwap.dataLengthList[index] == true )
         {
            $scope.Components.Confirm.type = 2 ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '该条数据过大，执行该操作可能会造成浏览器卡顿，是否继续？' ) ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
            $scope.Components.Confirm.ok = function(){
               $scope.Components.Confirm.isShow = false ;
               showWindow( index ) ;
            }
         }
         else
         {
            showWindow( index ) ;
         }
         
      }

      //转换null显示
      $scope.ReturnNull = function( value ){
         value = value === null ? 'null' : value ;
         return value ;
      }

      SdbSignal.on( 'getTableLength', function(){
         return $scope.GridTable['body'].length ;
      } ) ;

      $scope.ShowEditWindow = function( index ){
         SdbSignal.commit( 'showEditWindow', index ) ;
      }

      $scope.ShowRemoveData = function( index ){
         SdbSignal.commit( 'showRemoveData', index ) ;
      }
   } ) ;

}());