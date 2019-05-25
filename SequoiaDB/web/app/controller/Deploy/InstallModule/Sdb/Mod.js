//@ sourceURL=Mod.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, $interval, SdbRest, SdbFunction, Loading ){
      
      //初始化
      var selectGroup        = [ { 'key': $scope.autoLanguage( '全部' ), 'value': '' } ] ;
      $scope.GroupList       = [] ;
      $scope.NodeList        = [] ;
      $scope.Template        = [] ;
      $scope.installConfig   = {} ;
      $scope.StandaloneShow  = 1 ;
      $scope.StandaloneForm1 = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.StandaloneForm2 = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.StandaloneForm3 = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      //编辑配置
      $scope.ConfigWindows = {
         'config': {
            'type': 'json',
            'text': ''
         },
         'callback': {}
      } ;
      //节点列表
      $scope.NodeTable = {
         'title': {
            'checked':     '',
            'HostName':    $scope.autoLanguage( '主机名' ),
            'svcname':     $scope.autoLanguage( '服务名' ),
            'dbpath':      $scope.autoLanguage( '数据路径' ),
            'role':        $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' )
         },
         'body': [],
         'options': {
            'width': {
               'checked': '26px',
               'HostName': '23%',
               'svcname': '18%',
               'dbpath': '31%',
               'role': '10%',
               'datagroupname': '18%'
            },
            'sort': {
               'checked': false,
               'HostName': true,
               'svcname': true,
               'dbpath': true,
               'role': true,
               'datagroupname': true
            },
            'max': 50,
            'filter': {
               'checked': null,
               'HostName': 'indexof',
               'svcname': 'indexof',
               'dbpath': 'indexof',
               'role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'coord', 'value': 'coord' },
                  { 'key': 'catalog', 'value': 'catalog' },
                  { 'key': 'data', 'value': 'data' }
               ],
               'datagroupname': selectGroup
            }
         },
         'callback': {}
      } ;

      $scope.Configure   = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( deployType == null || clusterName == null || $scope.ModuleName == null || $scope.Configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployType, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //单机模式，切换普通和高级
      $scope.SwitchParam = function( type ){
         $scope.StandaloneShow = type ;
      }
      
      //创建分区组 弹窗
      $scope.CreateGroupWindow = {
         'config': {
            'inputList': [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '分区组名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         },
         'callback': {}
      } ;

      //打开 创建分区组 弹窗
      $scope.ShowCreateGroup = function(){
         $scope.CreateGroupWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.CreateGroupWindow['config'].check( function( thisValue ){
               var isFind = false ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  if( groupInfo['role'] == 'data' && groupInfo['groupName'] == thisValue['groupName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.CreateGroupWindow['config'].getValue() ;
               countGroup( 'data', formVal['groupName'], 0 ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.CreateGroupWindow['callback']['SetTitle']( $scope.autoLanguage( '创建分区组' ) ) ;
         $scope.CreateGroupWindow['callback']['Open']() ;
      }

      /*
      创建 设置节点配置 弹窗
            type    0   默认配置, 创建新节点
                    1   新建配置, 创建新节点
                    2   加载指定节点配置, 创建新节点
                    3   加载指定节点配置, 修改节点
                    4   加载批量节点配置, 修改节点
            groupIndex  分区组的索引id
            hostIndex   主机的索引id
            nodeIndex   节点的索引id
            isShow      是否马上打开弹窗
      */
      $scope.CreateSetNodeConfModel = function( type, groupIndex, hostIndex, currentNodeInfo, isShow ){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '编辑节点配置' ) ;
         $scope.Components.Modal.ShowType = 1 ;
         $scope.Components.Modal.form1 = {
            'keyWidth': '160px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
         } ;
         if( $scope.Configure['DeployMod'] == 'distribution' )
         {
            $scope.Components.Modal.form1['inputList'][0]['valid'] = {} ;
         }
         $scope.Components.Modal.form2 = {
            'keyWidth': '160px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 1 )
         } ;
         $scope.Components.Modal.form3 = {
            'keyWidth': '160px',
            'inputList': [
               {
                  "name": "other",
                  "webName": $scope.autoLanguage( "自定义配置" ),
                  "type": "list",
                  "valid": {
                     "min": 0
                  },
                  "child": [
                     [
                        {
                           "name": "name",
                           "webName": $scope.autoLanguage( "参数名" ), 
                           "placeholder": $scope.autoLanguage( "参数名" ),
                           "type": "string",
                           "valid": {
                              "min": 1
                           },
                           "default": "",
                           "value": ""
                        },
                        {
                           "name": "value",
                           "webName": $scope.autoLanguage( "值" ), 
                           "placeholder": $scope.autoLanguage( "值" ),
                           "type": "string",
                           "valid": {
                              "min": 1
                           },
                           "default": "",
                           "value": ""
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Switch1 = function(){
            $scope.Components.Modal.ShowType = 1 ;
         }
         $scope.Components.Modal.Switch2 = function(){
            $scope.Components.Modal.ShowType = 2 ;
         }
         $scope.Components.Modal.Switch3 = function(){
            $scope.Components.Modal.ShowType = 3 ;
         }
         if( type == 1 )
         {
            //空白配置
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               if( $scope.Components.Modal.form1['inputList'][index]['type'] != 'select' )
               {
                  $scope.Components.Modal.form1['inputList'][index]['value'] = '' ;
               }
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               if( $scope.Components.Modal.form2['inputList'][index]['type'] != 'select' )
               {
                  $scope.Components.Modal.form2['inputList'][index]['value'] = '' ;
               }
            } ) ;
         }
         else if( type == 2 || type == 3 )
         {
            var loadName = [] ;
            //加载指定某个节点的配置
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               var name = $scope.Components.Modal.form1['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               $scope.Components.Modal.form1['inputList'][index]['value'] = currentNodeInfo[name] ;
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               var name = $scope.Components.Modal.form2['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               $scope.Components.Modal.form2['inputList'][index]['value'] = currentNodeInfo[name] ;
            } ) ;
            //加载自定义配置项
            var isFirst = true ;
            $.each( currentNodeInfo, function( key, value ){
               if( key.toLowerCase() != 'hostname' &&
                   key.toLowerCase() != 'datagroupname' &&
                   key.toLowerCase() != 'role' &&
                   key.toLowerCase() != 'checked' &&
                   key.toLowerCase() != 'i' &&
                   loadName.indexOf( key.toLowerCase() ) == -1 )
               {
                  if( isFirst )
                  {
                     $scope.Components.Modal.form3['inputList'][0]['child'][0][0]['value'] = key ;
                     $scope.Components.Modal.form3['inputList'][0]['child'][0][1]['value'] = value ;
                     isFirst = false ;
                  }
                  else
                  {
                     var newInput = $.extend( true, [], $scope.Components.Modal.form3['inputList'][0]['child'][0] ) ;
                     newInput[0]['value'] = key ;
                     newInput[1]['value'] = value ;
                     $scope.Components.Modal.form3['inputList'][0]['child'].push( newInput ) ;
                  }
               }
            } ) ;
         }
         else if( type == 4 )
         {
            var loadName = [] ;
            //批量加载配置
            var sum = 0 ;
            $.each( $scope.NodeList, function( index ){
               if( $scope.NodeList[index]['checked'] == true )
               {
                  ++sum ;
               }
            } ) ;
            if( sum == 0 )
            {
               _IndexPublic.createInfoModel( $scope, $scope.autoLanguage( '修改配置至少需要选择一个节点。' ), $scope.autoLanguage( '好的' ) ) ;
               return ;
            }
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               var isFirst = true ;
               var name = $scope.Components.Modal.form1['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               var value = '' ;
               var offset = null ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['checked'] == true )
                  {
                     if( name == 'dbpath' )
                     {
                        if( isFirst == true )
                        {
                           value = selectDBPath( $scope.NodeList[index2]['dbpath'], $scope.NodeList[index2]['role'], $scope.NodeList[index2]['svcname'], $scope.NodeList[index2]['datagroupname'], $scope.NodeList[index2]['HostName'] ) ;
                           isFirst = false ;
                        }
                        if( value != selectDBPath( $scope.NodeList[index2]['dbpath'], $scope.NodeList[index2]['role'], $scope.NodeList[index2]['svcname'], $scope.NodeList[index2]['datagroupname'], $scope.NodeList[index2]['HostName'] ) )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                     else if( name == 'svcname' )
                     {
                        if( isFirst == true )
                        {
                           value = $scope.NodeList[index2]['svcname'] ;
                           isFirst = false ;
                        }
                        else
                        {
                           if( offset == null )
                           {
                              offset = parseInt( $scope.NodeList[index2]['svcname'] ) - parseInt( $scope.NodeList[index2-1]['svcname'] ) ;
                              if( offset != 0 )
                              {
                                 value = value + '[' + ( offset > 0 ? '+' : '' ) + offset + ']' ;
                              }
                           }
                           else
                           {
                              if( offset != parseInt( $scope.NodeList[index2]['svcname'] ) - parseInt( $scope.NodeList[index2-1]['svcname'] ) )
                              {
                                 value = '' ;
                                 return false ;
                              }
                           }
                        }
                     }
                     else
                     {
                        if( isFirst == true )
                        {
                           value = $scope.NodeList[index2][name] ;
                           isFirst = false ;
                        }
                        if( value != $scope.NodeList[index2][name] )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                  }
               } ) ;
               $scope.Components.Modal.form1['inputList'][index]['value'] = value ;
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               var isFirst = true ;
               var name = $scope.Components.Modal.form2['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               var value = '' ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['checked'] == true )
                  {
                     if( isFirst == true )
                     {
                        value = $scope.NodeList[index2][name] ;
                        isFirst = false ;
                     }
                     if( value != $scope.NodeList[index2][name] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               } ) ;
               $scope.Components.Modal.form2['inputList'][index]['value'] = value ;
            } ) ;
            //加载自定义配置项
            var customConfig = [] ;
            $.each( $scope.NodeList, function( nodeIndex ){
               if( $scope.NodeList[nodeIndex]['checked'] == true )
               {
                  $.each( $scope.NodeList[nodeIndex], function( key, value ){
                     if( key.toLowerCase() != 'hostname' &&
                         key.toLowerCase() != 'datagroupname' &&
                         key.toLowerCase() != 'role' &&
                         key.toLowerCase() != 'checked' &&
                         key.toLowerCase() != 'i' &&
                         loadName.indexOf( key.toLowerCase() ) == -1 &&
                         customConfig.indexOf( key.toLowerCase() ) == -1 )
                     {
                        customConfig.push( key ) ;
                     }
                  } ) ;
               }
            } ) ;
            var isFirst = true ;
            $.each( customConfig, function( customIndex, config ){
               var value = '' ;
               var isFirst2 = true ;
               $.each( $scope.NodeList, function( nodeIndex ){
                  if( $scope.NodeList[nodeIndex]['checked'] == true )
                  {
                     if( isFirst2 == true )
                     {
                        value = $scope.NodeList[nodeIndex][config] ;
                        isFirst2 = false ;
                     }
                     if( value != $scope.NodeList[nodeIndex][config] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               } ) ;
               if( isFirst )
               {
                  $scope.Components.Modal.form3['inputList'][0]['child'][0][0]['value'] = config ;
                  $scope.Components.Modal.form3['inputList'][0]['child'][0][1]['value'] = value ;
                  isFirst = false ;
               }
               else
               {
                  var newInput = $.extend( true, [], $scope.Components.Modal.form3['inputList'][0]['child'][0] ) ;
                  newInput[0]['value'] = config ;
                  newInput[1]['value'] = value ;
                  $scope.Components.Modal.form3['inputList'][0]['child'].push( newInput ) ;
               }
            } ) ;
         }
         if( isShow )
         {
            $scope.Components.Modal.isShow = true ;
         }
         else
         {
            $scope.Components.Modal.isRepaint = new Date().getTime() ;
         }
         $scope.Components.Modal.Context = '\
<div class="underlineTab" style="height:50px;">\
   <ul class="left">\
      <li ng-class="{true:\'active\'}[data.ShowType == 1]">\
         <a class="linkButton" ng-click="data.Switch1()">' + $scope.autoLanguage( '普通' ) + '</a>\
      </li>\
      <li ng-class="{true:\'active\'}[data.ShowType == 2]">\
         <a class="linkButton" ng-click="data.Switch2()">' + $scope.autoLanguage( '高级' ) + '</a>\
      </li>\
      <li ng-class="{true:\'active\'}[data.ShowType == 3]">\
         <a class="linkButton" ng-click="data.Switch3()">' + $scope.autoLanguage( '自定义' ) + '</a>\
      </li>\
   </ul>\
</div>\
<div form-create para="data.form1" ng-show="data.ShowType == 1"></div>\
<div form-create para="data.form2" ng-show="data.ShowType == 2"></div>\
<div form-create para="data.form3" ng-show="data.ShowType == 3"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear1 = $scope.Components.Modal.form1.check( function( valueList ){
               var error = [] ;
               if( type != 4 && valueList['dbpath'].length == 0 )
               {
                  error.push( { 'name': 'dbpath', 'error': $scope.autoLanguage( '数据路径不能为空。' ) } ) ;
               }
               if( type != 4 && valueList['svcname'].length == 0 )
               {
                  error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名不能为空。' ) } ) ;
               }
               else if( type != 4 && portEscape( valueList['svcname'], 0 ) == null )
               {
                  error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名格式错误。' ) } ) ;
               }
               return error ;
            } ) ;
            var isAllClear2 = $scope.Components.Modal.form2.check() ;
            var isAllClear3 = $scope.Components.Modal.form3.check( function( valueList ){
               var error = [] ;
               $.each( valueList['other'], function( index, configInfo ){
                  if( configInfo['name'].toLowerCase() == 'hostname' ||
                      configInfo['name'].toLowerCase() == 'datagroupname' ||
                      configInfo['name'].toLowerCase() == 'role' ||
                      configInfo['name'].toLowerCase() == 'checked' ||
                      configInfo['name'].toLowerCase() == 'i' )
                  {
                     error.push( { 'name': 'other', 'error': $scope.autoLanguage( '自定义配置不能设置HostName、datagroupname、role、checked、i。' ) } ) ;
                     return false ;
                  }
               } )
               return error ;
            } ) ;
            if( isAllClear1 && isAllClear2 && isAllClear3 )
            {
               var formVal1 = $scope.Components.Modal.form1.getValue() ;
               var formVal2 = $scope.Components.Modal.form2.getValue() ;
               var formVal3 = $scope.Components.Modal.form3.getValue() ;
               var formVal = $.extend( true, formVal1, formVal2 ) ;
               $.each( formVal3['other'], function( index, configInfo ){
                  if( configInfo['name'] != '' )
                  {
                     formVal[ configInfo['name'] ] = configInfo['value'] ;
                  }
               } ) ;
               if( type == 0 || type == 1 || type == 2 )
               {
                  //创建节点
                  ++$scope.GroupList[groupIndex]['nodeNum'] ;
                  formVal['HostName'] = $scope.Configure['HostInfo'][hostIndex]['HostName'] ;
                  formVal['datagroupname'] = $scope.GroupList[groupIndex]['groupName'] ;
                  formVal['role'] = $scope.GroupList[groupIndex]['role'] ;
                  formVal['svcname'] = portEscape( formVal['svcname'], 0 ) ;
                  formVal['dbpath'] = dbpathEscape( formVal['dbpath'], formVal['HostName'], formVal['svcname'], formVal['role'], formVal['datagroupname'] ) ;
                  formVal['i'] = $scope.NodeList.length ;
                  $scope.NodeList.push( formVal ) ;
                  if( $scope.GroupList[groupIndex]['role'] != 'coord' && $scope.GroupList[groupIndex]['nodeNum'] >= 7 )
                  {
                     $scope.GroupList[groupIndex]['DropdownMenu'][0]['disabled'] = true ;
                  }
                  if( $scope.GroupList[groupIndex]['nodeNum'] > 0 )
                  {
                     $scope.GroupList[groupIndex]['DropdownMenu'][1]['disabled'] = false ;
                  }
               }
               else if( type == 3 )
               {
                  //保存单个节点配置
                  formVal['svcname'] = portEscape( formVal['svcname'], 0 ) ;
                  formVal['dbpath']  = dbpathEscape( formVal['dbpath'], formVal['HostName'], formVal['svcname'], currentNodeInfo['role'], formVal['datagroupname'] ) ;
                  $.each( formVal, function( key, value ){
                     if( key == '' )
                     {
                        return true ;
                     }
                     currentNodeInfo[key] = value ;
                  } ) ;
               }
               else if( type == 4 )
               {
                  //保存批量节点配置
                  var num = 0 ;
                  $.each( $scope.NodeList, function( index ){
                     if( $scope.NodeList[index]['checked'] == true )
                     {
                        //把配置复制出来
                        var newFormVal = $.extend( true, {}, formVal ) ;
                        //根据实际节点，转换服务名和路径
                        newFormVal['svcname'] = portEscape( newFormVal['svcname'], num ) ;
                        newFormVal['dbpath']  = dbpathEscape( formVal['dbpath'],
                                                              formVal['HostName'],
                                                              newFormVal['svcname'].length == 0 ? $scope.NodeList[index]['svcname'] : newFormVal['svcname'],
                                                              $scope.NodeList[index]['role'],
                                                              formVal['datagroupname'] ) ;
                        $scope.NodeList[index] = {
                           'HostName': $scope.NodeList[index]['HostName'],
                           'datagroupname': $scope.NodeList[index]['datagroupname'],
                           'dbpath': $scope.NodeList[index]['dbpath'],
                           'svcname': $scope.NodeList[index]['svcname'],
                           'role': $scope.NodeList[index]['role'],
                           'checked': $scope.NodeList[index]['checked'],
                           'i': $scope.NodeList[index]['i']
                        } ;
                        $.each( newFormVal, function( key, value ){
                           if( ( ( key == 'dbpath' || key == 'svcname' ) && value.length == 0 ) || key == '' )
                           {
                              return true ;
                           }
                           $scope.NodeList[index][key] = value ;
                        } ) ;
                        ++num ;
                     }
                  } ) ;
               }
            }
            else
            {
               if( !isAllClear1 )
               {
                  $scope.Components.Modal.ShowType = 1 ;
               }
               else if( !isAllClear2 )
               {
                  $scope.Components.Modal.ShowType = 2 ;
               }
               else if( !isAllClear3 )
               {
                  $scope.Components.Modal.ShowType = 3 ;
               }
            }
            return isAllClear1 && isAllClear2 && isAllClear3 ;
         }
      }

      //创建 添加节点第一步 弹窗
      $scope.CreateAddNodeModel = function( index ){
         if( $scope.GroupList[index]['nodeNum'] >=7 )
         {
            return ;
         }
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '添加节点' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form1 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '新建配置' ), 'value': 1 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 2 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value == 2 )
                     {
                        $scope.Components.Modal.ShowType = 2 ;
                        $scope.Components.Modal.form2['inputList'][0]['value'] = $scope.Components.Modal.form1['inputList'][0]['value'] ;
                        $scope.Components.Modal.form2['inputList'][1]['value'] = $scope.Components.Modal.form1['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "hostname",
                  "webName": $scope.autoLanguage( '添加节点的主机' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               }
            ]
         } ;
         $scope.Components.Modal.form2 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 2,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '新建配置' ), 'value': 1 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 2 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value != 2 )
                     {
                        $scope.Components.Modal.ShowType = 1 ;
                        $scope.Components.Modal.form1['inputList'][0]['value'] = $scope.Components.Modal.form2['inputList'][0]['value'] ;
                        $scope.Components.Modal.form1['inputList'][1]['value'] = $scope.Components.Modal.form2['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "hostname",
                  "webName": $scope.autoLanguage( '添加节点的主机' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               },
               {
                  "name": "copyNode",
                  "webName": $scope.autoLanguage( '复制的节点' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
            $scope.Components.Modal.form1['inputList'][1]['valid'].push( { 'key': hostInfo['HostName'], 'value': index2 } ) ;
            $scope.Components.Modal.form2['inputList'][1]['valid'].push( { 'key': hostInfo['HostName'], 'value': index2 } ) ;
         } ) ;
         $.each( $scope.NodeList, function( index2, nodeInfo ){
            $scope.Components.Modal.form2['inputList'][2]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'] + ' ' + nodeInfo['role'], 'value': index2 } ) ;
         } ) ;
         $scope.Components.Modal.ShowType = 1 ;
         $scope.Components.Modal.Context = '<div ng-show="data.ShowType == 1" form-create para="data.form1"></div><div ng-show="data.ShowType == 2" form-create para="data.form2"></div>' ;
         $scope.Components.Modal.ok = function(){
            var form ;
            if( $scope.Components.Modal.ShowType == 1 )
            {
               form = $scope.Components.Modal.form1 ;
            }
            else
            {
               form = $scope.Components.Modal.form2 ;
            }
            var isAllClear = form.check() ;
            if( isAllClear )
            {
               var formVal = form.getValue() ;
               $scope.CreateSetNodeConfModel( formVal['createModel'], index, formVal['hostname'], $scope.NodeList[formVal['copyNode']], false ) ;
            }
            else
            {
               return false ;
            }
         }
      }

      //删除节点 弹窗
      $scope.RemoveNodeWindow = {
         'config': {
            'inputList': [
               {
                  "name": "nodename",
                  "webName": $scope.autoLanguage( '节点名' ),
                  "type": "select",
                  "value": '',
                  "valid": []
               }
            ]
         },
         'callback': {}
      } ;

      //打开 删除节点 弹窗
      $scope.ShowRemoveNode = function( index ){
         if( $scope.GroupList[index]['nodeNum'] == 0 )
         {
            return;
         }
         var isFirst = true ;
         $.each( $scope.NodeList, function( index2, nodeInfo ){
            if( $scope.GroupList[index]['role'] == 'data' && nodeInfo['datagroupname'] == $scope.GroupList[index]['groupName'] )
            {
               if( isFirst == true )
               {
                  $scope.RemoveNodeWindow['config']['inputList'][0]['value'] = index2 ;
                  isFirst = false ;
               }
               $scope.RemoveNodeWindow['config']['inputList'][0]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'], 'value': index2 } ) ;
            }
            if( $scope.GroupList[index]['role'] != 'data' && nodeInfo['role'] == $scope.GroupList[index]['role'] )
            {
               if( isFirst == true )
               {
                  $scope.RemoveNodeWindow['config']['inputList'][0]['value'] = index2 ;
                  isFirst = false ;
               }
               $scope.RemoveNodeWindow['config']['inputList'][0]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'], 'value': index2 } ) ;
            }
         } ) ;
         $scope.RemoveNodeWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveNodeWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveNodeWindow['config'].getValue() ;
               $scope.NodeList.splice( formVal['nodename'], 1 ) ;
               $.each( $scope.NodeList, function( index, nodeInfo ){
                  $scope.NodeList[index]['i'] = index ;
               } ) ;
               --$scope.GroupList[index]['nodeNum'] ;
               //没有节点了，禁用删除节点的按钮
               if( $scope.GroupList[index]['nodeNum'] < 7 )
               {
                  $scope.GroupList[index]['DropdownMenu'][0]['disabled'] = false ;
               }
               if( $scope.GroupList[index]['nodeNum'] == 0 )
               {
                  $scope.GroupList[index]['DropdownMenu'][1]['disabled'] = true ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveNodeWindow['callback']['SetTitle']( $scope.autoLanguage( '删除节点' ) ) ;
         $scope.RemoveNodeWindow['callback']['Open']() ;
      }

      //修改分区组名 弹窗
      $scope.RenameGroupWindow = {
         'config': {},
         'callback': {}
      } ;
      //打开 修改分区组名 弹窗
      $scope.ShowRenameGroup = function( index ){
         $scope.RenameGroupWindow['config'] = {
            'inputList': [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '新的分区组名' ),
                  "type": "string",
                  "required": true,
                  "value": $scope.GroupList[index]['groupName'],
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         } ;
         $scope.RenameGroupWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RenameGroupWindow['config'].check( function( thisValue ){
               var isFind = false ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  if( groupInfo['role'] == 'data' && groupInfo['groupName'] == thisValue['groupName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.RenameGroupWindow['config'].getValue() ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['datagroupname'] == $scope.GroupList[index]['groupName'] )
                  {
                     $scope.NodeList[index2]['datagroupname'] = formVal['groupName'] ;
                  }
               } ) ;
               $scope.GroupList[index]['groupName'] = formVal['groupName'] ;
               if( index >= 1 )
               {
                  selectGroup[ index - 1 ]['key'] = formVal['groupName'] ;
                  selectGroup[ index - 1 ]['value'] = formVal['groupName'] ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.RenameGroupWindow['callback']['SetTitle']( $scope.autoLanguage( '修改分区组名' ) ) ;
         $scope.RenameGroupWindow['callback']['Open']() ;
      }

      //创建 删除分区组 弹窗
      $scope.CreateRemoveGroupModel = function( index ){
         if( $scope.GroupList[index]['nodeNum'] > 0 )
         {
            _IndexPublic.createInfoModel( $scope, sprintf( $scope.autoLanguage( '确定要把分区组?和分区组下的?个节点都删除吗？' ), $scope.GroupList[index]['groupName'], $scope.GroupList[index]['nodeNum'] ), $scope.autoLanguage( '是的' ), function(){
               var removeNode = function(){
                  var isFind = false ;
                  $.each( $scope.NodeList, function( index2 ){
                     if( $scope.NodeList[index2]['datagroupname'] == $scope.GroupList[index]['groupName'] )
                     {
                        $scope.NodeList.splice( index2, 1 ) ;
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == true )
                  {
                     removeNode() ;
                  }
               }
               removeNode() ;
               $scope.GroupList.splice( index, 1 ) ;
               if( index >= 1 )
               {
                  selectGroup.splice( index - 1, 1 ) ;
               }
               $scope.GroupList = [] ;
               $.each( $scope.NodeList, function( index, nodeInfo ){
                  countGroup( nodeInfo['role'], nodeInfo['datagroupname'], 1 ) ;
               } ) ;
            } ) ;
         }
         else
         {
            $scope.GroupList.splice( index, 1 ) ;
            if( index >= 1 )
            {
               selectGroup.splice( index - 1, 1 ) ;
            }
         }
      }

      //导入配置
      var saveConfig = function(){
         var isObject = function( val ){
            return typeof( val ) == 'object' ;
         }
         //补充缺漏的配置
         var addConfig = function( nodeInfo ){
            if( typeof( nodeInfo['HostName'] ) == 'undefined' )
            {
               nodeInfo['HostName'] = 'Unknown' ;
            }
            $.each( $scope.Template, function( index, configInfo ){
               if( typeof( nodeInfo[ configInfo['Name'] ] ) == 'undefined' )
               {
                  nodeInfo[ configInfo['Name'] ] = String( configInfo['Default'] ) ;
               }
            } ) ;
         }
         var json, data ;
         //转成json字符串
         if( $scope.ConfigWindows['config']['type'] == 'json' )
         {
            data = $scope.ConfigWindows['config']['text'] ;
         }
         else if( $scope.ConfigWindows['config']['type'] == 'xml' )
         {
            var xotree = new XML.ObjTree();
				var dumper = new JKL.Dumper(); 
				var tree = xotree.parseXML( $scope.ConfigWindows['config']['text'] ) ;
				data = dumper.dump( tree ) ;
         }
         //解析成对象
         try{
            data = JSON.parse( data ) ;
         }catch( e ){
            alert( e.message ) ;
            return false ;
         }
         //简单校验
         if( isObject( data['Deploy'] ) == false )
         {
            alert( sprintf( $scope.autoLanguage( '导入失败, ?解析失败。' ), 'Deploy' ) ) ;
            return false ;
         }
         var i = 0 ;
         $scope.installConfig['Config'] = [] ;
         //转换coord
         if( isObject( data['Deploy']['Coord'] ) )
         {
            if( isArray( data['Deploy']['Coord']['Node'] ) )
            {
               $.each( data['Deploy']['Coord']['Node'], function( index, nodeInfo ){
                  nodeInfo['role'] = 'coord' ;
                  nodeInfo['datagroupname'] = '' ;
                  nodeInfo['checked'] = false ;
                  nodeInfo['i'] = i ;
                  addConfig( nodeInfo ) ;
                  $scope.installConfig['Config'].push( nodeInfo ) ;
                  ++i ;
               } ) ;
            }
            else if( isObject( data['Deploy']['Coord']['Node'] ) )
            {
               var nodeInfo = data['Deploy']['Coord']['Node'] ;
               nodeInfo['role'] = 'coord' ;
               nodeInfo['datagroupname'] = '' ;
               nodeInfo['checked'] = false ;
               nodeInfo['i'] = i ;
               addConfig( nodeInfo ) ;
               $scope.installConfig['Config'].push( nodeInfo ) ;
               ++i ;
            }
         }
         //转换catalog
         if( isObject( data['Deploy']['Catalog'] ) )
         {
            if( isArray( data['Deploy']['Catalog']['Node'] ) )
            {
               $.each( data['Deploy']['Catalog']['Node'], function( index, nodeInfo ){
                  nodeInfo['role'] = 'catalog' ;
                  nodeInfo['datagroupname'] = '' ;
                  nodeInfo['checked'] = false ;
                  nodeInfo['i'] = i ;
                  addConfig( nodeInfo ) ;
                  $scope.installConfig['Config'].push( nodeInfo ) ;
                  ++i ;
               } ) ;
            }
            else if( isObject( data['Deploy']['Catalog']['Node'] ) )
            {
               var nodeInfo = data['Deploy']['Catalog']['Node'] ;
               nodeInfo['role'] = 'catalog' ;
               nodeInfo['datagroupname'] = '' ;
               nodeInfo['checked'] = false ;
               nodeInfo['i'] = i ;
               addConfig( nodeInfo ) ;
               $scope.installConfig['Config'].push( nodeInfo ) ;
               ++i ;
            }
         }
         //转换data
         if( isObject( data['Deploy']['Data'] ) )
         {
            if( isArray( data['Deploy']['Data']['Group'] ) )
            {
               $.each( data['Deploy']['Data']['Group'], function( index, groupInfo ){
                  if( typeof( groupInfo['GroupName'] ) == 'string' && isArray( groupInfo['Node'] ) )
                  {
                     $.each( groupInfo['Node'], function( index, nodeInfo ){
                        nodeInfo['role'] = 'data' ;
                        nodeInfo['datagroupname'] = groupInfo['GroupName'] ;
                        nodeInfo['checked'] = false ;
                        nodeInfo['i'] = i ;
                        addConfig( nodeInfo ) ;
                        $scope.installConfig['Config'].push( nodeInfo ) ;
                        ++i ;
                     } ) ;
                  }
                  else if( typeof( groupInfo['GroupName'] ) == 'string' && isObject( groupInfo['Node'] ) )
                  {
                     var nodeInfo = groupInfo['Node'] ;
                     nodeInfo['role'] = 'data' ;
                     nodeInfo['datagroupname'] = groupInfo['GroupName'] ;
                     nodeInfo['checked'] = false ;
                     nodeInfo['i'] = i ;
                     addConfig( nodeInfo ) ;
                     $scope.installConfig['Config'].push( nodeInfo ) ;
                     ++i ;
                  }
               } ) ;
            }
            else if( isObject( data['Deploy']['Data']['Group'] ) )
            {
               var groupInfo = data['Deploy']['Data']['Group'] ;
               if( typeof( groupInfo['GroupName'] ) == 'string' && isArray( groupInfo['Node'] ) )
               {
                  $.each( groupInfo['Node'], function( index, nodeInfo ){
                     nodeInfo['role'] = 'data' ;
                     nodeInfo['datagroupname'] = groupInfo['GroupName'] ;
                     nodeInfo['checked'] = false ;
                     nodeInfo['i'] = i ;
                     addConfig( nodeInfo ) ;
                     $scope.installConfig['Config'].push( nodeInfo ) ;
                     ++i ;
                  } ) ;
               }
               else if( typeof( groupInfo['GroupName'] ) == 'string' && isObject( groupInfo['Node'] ) )
               {
                  var nodeInfo = groupInfo['Node'] ;
                  nodeInfo['role'] = 'data' ;
                  nodeInfo['datagroupname'] = groupInfo['GroupName'] ;
                  nodeInfo['checked'] = false ;
                  nodeInfo['i'] = i ;
                  addConfig( nodeInfo ) ;
                  $scope.installConfig['Config'].push( nodeInfo ) ;
                  ++i ;
               }
            }
         }
         $scope.NodeList = $scope.installConfig['Config'] ;
         $scope.GroupList = [] ;
         selectGroup = [ { 'key': $scope.autoLanguage( '全部' ), 'value': '' } ] ;
         $scope.NodeTable['options']['filter'][5] = selectGroup ;
         $.each( $scope.NodeList, function( index, nodeInfo ){
            countGroup( nodeInfo['role'], nodeInfo['datagroupname'], 1 ) ;
         } ) ;
         $scope.NodeTable['body'] = $scope.NodeList ;
         return true ;
      }

      //创建 导出配置 弹窗
      $scope.CreateExportConfigModel = function(){
         $scope.BuildConfig() ;
         //设置确定按钮
         $scope.ConfigWindows['callback']['SetOkButton']( $scope.autoLanguage( '保存' ), function(){
            return saveConfig() ;
         } ) ;
         //关闭窗口滚动条
         $scope.ConfigWindows['callback']['DisableBodyScroll']() ;
         //设置标题
         $scope.ConfigWindows['callback']['SetTitle']( $scope.autoLanguage( '编辑配置' ) ) ;
         //设置图标
         $scope.ConfigWindows['callback']['SetIcon']( 'fa-edit' ) ;
         //打开窗口
         $scope.ConfigWindows['callback']['Open']() ;
      }

      //下载配置
      $scope.DownloadConfig = function(){
         var blob = new Blob( [ $scope.ConfigWindows['config']['text'] ], { type: "text/plain;charset=utf-8" } ) ;
         if( $scope.ConfigWindows['config']['type'] == 'json' )
         {
            saveAs( blob, $scope.ModuleName + '.json' ) ;
         }
         else if( $scope.ConfigWindows['config']['type'] == 'xml' )
         {
            saveAs( blob, $scope.ModuleName + '.xml' ) ;
         }
      }

      //生成对应格式的配置
      $scope.BuildConfig = function(){
         $scope.ConfigWindows['config']['text'] = '' ;
         var newConfig = { 'Deploy': { 'Coord': { 'Node': [] }, 'Catalog': { 'Node': [] }, 'Data': { 'Group': [] } } } ;
         var config = convertConfig() ;
         if( !config )
            return ;
         var groupIsExist = function( groupName ){
            var isExist = -1 ;
            $.each( newConfig['Deploy']['Data']['Group'], function( index, groupInfo ){
               if( groupInfo['GroupName'] == groupName )
               {
                  isExist = index ;
                  return false ;
               }
            } ) ;
            return isExist ;
         }
         if( config['DeployMod'] == 'distribution' )
         {
            Loading.create() ;
            var length = config['Config'].length ;
            var index = 0 ;
            //定时循环转换，防止浏览器卡死
            var timer = $interval( function(){
               var nodeInfo = config['Config'][index] ;
               if( nodeInfo['role'] == 'coord' )
               {
                  newConfig['Deploy']['Coord']['Node'].push( deleteJson( nodeInfo, [ 'datagroupname', 'role' ] ) ) ;
               }
               else if( nodeInfo['role'] == 'catalog' )
               {
                  newConfig['Deploy']['Catalog']['Node'].push( deleteJson( nodeInfo, [ 'datagroupname', 'role' ] ) ) ;
               }
               else if( nodeInfo['role'] == 'data' )
               {
                  var id = groupIsExist( nodeInfo['datagroupname'] ) ;
                  if( id >=0 )
                  {
                     newConfig['Deploy']['Data']['Group'][id]['Node'].push( deleteJson( nodeInfo, [ 'datagroupname', 'role' ] ) ) ;
                  }
                  else
                  {
                     newConfig['Deploy']['Data']['Group'].push( {
                        'GroupName': nodeInfo['datagroupname'],
                        'Node': [ deleteJson( nodeInfo, [ 'datagroupname', 'role' ] ) ]
                     } ) ;
                  }
               }
               ++index ;
               if( index >= length )
               {
                  $interval.cancel( timer ) ;
                  timer = $interval( function(){
                     if( $scope.ConfigWindows['config']['type'] == 'json' )
                     {
                        $scope.ConfigWindows['config']['text'] = JSON.stringify( newConfig, null, 3 ) ;
                     }
                     else if( $scope.ConfigWindows['config']['type'] == 'xml' )
                     {
                        var xotree = new XML.ObjTree();
                        $scope.ConfigWindows['config']['text'] = formatXml( xotree.writeXML( newConfig ) ) ;
                     }
                     Loading.cancel() ;
                     $interval.cancel( timer ) ;
                  }, 1 ) ;
               }
            }, 1 ) ;
         }
      }

      //选择分区组
      $scope.SwitchGroup = function( index ){
         //关闭分区组列表其他组的选中状态
         $.each( $scope.GroupList, function( index2 ){
            if( index != index2 )
            {
               $scope.GroupList[index2]['checked'] = false ;
            }
         } ) ;
         //切换分区组状态
         $scope.GroupList[index]['checked'] = !$scope.GroupList[index]['checked'] ;
         if( $scope.GroupList[index]['checked'] == true )
         {
            //选中状态
            $scope.NodeTable['callback']['SetFilter']( 'HostName', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'svcname', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'dbpath', '' ) ;
            if( $scope.GroupList[index]['role'] == 'data' )
            {
               $scope.NodeTable['callback']['SetFilter']( 'role', $scope.GroupList[index]['role'] ) ;
               $scope.NodeTable['callback']['SetFilter']( 'datagroupname', $scope.GroupList[index]['groupName'] ) ;
            }
            else
            {
               $scope.NodeTable['callback']['SetFilter']( 'role', $scope.GroupList[index]['role'] ) ;
               $scope.NodeTable['callback']['SetFilter']( 'datagroupname', '' ) ;
            }
         }
         else
         {
            //取消选中状态
            $scope.NodeTable['callback']['SetFilter']( 'role', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'datagroupname', '' ) ;
         }
      }

      //从节点列表中，聚合成分区组列表
      var countGroup = function( role, groupName, defaultNum ){
         var isFind = false ;
         $.each( $scope.GroupList, function( index, groupInfo ){
            if( ( role == 'data' && groupInfo['groupName'] == groupName ) || ( role != 'data' && groupInfo['role'] == role ) )
            {
               ++$scope.GroupList[index]['nodeNum'] ;
               $scope.GroupList[index]['DropdownMenu'][1]['disabled'] = false ;
               if( role != 'coord' && $scope.GroupList[index]['nodeNum'] >= 7 )
               {
                  $scope.GroupList[index]['DropdownMenu'][0]['disabled'] = true ;
               }
               isFind = true ;
               return false ;
            }
         } ) ;
         if( isFind == false )
         {
            var groupIndex = $scope.GroupList.length ;
            if( role == 'data' )
            {
               $scope.GroupList.push( { 'role': role, 'groupName': groupName, 'nodeNum': defaultNum, 'DropdownMenu': [
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateAddNodeModel(' + groupIndex + ')">{{autoLanguage("添加节点")}}</div>' )( $scope ), 'disabled': false },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="ShowRemoveNode(' + groupIndex + ')">{{autoLanguage("删除节点")}}</div>' )( $scope ), 'disabled': true },
                  {},
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="ShowRenameGroup(' + groupIndex + ')">{{autoLanguage("修改分区组名")}}</div>' )( $scope ) },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateRemoveGroupModel(' + groupIndex + ')">{{autoLanguage("删除分区组")}}</div>' )( $scope ) }
               ] } ) ;
            }
            else
            {
               $scope.GroupList.push( { 'role': role, 'groupName': groupName, 'nodeNum': defaultNum, 'DropdownMenu': [
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateAddNodeModel(' + groupIndex + ')">{{autoLanguage("添加节点")}}</div>' )( $scope ), 'disabled': false },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="ShowRemoveNode(' + groupIndex + ')">{{autoLanguage("删除节点")}}</div>' )( $scope ), 'disabled': true }
               ] } ) ;
            }
            if( role == 'data' )
            {
               selectGroup.push( { 'key': groupName, 'value': groupName } ) ;
            }
         }
      }

      //获取业务配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( $scope.Configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function( configure ){
               $scope.installConfig = configure[0] ;
               $scope.NodeList = configure[0]['Config'] ;
               $.each( $scope.NodeList, function( index ){
                  $scope.NodeList[index]['i'] = index ;
               } ) ;
               //删除单机版不需要的配置项
               if( $scope.Configure['DeployMod'] == 'standalone' )
               {
                  $scope.Template = [] ;
                  $.each( configure[0]['Property'], function( index, info ){
                     if( info['Name'] == 'preferedinstance' ||
                         info['Name'] == 'syncstrategy' ||
                         info['Name'] == 'weight' ||
                         info['Name'] == 'maxreplsync' )
                     {
                        return true ;
                     }
                     $scope.Template.push( info ) ;
                  } ) ;
               }
               else
               {
                  $scope.Template = configure[0]['Property'] ;
               }
               $scope.NodeTable['body'] = $scope.NodeList ;
               if( $scope.Configure['DeployMod'] == 'standalone' )
               {
                  $scope.StandaloneForm1 = {
                     'keyWidth': '160px',
                     'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
                  } ;
                  $scope.StandaloneForm2 = {
                     'keyWidth': '160px',
                     'inputList': _Deploy.ConvertTemplate( $scope.Template, 1 )
                  } ;
                  $scope.StandaloneForm3 = {
                     'keyWidth': '160px',
                     'inputList': [
                        {
                           "name": "other",
                           "webName": $scope.autoLanguage( "自定义配置" ),
                           "type": "list",
                           "valid": {
                              "min": 0
                           },
                           "child": [
                              [
                                 {
                                    "name": "name",
                                    "webName": $scope.autoLanguage( "参数名" ), 
                                    "placeholder": $scope.autoLanguage( "参数名" ),
                                    "type": "string",
                                    "valid": {
                                       "min": 1
                                    },
                                    "default": "",
                                    "value": ""
                                 },
                                 {
                                    "name": "value",
                                    "webName": $scope.autoLanguage( "值" ), 
                                    "placeholder": $scope.autoLanguage( "值" ),
                                    "type": "string",
                                    "valid": {
                                       "min": 1
                                    },
                                    "default": "",
                                    "value": ""
                                 }
                              ]
                           ]
                        }
                     ]
                  } ;
                  $.each( $scope.StandaloneForm1['inputList'], function( index ){
                     var name = $scope.StandaloneForm1['inputList'][index]['name'] ;
                     $scope.StandaloneForm1['inputList'][index]['value'] = $scope.NodeList[0][name] ;
                  } ) ;
                  $scope.StandaloneForm1['inputList'].splice( 0, 0, {
                     "name": "HostName",
                     "webName": $scope.autoLanguage( '主机名' ),
                     "type": "string",
                     "value": $scope.NodeList[0]['HostName'],
                     "disabled": true
                  } ) ;
                  $.each( $scope.StandaloneForm2['inputList'], function( index ){
                     var name = $scope.StandaloneForm2['inputList'][index]['name'] ;
                     $scope.StandaloneForm2['inputList'][index]['value'] = $scope.NodeList[0][name] ;
                  } ) ;
               }
               $.each( $scope.NodeList, function( index, nodeInfo ){
                  countGroup( nodeInfo['role'], nodeInfo['datagroupname'], 1 ) ;
               } ) ;
               $scope.$apply() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getModuleConfig() ;

      //帮助信息 弹窗
      $scope.HelperWindow = {
         'config': {},
         'callback': {}
      } ;
      
      //打开 帮助信息 弹窗
      $scope.ShowHelper = function(){
         $scope.HelperWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            $scope.HelperWindow['callback']['Close']() ;
         } ) ;
         $scope.HelperWindow['callback']['SetTitle']( $scope.autoLanguage( '帮助' ) ) ;
         $scope.HelperWindow['callback']['SetIcon']( '' ) ;
         $scope.HelperWindow['callback']['Open']() ;
      }

      //全选
      $scope.SelectAll = function(){
         var dataList ;
         var isFilter = $scope.NodeTable['callback']['GetFilterStatus']() ;
         if( isFilter )
         {
            //如果开了过滤，那么只修改过滤的
            dataList = $scope.NodeTable['callback']['GetFilterAllData']() ;
         }
         else
         {
            dataList = $scope.NodeTable['callback']['GetAllData']() ;
         }
         $.each( dataList, function( index ){
            dataList[index]['checked'] = true ;
         } ) ;
      }

      //反选
      $scope.Unselect = function(){
         var dataList ;
         var isFilter = $scope.NodeTable['callback']['GetFilterStatus']() ;
         if( isFilter )
         {
            //如果开了过滤，那么只修改过滤的
            dataList = $scope.NodeTable['callback']['GetFilterAllData']() ;
         }
         else
         {
            dataList = $scope.NodeTable['callback']['GetAllData']() ;
         }
         $.each( dataList, function( index ){
            dataList[index]['checked'] = !dataList[index]['checked'] ;
         } ) ;
      }

      $scope.GotoConf = function(){
         $location.path( '/Deploy/SDB-Conf' ).search( { 'r': new Date().getTime() } ) ;
      }

      var installSdb = function( installConfig ){
         var data = { 'cmd': 'add business', 'Force': true, 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/InstallModule' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installSdb( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      var convertConfig = function(){
         var configure = {} ;
         configure['ClusterName']  = $scope.installConfig['ClusterName'] ;
         configure['BusinessType'] = $scope.installConfig['BusinessType'] ;
         configure['BusinessName'] = $scope.installConfig['BusinessName'] ;
         configure['DeployMod']    = $scope.installConfig['DeployMod'] ;
         if( $scope.Configure['DeployMod'] == 'distribution' )
         {
            configure['Config'] = $.extend( true, [], $scope.installConfig['Config'] ) ;
            $.each( configure['Config'], function( index ){
               configure['Config'][index] = deleteJson( configure['Config'][index], [ 'checked', 'i' ] ) ;
               configure['Config'][index] = convertJsonValueString( configure['Config'][index] ) ;
            } ) ;
         }
         else if( $scope.Configure['DeployMod'] == 'standalone' )
         {
            var isAllClear1 = $scope.StandaloneForm1.check( function( formVal ){
               var error = [] ;
               if( checkPort( formVal['svcname'] ) == false )
               {
                  error.push( { 'name': 'svcname', 'error': sprintf( $scope.autoLanguage( '?格式错误。' ), $scope.autoLanguage( '服务名' ) ) } ) ;
               }
               if( formVal['dbpath'].length == 0 )
               {
                  error.push( { 'name': 'dbpath', 'error': sprintf( $scope.autoLanguage( '?长度不能小于?。' ), $scope.autoLanguage( '数据路径' ), 1 ) } ) ;
               }
               return error ;
            } ) ;
            var isAllClear2 = $scope.StandaloneForm2.check() ;
            var isAllClear3 = $scope.StandaloneForm3.check( function( valueList ){
               var error = [] ;
               $.each( valueList['other'], function( index, configInfo ){
                  if( configInfo['name'].toLowerCase() == 'hostname' ||
                      configInfo['name'].toLowerCase() == 'datagroupname' ||
                      configInfo['name'].toLowerCase() == 'role' ||
                      configInfo['name'].toLowerCase() == 'checked' ||
                      configInfo['name'].toLowerCase() == 'i' )
                  {
                     error.push( { 'name': 'other', 'error': $scope.autoLanguage( '自定义配置不能设置HostName、datagroupname、role、checked、i。' ) } ) ;
                     return false ;
                  }
               } )
               return error ;
            } ) ;
            if( isAllClear1 && isAllClear2 && isAllClear3 )
            {
               var formVal1 = $scope.StandaloneForm1.getValue() ;
               var formVal2 = $scope.StandaloneForm2.getValue() ;
               var formVal3 = $scope.StandaloneForm3.getValue() ;
               var formVal = $.extend( true, formVal1, formVal2 ) ;
               $.each( formVal3['other'], function( index, configInfo ){
                  if( configInfo['name'] != '' )
                  {
                     formVal[ configInfo['name'] ] = configInfo['value'] ;
                  }
               } ) ;
               configure['Config'] = [ {} ] ;
               $.each( formVal, function( key, value ){
                  configure['Config'][0][key] = value ;
               } ) ;
               configure['Config'][0]['role'] = 'standalone' ;
               configure['Config'][0]['datagroupname'] = '' ;
               configure['Config'][0] = convertJsonValueString( configure['Config'][0] ) ;
            }
            else
            {
               return ;
            }
         }
         return configure ;
      }

      $scope.GotoInstall = function(){
         var oldConfigure = convertConfig() ;
         if( typeof( oldConfigure ) == 'undefined' )
         {
            return ;
         }
         var configure = {} ;
         $.each( oldConfigure, function( key, value ){
            configure[key] = value ;
         } ) ;
         configure['Config'] = [] ;
         $.each( oldConfigure['Config'], function( nodeIndex, nodeInfo ){
            var nodeConfig = {} ;
            $.each( nodeInfo, function( key, value ){
               if( value.length > 0 || key == 'datagroupname' )
               {
                   nodeConfig[key] = value ;
               }
            } ) ;
            configure['Config'].push( nodeConfig ) ;
         } ) ;
         if( configure )
            installSdb( configure ) ;
      }

   } ) ;
}());