//@ sourceURL=Domain.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.Domain.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( moduleMode == 'standalone' )
      {
         $location.path( '/Monitor/SDB/Index').search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      //domain列表
      var domainList = [] ;

      //表单用的domain列表
      var domainNameList = [] ;

      //表单用的group列表
      var groupNameList = [] ;

      //显示列 下拉菜单
      $scope.FieldDropdown = {
         'config':[
            { 'key': 'Name',            'field': 'Name',              'show': true },
            { 'key': 'AutoSplit',       'field': 'AutoSplit',         'show': true },
            { 'key': 'Groups',          'field': 'GroupList',         'show': true },
            { 'key': 'CSList',          'field': 'CsList',            'show': true },
            { 'key': 'CLList',          'field': 'ClList',            'show': true },
            { 'key': 'TotalGroups',     'field': 'TotalGroups',       'show': true },
            { 'key': 'TotalCS',         'field': 'TotalCS',           'show': true },
            { 'key': 'TotalCL',         'field': 'TotalCL',           'show': true }
         ],
         'callback': {}
      } ;

      //新表格
      $scope.DomainTable = {
         'title': {
            'Name': 'Name',
            '_id': false,
            'AutoSplit': 'AutoSplit',
            'Groups': 'GroupList',
            'CSList': 'CSList',
            'CLList': 'CLList',
            'TotalGroups': 'TotalGroups',
            'TotalCS': 'TotalCS',
            'TotalCL': 'TotalCL'
         },
         'body': [],
         'options': {
            'width': {},
            'sort': {
               'Name': true,
               '_id': true,
               'AutoSplit': true,
               'Groups': true,
               'CSList': true,
               'CLList': true,
               'TotalGroups': true,
               'TotalCS': true,
               'TotalCL': true
            },
            'max': 50,
            'filter': {
               'Name': 'indexof',
               '_id': 'indexof',
               'AutoSplit': 'indexof',
               'Groups': 'indexof',
               'CSList': 'indexof',
               'CLList': 'indexof',
               'TotalGroups': 'number',
               'TotalCS': 'number',
               'TotalCL': 'number'
            }
         },
         'callback': {}
      } ;

      //domain详细信息 弹窗
      $scope.DomainInfo = {
         'config': {},
         'callback': {}
      } ;
      //创建domian 弹窗
      $scope.CreateDomain = {
         'config': {},
         'callback': {}
      } ;
      //删除domain 弹窗
      $scope.DeleteDomain = {
         'config': {},
         'callback': {}
      } ;
      //编辑domain 弹窗
      $scope.AlterDomain = {
         'config': {},
         'callback': {}
      } ;

      //获取CS列表
      var getCsList = function(){
         var sql = 'select Collection, Domain, Name from $LIST_CS where not Domain is null' ;
         SdbRest.Exec( sql, {
            'success': function( csList ){
               if( csList.length > 0 )
               {
                  $.each( csList, function( index, csInfo ){
                     $.each( domainList, function( domainIndex, domainInfo ){
                        if( csInfo['Domain'] == domainInfo['Name'] )
                        {
                           ++ domainList[domainIndex]['TotalCS'] ;
                           domainList[domainIndex]['CSList'].push( csInfo['Name'] ) ;
                           if( csInfo['Collection'].length >0 )
                           {
                              $.each( csInfo['Collection'], function( clIndex, clInfo ){
                                 domainList[domainIndex]['CLList'].push( clInfo['Name'] ) ;
                                 ++ domainList[domainIndex]['TotalCL'] ;
                              } ) ;
                           }
                        }
                        return ;
                     } ) ;
                  } ) ;
               }
               $.each( domainList, function( domainIndex, domainInfo ){
                  var groupList = [] ;
                  domainList[domainIndex]['CLList'] = domainInfo['CLList'] ? ( domainInfo['CLList'].length > 0 ? domainInfo['CLList'].join() : '-' ) : '-' ;
                  domainList[domainIndex]['CSList'] = domainInfo['CSList'] ? ( domainInfo['CSList'].length > 0 ? domainInfo['CSList'].join() : '-' ) : '-' ;
                  if( domainInfo['Groups'] )
                  {
                     $.each( domainInfo['Groups'], function( groupIndex, groupInfo ){
                        groupList.push( groupInfo['GroupName'] ) ;
                     } ) ;
                  }
                  domainList[domainIndex]['GroupList'] = groupList ;
                  domainList[domainIndex]['Groups'] = groupList.length > 0 ? groupList.join() : '-' ;
                  
               } ) ;
               $scope.DomainTable['body'] = domainList ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getCsList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //获取分区组列表
      var getGroupList = function(){
         groupNameList = [] ;
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               if( groups.length > 0 )
               {
                  $.each( groups, function( index, groupInfo ){
                     if( groupInfo['Role'] == 0 )
                     {
                        groupNameList.push( groupInfo['GroupName'] ) ;
                     }
                  } ) ;
               }
               $scope.GroupList = groups ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getGroupList() ;
                  return true ;
               } ) ;
            } 
         }, { 'showLoading': false } ) ;
      }

      //获取domain列表
      var getDomainList = function(){
         domainNameList = [] ;
         $scope.DomainTable['body'] = [] ;
         domainList = [] ;
         var data = { 'cmd': 'list domains' } ;
         SdbRest.DataOperation( data, {
            'success': function( domains ){
               //存在域时才执行
               if( domains.length > 0 )
               {
                  $.each( domains, function( index, domainInfo ){
                     domainInfo['_id'] = domainInfo['_id']['$oid'] ;
                     domainInfo['TotalGroups'] = domainInfo['Groups'] ? domainInfo['Groups'].length : 0 ;
                     domainInfo['AutoSplit'] = typeof( domainInfo['AutoSplit'] ) == 'undefined' ? 'false' : domainInfo['AutoSplit'] ;
                     domainInfo['CSList'] = [] ;
                     domainInfo['TotalCS'] = 0 ;
                     domainInfo['CLList'] = [] ;
                     domainInfo['TotalCL'] = 0 ;
                     //弹窗表单用
                     domainNameList.push( { 'key': domainInfo['Name'], 'value': domainInfo['Name'] } ) ;
                  } ) ;
                  domainList = domains ;
                  getCsList() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDomainList() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }

      getDomainList();
      getGroupList() ;

      //创建domain - 在弹窗创建domain时使用
      var createDomain = function( domainName, autoSplit, groupList ){
         var data = {
            'cmd': 'create domain',
            'name': domainName,
            'options': JSON.stringify( {
               'AutoSplit': autoSplit,
               'Groups': groupList
            } )
         } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               //刷新domain页面
               getDomainList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createDomain( domainName, autoSplit, groupList ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      //删除domain - 在弹窗删除domain时使用
      var deleteDomain = function( domainName ){
         var data = { 'cmd': 'drop domain', 'name': domainName } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               //刷新domain页面
               getDomainList() ;
            
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deleteDomain( domainName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //修改domain属性 - 在弹窗编辑domain时使用
      var alterDomain = function( domainName, autoSplit, groupList ){
         var data = { 'cmd': 'alter domain', 'name': domainName, 'options':JSON.stringify( { 'AutoSplit': autoSplit, 'Groups': groupList } ) } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               //刷新domain页面
               getDomainList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  alterDomain( domainName, autoSplit, groupList ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      //打开创建domain弹窗
      $scope.ShowCreateDomain = function(){
         //表单数据
         var createDomainForm = {
            inputList: [
               {
                  "name": "domainName",
                  "webName": $scope.autoLanguage( '域名' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid":{
                     "min": 1
                  }
               },
               {
                  "name": "autoSplit",
                  "webName": $scope.autoLanguage( '自动切分' ),
                  "type": "select",
                  "value": true,
                  "valid": [
                     { "key": "true", "value": true },
                     { "key": "false", "value": false }   
                  ]
               },
               {
                  "name": "groupList",
                  "webName": $scope.autoLanguage( '选择分区组' ),
                  "type": "multiple",
                  "value": [],
                  "valid": {
                     'min': 0,
                     'list': []
                  }
               }
            ]
         } ;

         $.each( groupNameList, function( index2, groupname ){
            createDomainForm['inputList'][2]['valid']['list'].push( {
               'key': groupname,
               'value': groupname,
               'checked': false
            } ) ;
         } ) ;

         $scope.CreateDomain['config'] = createDomainForm ;

         //设置确定按钮
         $scope.CreateDomain['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = createDomainForm.check() ;
            if( isAllClear )
            {
               var formVal = createDomainForm.getValue() ;
               //关闭窗口
               $scope.CreateDomain['callback']['Close']() ;
               //调用创建domain函数
               createDomain( formVal['domainName'], formVal['autoSplit'], formVal['groupList'] ) ;
            }
         } ) ;
         //设置标题
         $scope.CreateDomain['callback']['SetTitle']( $scope.autoLanguage( '创建域' ) ) ;
         //设置图标
         $scope.CreateDomain['callback']['SetIcon']( 'fa-plus' ) ;
         //打开窗口
         $scope.CreateDomain['callback']['Open']() ;
      }

      //打开删除domain弹窗
      $scope.ShowDeleteDomain = function(){
         if( $scope.DomainTable['body'].length > 0 )
         {
            var deleteDomainForm = {
               inputList: [
                  {
                     "name": "domainName",
                     "webName": $scope.autoLanguage( '域名' ),
                     "type": "select",
                     "value": domainNameList[0]['value'],
                     "valid": domainNameList
                  }
               ]
            } ;
            $scope.DeleteDomain['config'] = deleteDomainForm ;
            //设置确定按钮
            $scope.DeleteDomain['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = deleteDomainForm.check() ;
               if( isAllClear )
               {
                  var formVal = deleteDomainForm.getValue() ;
                  //关闭窗口
                  $scope.DeleteDomain['callback']['Close']() ;
                  //调用删除domain函数
                  deleteDomain( formVal['domainName'] ) ;
               }
            } ) ;
            //设置标题
            $scope.DeleteDomain['callback']['SetTitle']( $scope.autoLanguage( '删除域' ) ) ;
            //设置图标
            $scope.DeleteDomain['callback']['SetIcon']( 'fa-trash-o' ) ;
            //打开窗口
            $scope.DeleteDomain['callback']['Open']() ;
         }
      }
      
      //编辑domain弹窗
      $scope.ShowAlterDomain = function(){
         if( $scope.DomainTable['body'].length > 0 )
         {
            var setSelectGroup = function( alterDomainForm, currentGroupName ){
               $.each( domainList, function( index, domainInfo ){
                  if( domainInfo['Name'] == currentGroupName )
                  {
                     var groupList = domainInfo['GroupList'] ;
                     alterDomainForm['inputList'][2]['valid']['list'] = [] ;  //初始化

                     $.each( groupNameList, function( index2, groupname ){
                        alterDomainForm['inputList'][2]['valid']['list'].push( {
                           'key': groupname,
                           'value': groupname,
                           'checked': groupList.indexOf( groupname ) >= 0
                        } ) ;
                     } ) ;

                     alterDomainForm['inputList'][1]['value'] = domainInfo['AutoSplit'] ;
                  }
               } ) ;
            }
            var alterDomainForm = {
               inputList: [
                  {
                     "name": "domainName",
                     "webName": $scope.autoLanguage( '域名' ),
                     "type": "select",
                     "value": domainNameList[0]['value'],
                     "valid": domainNameList,
                     "onChange": function( name, key, value ){
                        setSelectGroup( alterDomainForm, value ) ;
                     }
                  },
                  {
                     "name": "autoSplit",
                     "webName": $scope.autoLanguage( '自动切分' ),
                     "type": "select",
                     "value": true,
                     "valid": [
                        { "key": "true", "value": true },
                        { "key": "false", "value": false }   
                     ]
                  },
                  {
                     "name": "groupList",
                     "webName": $scope.autoLanguage( '选择分区组' ),
                     "type": "multiple",
                     "value": [],
                     "valid": {
                        'min': 0,
                        'list': []
                     }
                  }
               ]
            } ;
            setSelectGroup( alterDomainForm, domainNameList[0]['value'] ) ;
            $scope.AlterDomain['config'] = alterDomainForm ;
            //设置确定按钮
            $scope.AlterDomain['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = alterDomainForm.check() ;
               if( isAllClear )
               {
                  var formVal = alterDomainForm.getValue() ;
                  //调用编辑domain函数
                  alterDomain( formVal['domainName'], formVal['autoSplit'], formVal['groupList'] ) ;
                  //关闭窗口
                  $scope.AlterDomain['callback']['Close']() ;
               }
            } ) ;
            //设置标题
            $scope.AlterDomain['callback']['SetTitle']( $scope.autoLanguage( '编辑域' ) ) ;
            //设置图标
            $scope.AlterDomain['callback']['SetIcon']( 'fa-plus' ) ;
            //打开窗口
            $scope.AlterDomain['callback']['Open']() ;
         }
      }

      //显示域信息弹窗
      $scope.ShowDomain = function( name ){
         var domainInfo = [] ;
         var groupList = [] ;
         $.each( domainList, function( index, info ){
            if( info['Name'] == name )
            {
               $scope.DomainInfo['config'] = info ;
               $scope.DomainInfo['callback']['SetTitle']( $scope.autoLanguage( '域信息' ) ) ;
               $scope.DomainInfo['callback']['SetIcon']('') ;
               $scope.DomainInfo['callback']['Open']() ;
            }
         } ) ;
      }

      //打开 显示列 下拉菜单
      $scope.OpenShowFieldDropdown = function( event ){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.FieldDropdown['config'][index]['show'] = typeof( $scope.DomainTable['title'][fieldInfo['key']] ) == 'string' ;
         } ) ;
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存显示列
      $scope.SaveField = function(){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.DomainTable['title'][fieldInfo['key']] = fieldInfo['show'] ? fieldInfo['key'] : false ;
         } ) ;
         $scope.DomainTable['callback']['ShowCurrentPage']() ;
      }


      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHostList = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      //跳转至节点列表
      $scope.GotoNodeList = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      } ;

   } ) ;
}());