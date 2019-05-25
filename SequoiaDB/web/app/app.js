(function () {
   window.SdbSacManagerConf = {};
   //模块
   window.SdbSacManagerModule = angular.module('sacApp', ['ngRoute'], function ($rootScopeProvider) {
      $rootScopeProvider.digestTtl(1000);
   });

   window.Config = {
      recv: false,   //还没同步数据
      Edition: 'Community',
      Version: {
      },
      Controller: {
         sequoiadb: {
            Metadata: true,
            Data: true
         },
         sequoiasql: {
         }
      }
   };
   window.SdbDebug = false;
}());