//@ sourceURL=CPU.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Performance.CPU.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( hostName == null )
      {
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.ModuleName = moduleName ;
      $scope.HostName = hostName ;
      //主机状态
      $scope.HostStatus = null ;
      $scope.CpuList = [] ;
      var chartDetail = {} ;
      var sumCpuOld = 0 ;
      var sumCpuOld1 = 0 ;
      var sumCpuOld2 = 0 ;
      var idleCpuOld = 0 ;
      var sumCpu = 0 ;
      var idleCpu = 0 ;
      //获取cpu配置
      var getCpuInfo = function(){
         var data = {
            'cmd': 'query host',
            'filter': JSON.stringify( { 'HostName': hostName } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostInfo ){
               if( hostInfo.length > 0 )
               {
                  $.each( hostInfo[0]['CPU'], function( index, cpuInfo ){
                     if( typeof( cpuInfo['Freq'] ) != 'string' )
                     {
                        cpuInfo['Freq'] = '-' ;
                     }
                     else
                     {
                        var freqNum = parseFloat( cpuInfo['Freq'].substr(0,4) ) ;
                        cpuInfo['Freq'] =
                           ( typeof( freqNum ) == 'number' && !isNaN( freqNum ) ) ? freqNum : '-' ;
                     }

                     //数据暂无，待补充
                     cpuInfo['L1Cache'] = '-' ;
                     cpuInfo['L2Cache'] = '-' ;
                     cpuInfo['L3Cache'] = '-' ;
                     //逻辑处理器
                     cpuInfo['Processor'] = '-' ;
                     //进程数
                     $scope.CpuList.push( cpuInfo ) ;
                  } ) ;
               }
               else
               {
                  getCpuInfo() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getCpuInfo() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      } ;

      var getChartInfo = function(){
         var data = {
            'cmd':'query host status',
            'HostInfo': JSON.stringify( { "HostInfo": [ { 'HostName': hostName } ] } )
         } ;
         var lastCPU = null ;
         SdbRest.OmOperation( data, {
            'success': function( chartInfo ){
               if( chartInfo.length > 0 )
               {
                  if( chartInfo[0]['HostInfo'].length > 0 )
                  {
                     hostInfo = chartInfo[0]['HostInfo'][0] ;

                     if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
                     {
                        $scope.HostStatus = false ;
                        return ;
                     }
                     $scope.HostStatus = true ;

                     //计算CPU
                     if( lastCPU === null )
                     {
                        lastCPU = getCpuUsePercent( [ hostInfo ] ) ;
                        $scope.charts['Cpu']['value'] = [ [ 0, 0, true, false ] ] ;
                     }
                     else
                     {
                        $scope.charts['Cpu']['value'] = [ [ 0, getCpuUsePercent( [ hostInfo ], lastCPU ), true, false ] ] ;
                     }

                  }
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      getCpuInfo() ;
      getChartInfo() ;

      $scope.charts = {}; 
      $scope.charts['Cpu'] = {} ;
      $scope.charts['Cpu']['options'] = window.SdbSacManagerConf.CpuEchart ;
      $scope.charts['Cpu']['options']['title']['text'] = $scope.autoLanguage( 'CPU利用率' ) ;

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      //跳转至节点列表
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
