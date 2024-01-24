//@ sourceURL=Structure.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.MySQL.Structure.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest, SdbSwap, SdbSignal ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      SdbSwap.dbName = SdbFunction.LocalData( 'MysqlDbName' ) ;
      SdbSwap.tbName = SdbFunction.LocalData( 'MysqlTbName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-mysql' || $scope.ModuleName == null || SdbSwap.tbName == null || SdbSwap.dbName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }
      $scope.FullName   = SdbSwap.dbName + '.' + SdbSwap.tbName ;
      SdbSwap.tbType    = SdbFunction.LocalData( 'MysqlTbType' ) ;
      $scope.PrimaryKey = false ;
      SdbSwap.indexList = [] ;

      //获取字段信息
      $scope.QueryTableStruct = function(){
         var sql = sprintf( "SELECT TABLE_NAME,COLUMN_NAME,COLUMN_DEFAULT,DATA_TYPE,COLUMN_TYPE,CHARACTER_MAXIMUM_LENGTH,IS_NULLABLE,COLUMN_COMMENT FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = '?' AND TABLE_SCHEMA = '?' ORDER BY ORDINAL_POSITION", SdbSwap.tbName, SdbSwap.dbName ) ;

         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName, 'Type': 'mysql', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( fieldList ){
               SdbSignal.commit( 'setTableData', fieldList ) ;
               
               SdbSignal.commit( 'setPrimarySelect', fieldList ) ;

               //查询索引
               var sql = sprintf( "select * from information_schema.STATISTICS where TABLE_SCHEMA = '?' and TABLE_NAME = '?'", SdbSwap.dbName, SdbSwap.tbName ) ;
               execSql( sql, true ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.QueryTableStruct() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //执行sql
      var execSql = function( sql, isQueryIndex ){
         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName, 'Type': 'mysql' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               //是否重新查询字段列表
               if( isQueryIndex )
               {
                  $scope.PrimaryKey = false ;
                  SdbSwap.indexList = result ;
                  if( result.length > 0 )
                  {
                     $.each( result, function( index, info ){
                        if( info['INDEX_NAME'] == 'PRIMARY' )
                        {
                           $scope.PrimaryKey = true ;
                        }
                     } ) ;
                  }
               }
               else
               {
                  $scope.QueryTableStruct() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  execSql( sql ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //添加字段 弹窗
      $scope.AddFieldWindow = {
         'config': {},
         'callback': {}
      }
      var appendOnly = false ;

      //打开 添加字段 弹窗
      $scope.ShowAddField = function(){
         $scope.AddFieldWindow['config'] = {
            'inputList': [
               {
                  "name": "fields",
                  "webName": $scope.pAutoLanguage( '字段' ),
                  "required": true,
                  "type": "list",
                  "desc": $scope.pAutoLanguage( '如字段类型是set或enum时，请在“长度/值”的输入框填写枚举的值，用半角逗号(,)隔开。' ),
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName": $scope.pAutoLanguage( "字段名" ),
                           "placeholder": $scope.pAutoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "max": 63,
                              "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                              "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '字段名' ) )
                           }
                        },
                        {
                           "name": "type",
                           "type": "select",
                           "value": "int",
                           "valid": [
                              { "key": 'tinyint', "value": "tinyint" },
                              { "key": 'smallint', "value": "smallint" },
                              { "key": 'mediumint', "value": "mediumint" },
                              { "key": 'int', "value": "int" },
                              { "key": 'bigint', "value": "bigint" },
                              { "key": 'float', "value": "float" },
                              { "key": 'double', "value": "double" },
                              { "key": 'decimal', "value": "decimal" },
                              { "key": 'bit', "value": "bit" },
                              { "key": 'date', "value": "date" },
                              { "key": 'datetime', "value": "datetime" },
                              { "key": 'timestamp', "value": "timestamp" },
                              { "key": 'year', "value": "year" },
                              { "key": 'time', "value": "time" },
                              { "key": 'char', "value": "char" },
                              { "key": 'varchar', "value": "varchar" },
                              { "key": 'text', "value": "text" },
                              { "key": 'tinytext', "value": "tinytext" },
                              { "key": 'mediumtext', "value": "mediumtext" },
                              { "key": 'longtext', "value": "longtext" },
                              { "key": 'binary', "value": "binary" },
                              { "key": 'blob', "value": "blob" },
                              { "key": 'tinyblob', "value": "tinyblob" },
                              { "key": 'mediumblob', "value": "mediumblob" },
                              { "key": 'longblob', "value": "longblob" },
                              { "key": 'json', "value": "json" },
                              { "key": 'set', "value": "set" },
                              { "key": 'enum', "value": "enum" }
                           ]
                        },
                        {
                           "name": "length",
                           "webName": $scope.pAutoLanguage( "长度/值" ),
                           "placeholder": $scope.pAutoLanguage( "长度/值" ),
                           "type": "string",
                           "value": ""
                        },
                        {
                           "name": "default",
                           "webName": $scope.pAutoLanguage( "默认值" ),
                           "placeholder": $scope.pAutoLanguage( "默认值" ),
                           "type": "string",
                           "valid": ""
                        },
                        {
                           "name": "attr",
                           "type": "select",
                           "value": "",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "无属性" ), "value": "" },
                              { "key": $scope.pAutoLanguage( "无符号" ), "value": "unsigned" },
                              { "key": $scope.pAutoLanguage( "零填充" ), "value": "zerofill" }
                           ]
                        },
                        {
                           "name": "null",
                           "webName": $scope.pAutoLanguage( "空" ),
                           "type": "checkbox",
                           "value": true
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.AddFieldWindow['callback']['SetOkButton']( $scope.pAutoLanguage('确定'), function(){
            var isClear = $scope.AddFieldWindow['config'].check( function( formVal ){
               var rv = [] ;
               $.each( formVal['fields'], function( index, info ){
                  if( info['type'] == 'enum' || info['type'] == 'set' )
                  {
                     if( info['length'].length == 0 )
                     {
                        rv.push( { 'name': 'fields', 'error': $scope.pAutoLanguage( '字段参数错误。' ) } ) ;
                     }
                  }
                  else
                  {
                     if( ( info['length'].length > 0 &&  !( parseInt( info['length'] ) > 0 ) ) || ( info['length'] < 0 || info['length'] > 4294967295 ) )
                     {
                        rv.push( { 'name': 'fields', 'error': $scope.pAutoLanguage( '字段参数错误。' ) } ) ;
                     }
                  }
               } ) ;
               return rv ;
            } ) ;
            if( isClear )
            {
               var value = $scope.AddFieldWindow['config'].getValue() ;
               var sql = sprintf( 'ALTER TABLE `?` ', SdbSwap.tbName ) ;
               $.each( value['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += 'ADD `' + fieldInfo['name'] + '` ' + fieldInfo['type'] ;
                  if( fieldInfo['length'].length > 0 )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'varchar':
                     case 'character':
                     case 'char':
                     case 'decimal':
                     case 'numeric':
                     case 'int':
                     case 'bit':
                     case 'bigint':
                     case 'tinyint':
                     case 'smallint':
                     case 'mediumint':
                     case 'datetime':
                     case 'timestamp':
                     case 'time':
                     case 'binary':
                     case 'double':
                     case 'float':
                        subSql += '(' + fieldInfo['length'] + ') ' ;
                        break ;
                     case 'set':
                     case 'enum':
                        if( fieldInfo['length'].indexOf( ',' ) >= 0 )
                        {
                           var tmpArray = fieldInfo['length'].split( ',' ) ;
                           var first = true ;
                           subSql += '(' ;
                           $.each( tmpArray, function( index, value ){
                              if( !first )
                              {
                                 subSql += ',' ;
                              }
                              subSql = subSql + "'" + value + "'" ;
                              first = false ;
                           } ) ;
                           subSql += ')' ;
                        }
                        else
                        {
                           subSql += '(\'' + fieldInfo['length'] + '\')' ;
                        }
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  else
                  {
                     subSql += ' ' ;
                  }
                  if( !isEmpty( fieldInfo['attr'] ) )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'tinyint':
                     case 'smallint':
                     case 'mediumint':
                     case 'int':
                     case 'double':
                     case 'bigint':
                     case 'decimal':
                     case 'float':
                        subSql += ' ' + fieldInfo['attr'] + ' ' ;
                        break ;
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  if( fieldInfo['null'] == true )
                  {
                     subSql += 'NULL ' ;
                  }
                  else
                  {
                     subSql += 'NOT NULL ' ;
                  }
                  if( isString( fieldInfo['default'] ) )
                  {
                     if( fieldInfo['null'] == true && fieldInfo['default'].toLowerCase() === 'null' )
                     {
                        subSql += 'DEFAULT ' + fieldInfo['default'] + ' ' ;
                     }
                     else
                     {
                        subSql += 'DEFAULT ' + sqlEscape( fieldInfo['default'] ) + ' ' ;
                     }
                  }
                  sql += subSql ;
               } ) ;
               execSql( sql ) ;
               $scope.AddFieldWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.AddFieldWindow['callback']['SetIcon']( '' ) ;
         $scope.AddFieldWindow['callback']['SetTitle']( $scope.pAutoLanguage( '添加字段' ) ) ;
         $scope.AddFieldWindow['callback']['Open']() ;
      } ;

      $scope.FieldsSelect = [] ;

      //设置主键 弹窗
      $scope.SetPrimaryWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 设置主键 弹窗
      var showSetPrimary = function(){
         if( !$scope.PrimaryKey )
         {
            $scope.SetPrimaryWindow['config'] = {
               'inputList': [
                  {
                     "name": "fields",
                     "webName": $scope.pAutoLanguage( '字段' ),
                     "required": true,
                     "type": "list",
                     "child":[
                        [
                           {
                              "name": "field",
                              "type": "select",
                              "value": $scope.FieldsSelect[0]['key'],
                              "default": $scope.FieldsSelect[0]['key'],
                              "valid": $scope.FieldsSelect
                           }
                        ]
                     ]
                  }
               ]
            } ;
            $scope.SetPrimaryWindow['callback']['SetOkButton']( $scope.pAutoLanguage('确定'), function(){
               var sql = sprintf( 'ALTER TABLE `?` ADD PRIMARY KEY ', SdbSwap.tbName ) ;
               var formValue = $scope.SetPrimaryWindow['config'].getValue() ;
               var isFrist = true ;
               var existList = {} ;
               $.each( formValue['fields'], function( index, field ){
                  if( isUndefined( existList[field['field']] ) )
                  {
                     existList[field['field']] = true ;
                     if( isFrist )
                     {
                        sql += '(' ;
                        isFrist = false ;
                     }
                     else
                     {
                        if( existList[field['field']] )
                        sql += ',' ;
                     }
                     sql += '`' + field['field'] + '`' ;
                  }
               } ) ;
               sql += ')' ;
               execSql( sql ) ;
               $scope.SetPrimaryWindow['callback']['Close']() ;
            } ) ;
            $scope.SetPrimaryWindow['callback']['SetIcon']( '' ) ;
            $scope.SetPrimaryWindow['callback']['SetTitle']( $scope.pAutoLanguage( '设置主键' ) ) ;
            $scope.SetPrimaryWindow['callback']['Open']() ;
         }
      }

      //移除主键
      var showDropPrimary = function(){
         if( $scope.PrimaryKey )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除主键？' ) ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
            $scope.Components.Confirm.ok = function(){
               var sql = sprintf( 'alter table `?` drop primary key', SdbSwap.tbName ) ;
               execSql( sql ) ;
               $scope.Components.Confirm.isShow = false ;
            }
         }
      }

      //创建索引 弹窗
      $scope.CreateIndexWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 创建索引
      var showCreateIndex = function(){
         $scope.CreateIndexWindow['config'] = {
            'inputList': [
               {
                  "name": "fields",
                  "webName": $scope.pAutoLanguage( '字段' ),
                  "required": true,
                  "type": "list",
                  "child":[
                     [
                        {
                           "name": "field",
                           "type": "select",
                           "value": $scope.FieldsSelect[0]['key'],
                           "default": $scope.FieldsSelect[0]['key'],
                           "valid": $scope.FieldsSelect
                        },
                        {
                           "name": "length",
                           "webName": $scope.pAutoLanguage( '长度' ),
                           "placeholder": $scope.pAutoLanguage( "长度" ),
                           "type": "int",
                           "value": '',
                           "valid": {
                             'empty': true
                           }
                        }
                     ]
                  ]
               },
               {
                  "name": "type",
                  "webName": $scope.pAutoLanguage( '索引类型' ),
                  "required": true,
                  "type": "select",
                  "value": "normal",
                  "valid": [
                     { 'key': $scope.pAutoLanguage( '普通索引' ), 'value': 'normal' },
                     { 'key': $scope.pAutoLanguage( '唯一索引' ), 'value': 'unique' }
                  ]
               }
            ]
         }
         $scope.CreateIndexWindow['callback']['SetTitle']( $scope.pAutoLanguage( '创建索引' ) ) ;
         $scope.CreateIndexWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.CreateIndexWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.CreateIndexWindow['config'].check() ;
            if( isClear )
            {
               var sql = '' ;
               var formValue = $scope.CreateIndexWindow['config'].getValue() ;
               if( formValue['type'] == 'normal' )
               {
                  sql = sprintf( 'alter table `?` add index ', SdbSwap.tbName ) ; 
               }
               else
               {
                  sql = sprintf( 'alter table `?` add unique ', SdbSwap.tbName ) ; 
               }
               var isFrist = true ;
               var existList = {} ;
               $.each( formValue['fields'], function( index, field ){
                  if( isUndefined( existList[field['field']] ) )
                  {
                     existList[field['field']] = true ;
                     if( isFrist )
                     {
                        sql += '(' ;
                        isFrist = false ;
                     }
                     else
                     {
                        if( existList[field['field']] )
                        sql += ',' ;
                     }
                     sql += '`' + field['field'] + '`' ;
                     if( field['length'] > 0 )
                     {
                        sql += '(' + field['length'] + ')' ;
                     }
                  }
               } ) ;
               sql += ')' ;

               execSql( sql ) ;
               $scope.CreateIndexWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.CreateIndexWindow['callback']['Open']() ;
      }
      
      //删除索引 弹窗
      $scope.DropIndexWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除索引
      var showDropIndex = function(){
         if( SdbSwap.indexList.length > 0 )
         {
            var selectList = [] ;
            $.each( SdbSwap.indexList, function( index, info ){
               selectList.push( { 'key': info['INDEX_NAME'], 'value': info['INDEX_NAME'] } ) ;
            } ) ;
            $scope.DropIndexWindow['config'] = {
               'inputList': [
                  {
                     "name": "index",
                     "webName": $scope.pAutoLanguage( '索引名' ),
                     "required": true,
                     "type": "select",
                     "value": selectList[0]['value'],
                     "valid": selectList
                  }
               ]
            } ;
            $scope.DropIndexWindow['callback']['SetTitle']( $scope.pAutoLanguage( '索引信息' ) ) ;
            $scope.DropIndexWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var formValue = $scope.DropIndexWindow['config'].getValue() ;
               var sql = '' ;
               if( formValue['index'] == 'PRIMARY' )
               {
                  sql = sprintf( 'alter table `?` drop primary key', SdbSwap.tbName ) ;
               }
               else
               {
                  sql = sprintf( 'alter table `?` drop index `?`', SdbSwap.tbName, formValue['index'] ) ;
               }
               execSql( sql ) ;
               $scope.DropIndexWindow['callback']['Close']() ;
            } ) ;
            $scope.DropIndexWindow['callback']['Open']() ;
         }
      }

      //索引操作 下拉菜单
      $scope.IndexDropdown = {
         'config': [],
         'callback': {}
      } ;

      //打开 索引操作
      $scope.ShowIndexDropdown = function( event ){
         $scope.IndexDropdown['config'] = [
            { 'key': $scope.pAutoLanguage( '创建索引' ) },
            { 'key': $scope.pAutoLanguage( '删除索引' ), 'disabled': SdbSwap.indexList.length>0 ? false : true },
            { 'key': $scope.pAutoLanguage( '设置主键' ), 'disabled': $scope.PrimaryKey },
            { 'key': $scope.pAutoLanguage( '移除主键' ), 'disabled': !$scope.PrimaryKey }
         ] ;
         $scope.IndexDropdown['OnClick'] = function( index ){
            if( index == 0 )
            {
               showCreateIndex() ;
            }
            else if( index == 1 )
            {
               showDropIndex() ;
            }
            else if( index == 2 )
            {
               showSetPrimary() ;
            }
            else
            {
               showDropPrimary() ;
            }

            $scope.IndexDropdown['callback']['Close']() ;
         }
         $scope.IndexDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //索引信息 弹窗
      $scope.IndexListWindow = {
         'config': [],
         'callback': {}
      } ;

      //打开 索引信息
      $scope.ShowIndexList = function(){
         $scope.IndexListWindow['config'] = SdbSwap.indexList ;
         $scope.IndexListWindow['callback']['SetTitle']( $scope.pAutoLanguage( '索引信息' ) ) ;
         $scope.IndexListWindow['callback']['SetCloseButton']( $scope.pAutoLanguage( '关闭' ), function(){
            $scope.IndexListWindow['callback']['Close']() ;
         } ) ;
         $scope.IndexListWindow['callback']['Open']() ;
      }

      //删除字段 弹窗
      $scope.DropFieldWindow = {
         'config': {},
         'callback': {}
      } ;

      SdbSignal.on( 'setPrimarySelect', function( fieldList ){
         $scope.FieldsSelect = [] ;
         if( fieldList.length > 0 )
         {
            $.each( fieldList, function( index, field ){
               $scope.FieldsSelect.push( { 'key': field['COLUMN_NAME'], 'value': field['COLUMN_NAME'] } ) ;
            } ) ;
         }
      } ) ;

      //打开 删除字段 弹窗
      SdbSignal.on( 'ShowDropFieldWindow', function( fieldName ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除字段：?？' ), fieldName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            var sql = sprintf( 'alter table `?` drop column `?`', SdbSwap.tbName, fieldName ) ;
            execSql( sql ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      } ) ;

      //设置字段默认值 弹窗
      $scope.SetfFieldDefaultWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 设置字段默认值 弹窗
      var showSetDefault = function( fieldName ){
         $scope.SetfFieldDefaultWindow['config'] = {
            'inputList': [
               {
                  "name": "fieldName",
                  "webName": $scope.pAutoLanguage( '字段名' ),
                  "type": "string",
                  "required": true,
                  "disabled": true,
                  "value": fieldName
               },
               {
                  "name": "default",
                  "webName": $scope.pAutoLanguage( '默认值' ),
                  "type": "string",
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               }
            ]
         }
         $scope.SetfFieldDefaultWindow['callback']['SetTitle']( $scope.pAutoLanguage( '设置默认值' ) ) ;
         $scope.SetfFieldDefaultWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.SetfFieldDefaultWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.SetfFieldDefaultWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.SetfFieldDefaultWindow['config'].getValue() ;
               var sql = sprintf( 'alter table `?` alter `?` set default ?', SdbSwap.tbName, fieldName, sqlEscape( formVal['default'] ) ) ;
               execSql( sql ) ;
               $scope.SetfFieldDefaultWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.SetfFieldDefaultWindow['callback']['Open']() ;
      }

      //打开 删除字段默认值 弹窗
      var shwoRemoveDefault = function( fieldName ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除字段 ? 的默认值？' ), fieldName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            var sql = sprintf( 'alter table `?` alter `?` drop default', SdbSwap.tbName, fieldName ) ;
            execSql( sql ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      //修改字段 弹窗
      $scope.EditFieldWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改字段 弹窗
      var showEditField= function( fieldName, type, length, columnType, comment ){
         if( length === null || type == 'set' || type == 'enum'  )
         {
            length = '' ;
         }
         else
         {
            length = length.toString() ;
         }
         if ( isEmpty( length ) )
         {
            length = getParenthesesStr( columnType ) ;
         }
         $scope.EditFieldWindow['config'] = {
            'inputList': [
               {
                  "name": "fieldName",
                  "webName": $scope.pAutoLanguage( '字段名' ),
                  "type": "string",
                  "required": true,
                  "value": fieldName
               },
               {
                  "name": "newType",
                  "webName": $scope.pAutoLanguage( '字段类型' ),
                  "type": "select",
                  "value": type,
                  "valid": [
                     { "key": 'tinyint', "value": "tinyint" },
                     { "key": 'smallint', "value": "smallint" },
                     { "key": 'mediumint', "value": "mediumint" },
                     { "key": 'int', "value": "int" },
                     { "key": 'bigint', "value": "bigint" },
                     { "key": 'float', "value": "float" },
                     { "key": 'double', "value": "double" },
                     { "key": 'decimal', "value": "decimal" },
                     { "key": 'bit', "value": "bit" },
                     { "key": 'date', "value": "date" },
                     { "key": 'datetime', "value": "datetime" },
                     { "key": 'timestamp', "value": "timestamp" },
                     { "key": 'year', "value": "year" },
                     { "key": 'time', "value": "time" },
                     { "key": 'char', "value": "char" },
                     { "key": 'varchar', "value": "varchar" },
                     { "key": 'text', "value": "text" },
                     { "key": 'tinytext', "value": "tinytext" },
                     { "key": 'mediumtext', "value": "mediumtext" },
                     { "key": 'longtext', "value": "longtext" },
                     { "key": 'binary', "value": "binary" },
                     { "key": 'blob', "value": "blob" },
                     { "key": 'tinyblob', "value": "tinyblob" },
                     { "key": 'mediumblob', "value": "mediumblob" },
                     { "key": 'longblob', "value": "longblob" },
                     { "key": 'json', "value": "json" },
                     { "key": 'set', "value": "set" },
                     { "key": 'enum', "value": "enum" }
                  ]
               },
               {
                  "name": "attr",
                  "webName": $scope.pAutoLanguage( "属性" ),
                  "type": "select",
                  "value": columnType.indexOf( 'zerofill' ) > 0 ? "zerofill" : ( columnType.indexOf( 'unsigned' ) > 0 ? "unsigned" : "" ) ,
                  "valid": [
                     { "key": $scope.pAutoLanguage( "无属性" ), "value": "" },
                     { "key": $scope.pAutoLanguage( "无符号" ), "value": "unsigned" },
                     { "key": $scope.pAutoLanguage( "零填充" ), "value": "zerofill" }
                  ]
               },
               {
                  "name": "length",
                  "webName": $scope.pAutoLanguage( "长度/值" ),
                  "type": "string",
                  "desc": $scope.pAutoLanguage( '如字段类型是set或enum时，请在“长度/值”的输入框填写枚举的值，用半角逗号(,)隔开。' ),
                  "value": length
               },
               {
                  "name": "comment",
                  "webName": $scope.pAutoLanguage( "注释" ),
                  "type": "string",
                  "value": comment
               }
            ]
         }
         $scope.EditFieldWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改字段' ) ) ;
         $scope.EditFieldWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.EditFieldWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.EditFieldWindow['config'].check() ;
            if( isClear == true )
            {
               var subSql = '' ;
               var formVal = $scope.EditFieldWindow['config'].getValue() ;
               var sql = sprintf( 'alter table `?` change `?` `?` ?', SdbSwap.tbName, fieldName, formVal['fieldName'], formVal['newType'] ) ;
               if( formVal['length'].length > 0 )
               {
                  switch( formVal['newType'] )
                  {
                  case 'varchar':
                  case 'character':
                  case 'char':
                  case 'decimal':
                  case 'numeric':
                  case 'int':
                  case 'bit':
                  case 'bigint':
                  case 'tinyint':
                  case 'smallint':
                  case 'mediumint':
                  case 'datetime':
                  case 'timestamp':
                  case 'time':
                  case 'binary':
                  case 'double':
                  case 'float':
                     subSql += '(' + formVal['length'] + ') ' ;
                     break ;
                  case 'set':
                  case 'enum':
                     
                     if( formVal['length'].indexOf( ',' ) >= 0 )
                     {
                        var tmpArray = formVal['length'].split( ',' ) ;
                        var first = true ;
                        subSql += '(' ;
                        $.each( tmpArray, function( index, value ){
                           if( !first )
                           {
                              subSql += ',' ;
                           }
                           subSql = subSql + "'" + value + "'" ;
                           first = false ;
                        } ) ;
                        subSql += ')' ;
                     }
                     else
                     {
                        subSql += '(\'' + formVal['length'] + '\')' ;
                     }
                  default:
                     subSql += ' ' ;
                     break ;
                  }
               }
               else if( ( type == 'set' || type == 'enum' ) && ( formVal['newType'] == 'set' || formVal['newType'] == 'enum') )
               {
                  subSql = columnType.replace( type, '' ) ;
               }
               sql += subSql ;
               if( !isEmpty( formVal['attr'] ) )
               {
                  switch( formVal['newType'] )
                  {
                  case 'tinyint':
                  case 'smallint':
                  case 'mediumint':
                  case 'int':
                  case 'double':
                  case 'bigint':
                  case 'decimal':
                  case 'float':
                     sql += ' ' + formVal['attr'] + ' ' ;
                     break ;
                  default:
                     break ;
                  }
               }
               sql += ' comment ' + sqlEscape( formVal['comment'] ) ;
               execSql( sql ) ;
               $scope.EditFieldWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.EditFieldWindow['callback']['Open']() ;
      }

      //编辑字段 下拉菜单
      $scope.EditFieldDropdown = {
         'config': [
            { 'key': $scope.pAutoLanguage( '修改字段' ) },
            { 'key': $scope.pAutoLanguage( '设置默认值' ) },
            { 'key': $scope.pAutoLanguage( '删除默认值' ) }
         ],
         'callback': {}
      } ;

      //打开 编辑字段 下拉菜单
      SdbSignal.on( 'ShowEditFieldDropdown', function( result ){
         var field = result['field'] ;
         $scope.EditFieldDropdown['OnClick'] = function( index ){
            if( index == 0 )
            {
               showEditField( field['COLUMN_NAME'], field['DATA_TYPE'], field['CHARACTER_MAXIMUM_LENGTH'], field['COLUMN_TYPE'], field['COLUMN_COMMENT'] ) ;
            }
            else if( index == 1 )
            {
               showSetDefault( field['COLUMN_NAME'] ) ;
            }
            else
            {
               shwoRemoveDefault( field['COLUMN_NAME'] ) ;
            }

            $scope.EditFieldDropdown['callback']['Close']() ;
         }
         $scope.EditFieldDropdown['callback']['Open']( result['event'].currentTarget ) ;
      } ) ;
   } ) ;

   //表格 控制器
   sacApp.controllerProvider.register( 'Data.MySQL.Structure.Table.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      //表格
      $scope.GridTable = {
         'title': {
            'index'            : '#',
            'COLUMN_NAME'      : $scope.pAutoLanguage( '字段名' ),
            'operation'        : '',
            'DATA_TYPE'        : $scope.pAutoLanguage( '类型' ),
            'COLUMN_DEFAULT'   : $scope.pAutoLanguage( '默认值' ),
            'IS_NULLABLE'      : $scope.pAutoLanguage( '空' ),
            'COLUMN_COMMENT'   : $scope.pAutoLanguage( '注释' )
         },
         'body': [],
         'options': {
            'width': {
               'index'            : '35px',
               'COLUMN_NAME'      : '19%',
               'operation'        : '60px',
               'DATA_TYPE'        : '19%',
               'COLUMN_DEFAULT'   : '18%',
               'IS_NULLABLE'      : '19%',
               'COLUMN_COMMENT'   : '25%'
            },
            'sort': {
               'index'            : true,
               'COLUMN_NAME'      : true,
               'operation'        : false,
               'DATA_TYPE'        : true,
               'COLUMN_DEFAULT'   : true,
               'IS_NULLABLE'      : true,
               'COLUMN_COMMENT'   : false
            },
            'max': 50
         }
      } ;


      SdbSignal.on( 'setTableData', function( result ){
         $scope.GridTable['body'] = result ;
      } ) ;

      //打开 编辑字段 下拉菜单
      $scope.ShowEditFieldDropdown = function( event, field ){
         SdbSignal.commit( 'ShowEditFieldDropdown', {
           'event': event,
           'field': field
         } ) ;
      }

      //打开 删除字段 弹窗
      $scope.ShowDropFieldWindow = function( fieldName ){
         SdbSignal.commit( 'ShowDropFieldWindow', fieldName ) ;
      }
      $scope.QueryTableStruct() ;

   } ) ;

}());