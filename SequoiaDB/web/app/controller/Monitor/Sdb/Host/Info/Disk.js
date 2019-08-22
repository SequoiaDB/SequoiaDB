//@ sourceURL=Disk.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Performance.Disk.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( hostName == null )
      {
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      //是否第一次
      var isFirstBuild = true ;
      //主机状态
      $scope.HostStatus = null ;
      $scope.hostName = hostName ;
      $scope.DiskList = [] ;

      //获取磁盘性能
      var getDiskProperty = function( diskList ){
         var data = {
            'cmd':'query host status',
            'HostInfo': JSON.stringify( { "HostInfo": [ { 'HostName': hostName } ] } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 && hostList[0]['HostInfo'].length > 0 )
               {
                  $.each( diskList, function( index, diskInfo ){
                  
                     if( isNaN( hostList[0]['HostInfo'][0]['errno'] ) == false && hostList[0]['HostInfo'][0]['errno'] != 0 )
                     {
                        $scope.HostStatus = false ;
                        return false ;
                     }
                     $scope.HostStatus = true ;

                     $.each( hostList[0]['HostInfo'][0]['Disk'], function( index2, diskInfo2 ){ //这是动态数据
                        if( diskInfo['Name'] == diskInfo2['Name'] )
                        {
                           var readSpeed  = 0 ;
                           var writeSpeed = 0 ;
                           diskInfo2['Used'] = diskInfo2['Size'] - diskInfo2['Free'] ;
                           diskList[index]['Used'] = fixedNumber( diskInfo2['Used'] / 1024, 2 ) ;
                           diskList[index]['Size'] = fixedNumber( diskInfo2['Size'] / 1024, 2 ) ;
                           diskList[index]['Free'] = fixedNumber( diskInfo2['Free'] / 1024, 2 ) ;

                           diskList[index]['StorageChart']['value'] = [
                              [ 0, fixedNumber( diskList[index]['Used'] / diskList[index]['Size'] * 100, 2 ), true, false ]
                           ] ;

                           if( isFirstBuild == false )
                           {
                              readSpeed  = diskInfo2['ReadSec']  - diskList[index]['ReadSec'] ;
                              writeSpeed = diskInfo2['WriteSec'] - diskList[index]['WriteSec'] ;
                              readSpeed  = fixedNumber( readSpeed  / 1024 / 1024 / 5 * 512, 2 ) ;
                              writeSpeed = fixedNumber( writeSpeed / 1024 / 1024 / 5 * 512, 2 ) ;
                           }

                           //磁盘扇区
                           diskList[index]['ReadSec'] = diskInfo2['ReadSec'] ;
                           diskList[index]['WriteSec'] = diskInfo2['WriteSec'] ;

                           diskList[index]['IOChart']['value'] = [
                              [ 0, readSpeed,  true, false ],
                              [ 1, writeSpeed, true, false ]
                           ] ;

                           isFirstBuild = false ;

                           return false ;
                        }
                     } ) ;
                  } ) ;
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      //获取主机磁盘信息
      var getDiskInfo = function(){
         var data = {
            'cmd':'query host',
            'filter': JSON.stringify( { 'HostName': hostName } ),
            'selector': JSON.stringify( { 'Disk': [] } )
         } ;
         //查询磁盘信息
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 )
               {
                  var diskList = [] ;
                  $.each( hostList[0]['Disk'], function( index, diskInfo ){
                     diskList.push( {
                        'Name': diskInfo['Name'],
                        'Used': '-',
                        'Size': diskInfo['Size'],
                        'Free': '-',
                        'Type': '-',
                        'Cache': '-',
                        'Path':diskInfo['Mount'],
                        'NodeName': '-',
                        'StorageChart': { 'options': window.SdbSacManagerConf.DiskStorageEchart },
                        'IOChart': { 'options': window.SdbSacManagerConf.DiskIOEchart }
                     } ) ;
                     diskList[index]['StorageChart']['options']['title']['text'] = $scope.autoLanguage( '磁盘利用率' ) ;
                     diskList[index]['IOChart']['options']['title']['text'] = $scope.autoLanguage( '读取/写入' ) ;
                  } ) ;
                  if( diskList.length > 0 )
                     getDiskProperty( diskList ) ;
                  $scope.DiskList = diskList ;
               }
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDiskInfo() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getDiskInfo() ;

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点
      $scope.GotoNodes = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;
   } ) ;

}());