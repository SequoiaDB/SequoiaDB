//@ sourceURL=Structure.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.SQL.Index.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
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
      var appendOnly = false ;
      var dbUser = moduleInfo['User'] ;
      var dbPwd  = moduleInfo['Pwd'] ;
      var dbName = moduleInfo['DbName'] ;
      var tbName = moduleInfo['TbName'] ;
      $scope.dbHost = moduleInfo['Host'] ;
      $scope.dbPort = moduleInfo['Port'] ;

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName ) ;

      $scope.fullName = dbName + '.' + tbName ;

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

      //添加字段
      var addColumn = function( sql ){
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
               $scope.queryTableStruct() ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, null, function(){
                  addColumn( sql ) ;
                  return true ;
               }, $scope.autoLanguage( '添加字段失败' ), state['result'] ) ;
            }
         } ) ;
      }

      //删除字段
      var dropColumn = function( sql ){
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
               $scope.queryTableStruct() ;
            }
            else if( isEnd == true && state['rc'] == false )
            {
               _IndexPublic.createRetryModel( $scope, null, function(){
                  dropColumn( sql ) ;
                  return true ;
               }, $scope.autoLanguage( '删除字段失败' ), state['result'] ) ;
            }
         } ) ;
      }

      //添加字段弹窗
      $scope.AddColumn = function(){
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '添加字段' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "fields",
                  "webName": $scope.autoLanguage( '字段' ),
                  "required": true,
                  "type": "list",
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "placeholder": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "max": 127,
                              "regex": "^[a-zA-Z]+[0-9a-zA-Z]*$",
                              "regexError": $scope.autoLanguage( "字段名由字母和数字组成，并且以字母起头。" )
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
                           "webName": $scope.autoLanguage( "长度" ),
                           "placeholder": $scope.autoLanguage( "长度" ),
                           "type": "int",
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
                           "type": "string",
                           "valid": appendOnly == true ? {} : { "min": 1 }
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
               var sql = 'ALTER TABLE ' + tbName + ' ' ;
               $.each( value['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += 'ADD ' + fieldInfo['name'] + ' ' + fieldInfo['type'] ;
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
               addColumn( sql ) ;
            }
            return isAllClear ;
         }
      }

      //打开删除字段窗口
      $scope.dropField = function( field ){
         _IndexPublic.createRetryModel( $scope, null, function(){
            var sql = 'ALTER TABLE ' + tbName + ' DROP ' + field ;
            dropColumn( sql ) ;
            return true ;
         }, $scope.autoLanguage( '确定要删除该字段吗？' ), $scope.autoLanguage( '字段名：' ) + field, $scope.autoLanguage( '是的，删除' ) ) ;
      }

      //解出默认值
      var parseDefault = function( modifiers ){
         var defaultIndex = modifiers.indexOf( 'default' ) ;
         if( defaultIndex >= 0 )
         {
            modifiers = modifiers.substr( defaultIndex ) ;
            return modifiers.split( ' ' )[1] ;
         }
         else
         {
            return '' ;
         }
      }

      //解出是否NULL
      var parseNULL = function( modifiers ){
         if( modifiers.indexOf( 'not null' ) >= 0 )
         {
            return $scope.autoLanguage( '否' )
         }
         return $scope.autoLanguage( '是' ) ;
      }

      //解析是否appendonly=true
      var parseAppendOnly = function( attr ){
         if( attr.indexOf( 'appendonly=true' ) >= 0 )
         {
            appendOnly = true ;
         }
         if( attr.indexOf( 'appendonly=false' ) >= 0 )
         {
            appendOnly = false ;
         }
      }

      //查询表结构
      $scope.queryTableStruct = function(){
         var fieldGridData = {
            'title': [
               { 'text': '#' },
               { 'text': $scope.autoLanguage( '字段名' ) },
               { 'text': '' },
               { 'text': $scope.autoLanguage( '类型' ) },
               { 'text': 'NULL' },
               { 'text': $scope.autoLanguage( '默认值' ) }
            ],
            'body': [],
            'tool': {
               'position': 'bottom',
               'left': [
                  { 'text': '' }
               ]
            },
            'options': {
               'grid': { 'tdModel': 'auto', 'tool': true, 'gridModel': 'fixed', titleWidth: [ '60px', 40, '45px', 25, 15, 20 ] } 
            }
         } ;
         var state = { 'status': 0 } ;
         var sql = '\\d+ ' + tbName ;
         sequoiasqlOperate( dbName, dbUser, dbPwd, sql, function( taskInfo, isEnd ){
            for( var i = 0, k = 1; i < taskInfo.length; ++i )
            {
               state = parseSSQL( taskInfo[i]['Value'], state ) ;
               if( state['status'] == 4 )
               {
                  var remove = $compile( '<a ng-click="dropField(\'' + state['value'][0] + '\')"></a>' )( $scope ).addClass( 'linkButton' ).append( $( '<i class="fa fa-remove"></i>' ).attr( 'data-desc', $scope.autoLanguage( '删除' ) ) ) ;
                  fieldList.push( state['value'][0] ) ;
                  fieldGridData['body'].push( [
                     { 'text': k },
                     { 'text': state['value'][0] },
                     { 'html': remove },
                     { 'text': state['value'][1] },
                     { 'text': parseNULL( state['value'][2] ) },
                     { 'text': parseDefault( state['value'][2] ) }
                  ] ) ;
                  ++k ;
               }
               if( state['status'] == 6 )
               {
                  break ;
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
            if( isEnd == true )
            {
               if( typeof( state['attr'] ) != 'undefined' )
               {
                  $.each( state['attr'], function( index, attr ){
                     parseAppendOnly( attr ) ;
                  } ) ;
               }
               fieldGridData['tool']['left'][0]['text'] = $scope.sprintf( $scope.autoLanguage( '一共 ? 个字段' ), fieldGridData['body'].length ) ;
               $scope.fieldGridData = $.extend( true, {}, fieldGridData ) ;
               $scope.$apply() ;
            }
         } ) ;
      }
      $scope.queryTableStruct() ;
   } ) ;
}());