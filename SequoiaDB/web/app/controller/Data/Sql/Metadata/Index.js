//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.SQL.Metadata.Ctrl', function( $scope, $compile, $location, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql' || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }
      
      //初始化
      var dbName    = '' ;
      var dbUser    = '' ;
      var dbPwd     = '' ;
      $scope.dbHost = '' ;
      $scope.dbPort = '' ;
      $scope.moduleName = moduleName ;
      $scope.databaseIndex = 0 ;
      $scope.databaseList = [] ;
      $scope.tableList = [] ;
      $scope.tablelength = 0 ;
      $scope.tableSum = 0 ;

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

      //获取database列表
      var queryDbList = function( needQueryTable ){
         $scope.databaseList = [] ;
         var state = { 'status': 0 } ;
         var sql = '\\l+' ;
         var isFindDb = false ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 4 )
               {
                  $scope.databaseList.push( { 'Name': state['value'][0] } ) ;
                  if( dbName == state['value'][0] )
                  {
                     $scope.databaseIndex = $scope.databaseList.length - 1 ;
                     if( needQueryTable )
                     {
                        queryTable() ;
                     }
                     isFindDb = true ;
                  }
               }
            }
            if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, null, function(){
                  queryDbList( needQueryTable ) ;
                  return true ;
               }, $scope.autoLanguage( '获取数据库列表失败' ), state['result'] ) ;
            }
            else if( isEnd == true && isFindDb == false && $scope.databaseList.length > 0 )
            {
               dbName = $scope.databaseList[0]['Name'] ;
               if( needQueryTable )
               {
                  queryTable() ;
               }
            }
            $scope.$apply() ;
         } ) ;
      }

      //获取table信息
      var queryTable = function(){
         $scope.tableList = [] ;
         //构造表格
         var gridData = {
            'title': [ 
               { "text": '#' },
               { "text": $scope.autoLanguage( '表名' ) } ,
               { "text": ''},
               { "text": $scope.autoLanguage( '类型' ) } ,
               { "text": $scope.autoLanguage( '数据库模式' ) } ,
               { "text": $scope.autoLanguage( '用户' ) } ,
               { "text": $scope.autoLanguage( '描述' ) }
            ],
            'body': [],
            'tool': {
               'position': 'bottom',
               'left': [ { 'text': '' } ],
               'right': [ ]
            },
            'options': {
               'grid': {  'tdModel': 'fixed', 'gridModel': 'fixed', 'tdHeight': '19px', 'titleWidth': [ 5, 19, '45px', 17, 17, 17, 17] }
            }
         } ;
         var state = { 'status': 0 } ;
         var sql = '\\d+' ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var tableName = $compile( '<a class="linkButton"></a>')( $scope ) ;
            var length = taskInfo.length ;
            for( var i = 0, index = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 4 )
               {  
                  var tmpEdit = $compile( '<a ng-click="gotoStructure(' + index + ')"></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-edit"></i>' ).attr( 'data-desc', $scope.autoLanguage( '编辑' ) ) ) ;
                  var tmpRemove = $compile( '<a ng-click="deleteTable(' + index + ')"></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-remove"></i>' ).attr( 'data-desc', $scope.autoLanguage( '删除' ) ) ) ;
                  $scope.tableList.push( { 'Name': state['value'][1] } ) ;
                  gridData['body'].push( [
                     { 'text': index + 1 },
                     { 'html': $compile( tableName.clone().attr( 'ng-click', 'gotoData(' + index + ')' ).text( state['value'][1] ) )( $scope )  },
                     { 'html': $compile( '<span></span>' )( $scope ).append( tmpEdit ).append( '&nbsp;' ).append( tmpRemove ) },
                     { 'text': state['value'][2] },
                     { 'text': state['value'][0] },
                     { 'text': state['value'][3] },
                     { 'text': state['value'][4] }

                  ] ) ;
                  ++index ;
               }
               if( state['status'] == 7 )
               {
                  break ;
               }
            }
            $scope.GridData = gridData ;
            if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, null, function(){
                  queryTable() ;
                  return true ;
               }, null, state['result'] ) ;
            }
            if( isEnd == true )
            {
               gridData['tool']['left'][0]['text'] = $scope.sprintf( $scope.autoLanguage( '一共 ? 个表' ), gridData['body'].length ) ;
            }
            $scope.$apply() ;
         } ) ;
      }

      //切换数据库
      $scope.showTableInfo = function( dbIndex ){
         $scope.databaseIndex = dbIndex ;
         dbName = $scope.databaseList[ $scope.databaseIndex ]['Name'] ;
         queryTable() ;
      } ;

      //创建数据库
      var createDatabase = function( newDBName ){
         var state = { 'status': 0 } ;
         var sql = 'create database ' + newDBName ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 7 )
               {
                  break ;
               }
            }
            if( isEnd == true && state['rc'] == true )
            {
               queryDbList( false ) ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createDatabase( newDBName ) ;
                  return true ;
               }, null, state['result'] ) ;
            }
         } ) ;
      }

      //删除数据库
      var dropDatabase = function( dropDBName ){
         if( $scope.databaseList.length == 0 )
         {
            return ;
         }
         var state = { 'status': 0 } ;
         var sql = 'drop database ' + dropDBName ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 7 )
               {
                  break ;
               }
            }
            if( isEnd == true && state['rc'] == true )
            {
               queryDbList( false ) ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  dropDatabase( dropDBName ) ;
                  return true ;
               }, $scope.autoLanguage( '删除数据库失败' ), state['result'] ) ;
            }
         } ) ;
      }

      //创建数据表
      var createTable = function( sql, index ){
         if( $scope.databaseList.length == 0 )
         {
            return ;
         }
         var state = { 'status': 0 } ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 7 )
               {
                  break ;
               }
            }
            if( isEnd == true && state['rc'] == true )
            {
               $scope.showTableInfo( index ) ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createTable( sql, index ) ;
                  return true ;
               }, $scope.autoLanguage( '创建数据表失败' ), state['result'] ) ;
            }
         } ) ;
      }

      //删除数据表
      var dropTable = function( dropTBName ){
         var sql = 'drop table ' + dropTBName ;
         var state = { 'status': 0 } ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            var length = taskInfo.length ;
            for( var i = 0; i < length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 7 )
               {
                  break ;
               }
            }
            if( isEnd == true && state['rc'] == true )
            {
               queryTable() ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  dropTable( dropTBName ) ;
                  return true ;
               }, $scope.autoLanguage( '删除数据表失败' ), state['result'] ) ;
            }
         } ) ;
      }

      //打开创建数据库窗口
      $scope.showCreateDatabase = function(){
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建数据库' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "DBName",
                  "webName": $scope.autoLanguage( '数据库名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 32,
                     "regex": "^[a-zA-Z]+[0-9a-zA-Z]*$",
                     "regexError": $scope.autoLanguage( '数据库名由字母和数字组成，并且以字母起头。' )
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               createDatabase( value['DBName'] ) ;
            }
            return isAllClear ;
         }
      } 

      //打开删除数据库窗口
      $scope.showRemoveDatabase = function(){
         var databaseIndex = $scope.databaseIndex ;
         var databaseList = $scope.databaseList ;
         var listValue = [] ;
         var defaultDb = null ;
         $.each( databaseList, function( index, databaseName ){
            if( databaseIndex == index )
            {
               return true ;
            }
            if( defaultDb == null )
            {
               defaultDb = databaseName["Name"] ;
            }
            listValue.push( { 'key': databaseName["Name"], 'value': databaseName["Name"] } ) ;
         } ) ;
         $scope.Components.Modal.icon = 'fa-remove' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '删除数据库' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "DBName",
                  "webName": $scope.autoLanguage( '数据库名' ),
                  "type": "select",
                  "required": true,
                  "value": defaultDb,
                  "valid": listValue
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               dropDatabase( value['DBName'] ) ;
            }
            return isAllClear ;
         }
      } ;

      //打开创建数据表窗口
      $scope.showCreateTable = function(){
         var databaseIndex = $scope.databaseIndex ;
         var databaseList = $scope.databaseList ;
         var listValue = [] ;
         $.each( databaseList, function( index, databaseName ){
            listValue.push( { 'key': databaseName["Name"], 'value': index } ) ;
         } ) ;
         $scope.Components.Modal.icon = 'fa-edit' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建数据表' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "dbName",
                  "webName": $scope.autoLanguage( '数据库' ),
                  "type": "select",
                  "required": true,
                  "value": databaseIndex,
                  "valid": listValue
               },
               {
                  "name": "tbName",
                  "webName": $scope.autoLanguage( '数据表名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "regex": "^[a-zA-Z]+[0-9a-zA-Z]*$",
                     "regexError": $scope.autoLanguage( '数据表名由字母和数字组成，并且以字母起头。' )
                  }
               },
               {
                  "name": "fields",
                  "webName":  $scope.autoLanguage( '字段结构' ),
                  "required": true,
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName":  $scope.autoLanguage( '字段' ),
                           "type": "string",
                           "placeholder": $scope.autoLanguage( "字段名" ),
                           "value": "",
                           "valid": {
                              "min": 1,
                              "max": 127,
                              "regex": "^[a-zA-Z]+[0-9a-zA-Z]*$",
                              "regexError": $scope.autoLanguage( '字段名由字母和数字组成，并且以字母起头。' )
                           }
                        },
                        {
                           "name": "type",
                           "type": "select",
                           "value": "integer",
                           "valid": [
                              { "key": 'smallint', "value": "smallint" },
                              { "key": 'integer', "value": "integer" },
                              { "key": 'bigint', "value": "bigint" },
                              { "key": 'decimal', "value": "decimal" },
                              { "key": 'numeric', "value": "numeric" },
                              { "key": 'real', "value": "real" },
                              { "key": 'double precision', "value": "double precision" },
                              { "key": 'serial', "value": "serial" },
                              { "key": 'bigserial', "value": "bigserial" },
                              { "key": 'money', "value": "money" },
                              { "key": 'varchar', "value": "varchar" },
                              { "key": 'character', "value": "character" },
                              { "key": 'char', "value": "char" },
                              { "key": 'text', "value": "text" },
                              { "key": 'timestamp', "value": "timestamp" },
                              { "key": 'interval', "value": "interval" },
                              { "key": 'date', "value": "date" },
                              { "key": 'time', "value": "time" },
                              { "key": 'boolean', "value": "boolean" }
                           ]
                        },
                        {
                           "name": "length",
                           "type": "int",
                           "webName": $scope.autoLanguage( '长度' ),
                           "placeholder": $scope.autoLanguage( "长度" ),
                           "value": "",
                           "valid": {
                              "min": 0,
                              "empty": true
                           }
                        },
                        {
                           "name": "default",
                           "webName": $scope.autoLanguage( "默认值" ),
                           "placeholder": $scope.autoLanguage( "默认值" ),
                           "type": "string"
                        },
                        {
                           "name": "null",
                           "webName": $scope.autoLanguage( "空" ),
                           "type": "checkbox",
                           "value": true
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               dbName = databaseList[value['dbName']]['Name'] ;
               var sql = 'CREATE TABLE ' + value['tbName'] + ' ( ' ;
               $.each( value['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += fieldInfo['name'] + ' ' + fieldInfo['type'] ;
                  if( isNaN( fieldInfo['length'] ) == false )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'varchar':
                     case 'character':
                     case 'char':
                     case 'decimal':
                     case 'numeric':
                        subSql += '(' + fieldInfo['length'] + ') ' ;
                        break ;
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  else
                  {
                     subSql += ' ' ;
                  }
                  if( fieldInfo['null'] == true )
                  {
                     subSql += 'NULL ' ;
                  }
                  else
                  {
                     subSql += 'NOT NULL ' ;
                  }
                  if( typeof( fieldInfo['default'] ) == 'string' )
                  {
                     subSql += 'DEFAULT ' + sqlEscape( fieldInfo['default'] ) + ' ' ;
                  }
                  sql += subSql ;
               } ) ;
               sql += ' )' ;
               createTable( sql, value['dbName'] ) ;
            }
            return isAllClear ;
         } ;
      } ;

      //打开删除表窗口
      $scope.deleteTable = function( index ){
         var databaseIndex = $scope.databaseIndex ;
         var databaseName = $scope.databaseList[databaseIndex]["Name"] ;
         var tableName = $scope.tableList[index]['Name'] ;
         _IndexPublic.createRetryModel( $scope, null, function(){
            dropTable( tableName ) ;
            return true ;
         }, $scope.autoLanguage( '确定要删除该表吗？' ), $scope.autoLanguage( '数据表名' ) + ':' + tableName, $scope.autoLanguage( '是的，删除' ) ) ;
      }

      //进入数据操作页面
      $scope.gotoData = function( tableIndex ){
         var databaseIndex = $scope.databaseIndex ;
         var dbName = $scope.databaseList[databaseIndex]["Name"] ;
         var tbName = $scope.tableList[tableIndex]['Name'] ;
         var moduleInfo = { 'Host': $scope.dbHost, 'Port': $scope.dbPort, 'User': dbUser, 'Pwd': dbPwd, 'DbName': dbName, 'TbName': tbName } ;
         SdbFunction.LocalData( 'SdbModuleInfo', JSON.stringify( moduleInfo ) ) ;
         $location.path( '/Data/SQL-Operate/Data' ).search( { 'r': new Date().getTime() } ) ;
      }

      //进入表结构页面
      $scope.gotoStructure = function( tableIndex ){
         var databaseIndex = $scope.databaseIndex ;
         var dbName = $scope.databaseList[databaseIndex]["Name"] ;
         var tbName = $scope.tableList[tableIndex]['Name'] ;
         var moduleInfo = { 'Host': $scope.dbHost, 'Port': $scope.dbPort, 'User': dbUser, 'Pwd': dbPwd, 'DbName': dbName, 'TbName': tbName } ;
         SdbFunction.LocalData( 'SdbModuleInfo', JSON.stringify( moduleInfo ) ) ;
         $location.path( '/Data/SQL-Operate/Structure' ).search( { 'r': new Date().getTime() } ) ;
      }

      //查询业务信息
      var data = { 'cmd': 'query business', 'filter': JSON.stringify( { 'ClusterName': clusterName, 'BusinessName': moduleName } ) } ;
      SdbRest.OmOperation( data, {
         'success': function( moduleInfo ){
            if( moduleInfo.length > 0 )
            {
               dbName = moduleInfo[0]['BusinessInfo']['DbName'] ;
               dbUser = moduleInfo[0]['BusinessInfo']['User'] ;
               dbPwd  = moduleInfo[0]['BusinessInfo']['Passwd'] ;
               $scope.dbHost = moduleInfo[0]['BusinessInfo']['HostName'] ;
               $scope.dbPort = moduleInfo[0]['BusinessInfo']['ServiceName'] ;
               queryDbList( true ) ;
            }
            else
            {
               $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
            }
         },
         'failed': function( errorInfo, retryFun ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               retryFun() ;
               return true ;
            } ) ;
         }
      } ) ;

   } ) ;
}());