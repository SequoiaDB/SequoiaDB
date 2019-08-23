(function(){
   window.SdbSacManagerConf.nowRoute = [
      { path: '/Transfer',
        options: {
           templateUrl: './app/template/Public/Transfer.html',
           resolve: resolveFun( [ './app/controller/Transfer.js' ] )
        }
      },
      //废弃
      /*
      { path: '/Index/Index/Index',
        options: {
           templateUrl: './app/template/Index/Index/Index.html',
           resolve: resolveFun( [ './app/controller/Index/Index/Index.js' ] )
        }
      },
      { path: '/Data/SDB-Overview/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Overview/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Overview/Index.js', './app/controller/Data/Sdb/Overview/Index.js' ] )
        }
      },
      { path: '/Data/SDB-Operate/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Operate/Index.js', './app/controller/Data/Sdb/Operate/Index.js' ] )
        }
      },
      */
      { path: '/Data/SDB-Operate/Record',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Record.html',
           resolve: resolveFun( [ './app/controller/Data/Sdb/Operate/Record.js' ] )
        }
      },
      { path: '/Data/SDB-Operate/Lobs',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Lobs.html',
           resolve: resolveFun( [ './app/controller/Data/Sdb/Operate/Lobs.js' ] )
        }
      },
      { path: '/Data/SDB-Database/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Database/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Database/Index.js', './app/controller/Data/Sdb/Database/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Overview/Index',
        options: {
           templateUrl: './app/template/Data/Sql/Overview/Index.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Overview/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Metadata/Index',
        options: {
           templateUrl: './app/template/Data/Sql/Metadata/Index.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Metadata/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Operate/Data',
        options: {
           templateUrl: './app/template/Data/Sql/Operate/Data.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Operate/Data.js' ] )
        }
      },
      { path: '/Data/SQL-Operate/Structure',
        options: {
           templateUrl: './app/template/Data/Sql/Operate/Structure.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Operate/Structure.js' ] )
        }
      },
      { path: '/Data/HDFS-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/SPARK-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/YARN-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/NotSupport',
        options: {
           templateUrl: './app/template/Data/Other/notsupport.html',
           resolve: resolveFun( [ './app/controller/Data/Other/notsupport.js' ] )
        }
      },
      { path: '/Data/Edition',
        options: {
           templateUrl: './app/template/Data/Other/edition.html',
           resolve: resolveFun( [ './app/controller/Data/Other/edition.js' ] )
        }
      },
      { path: '/Strategy/SDB/Index',
        options: {
           templateUrl: './app/template/Strategy/Sdb/Index.html',
           resolve: resolveFun( [ './app/controller/Strategy/Sdb/Index.js' ] )
        }
      },
      { path: '/Strategy/SDB/Strategy',
        options: {
           templateUrl: './app/template/Strategy/Sdb/Strategy.html',
           resolve: resolveFun( [ './app/controller/Strategy/Sdb/Strategy.js' ] )
        }
      },
      //监控主页
      { path: '/Monitor/SDB/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Index.js' ] )
         }
      },
      //SDB节点类
      { path: '/Monitor/SDB-Nodes/Groups',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/GroupList.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/GroupList.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Nodes',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/NodeList.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/NodeList.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Charts.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/GroupsSnapshot',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/GroupsSnapshot.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/GroupsSnapshot.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/NodesSnapshot',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/NodesSnapshot.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/NodesSnapshot.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/NodesSync',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/NodesSync.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/NodesSync.js' ] )
         }
      },
      //节点-分区组
      { path: '/Monitor/SDB-Nodes/Group/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Group/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Group/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Group/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Group/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Group/Charts.js' ] )
         }
      },
      //节点-节点
      { path: '/Monitor/SDB-Nodes/Node/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Node/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Node/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Node/Session',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Node/Session.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Node/Session.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Node/Context',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Node/Context.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Node/Context.js' ] )
         }
      },
      { path: '/Monitor/SDB-Nodes/Node/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Node/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Node/Charts.js' ] )
         }
      },
      //SDB资源类
      { path: '/Monitor/SDB-Resources/Domain',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/Domain.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/Domain.js' ] )
         }
      },
      { path: '/Monitor/SDB-Resources/Session',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/Session.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/Session.js' ] )
         }
      },
      { path: '/Monitor/SDB-Resources/Context',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/Context.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/Context.js' ] )
         }
      },
      { path: '/Monitor/SDB-Resources/Procedure',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/Procedure.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/Procedure.js', './scripts/jsformat/jsformat.js' ] )
         }
      },
      { path: '/Monitor/SDB-Resources/Transaction',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/Transaction.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/Transaction.js' ] )
         }
      },
      { path: '/Monitor/SDB-Resources/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Resource/ResourceCharts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Resource/ResourceCharts.js' ] )
         }
      },
      //监控-主机列表
      { path: '/Monitor/SDB-Host/List/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/List/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/List/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/List/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/List/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/List/Charts.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/List/HostsSnapshot',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/List/HostsSnapshot.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/List/HostsSnapshot.js' ] )
         }
      },
      //监控-单主机
      { path: '/Monitor/SDB-Host/Info/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/Info/Disk',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/Disk.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/Disk.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/Info/CPU',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/CPU.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/CPU.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/Info/Memory',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/Memory.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/Memory.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/Info/Network',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/Network.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/Network.js' ] )
         }
      },
      { path: '/Monitor/SDB-Host/Info/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Host/Info/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Host/Info/Charts.js' ] )
         }
      },
      //SQL监控页
      { path: '/Monitor/SQL-Module/Index',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Preview/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Preview/Index.js' ] )
         }
      },
      { path: '/Monitor/SQL-Module/Performance',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Performance/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Performance/Index.js' ] )
         }
      },
      { path: '/Monitor/SQL-Module/Analyze',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Analyze/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Analyze/Index.js' ] )
         }
      },
      //监控-社区版提示页面
      { path: '/Monitor/Preview',
        options: {
           templateUrl: './app/template/Monitor/other/Preview.html',
           resolve: resolveFun( [ './app/controller/Monitor/other/Preview.js' ] )
         }
      },
      //部署
      { path: '/Deploy/Index',
        options: {
           templateUrl: './app/template/Deploy/Index.html',
           resolve: resolveFun( [ './app/controller/Deploy/Index.js' ] )
        }
      },
      //部署主机
      { path: '/Deploy/ScanHost',
        options: {
           templateUrl: './app/template/Deploy/InstallHost/Scan.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallHost/Scan.js' ] )
        }
      },
      { path: '/Deploy/AddHost',
        options: {
           templateUrl: './app/template/Deploy/InstallHost/Add.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallHost/Add.js' ] )
        }
      },
      //部署业务
      { path: '/Deploy/SDB-Conf',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Sdb/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Sdb/Conf.js' ] )
        }
      },
      { path: '/Deploy/SDB-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Sdb/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Sdb/Mod.js' ] )
        }
      },
      { path: '/Deploy/InstallHost',
        options: {
           templateUrl: './app/template/Deploy/Task/Host.html',
           resolve: resolveFun( [ './app/controller/Deploy/Task/Host.js' ] )
        }
      },
      { path: '/Deploy/InstallModule',
        options: {
           templateUrl: './app/template/Deploy/Task/Module.html',
           resolve: resolveFun( [ './app/controller/Deploy/Task/Module.js' ] )
        }
      },
      { path: '/Deploy/SDB-ExtendConf',
        options: {
           templateUrl: './app/template/Deploy/ExtendModule/Sdb/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/ExtendModule/Sdb/Conf.js' ] )
        }
      },
      { path: '/Deploy/SDB-Extend',
        options: {
           templateUrl: './app/template/Deploy/ExtendModule/Sdb/Extend.html',
           resolve: resolveFun( [ './app/controller/Deploy/ExtendModule/Sdb/Extend.js' ] )
        }
      },
      { path: '/Deploy/SDB-ExtendInstall',
        options: {
           templateUrl: './app/template/Deploy/ExtendModule/Sdb/Install.html',
           resolve: resolveFun( [ './app/controller/Deploy/ExtendModule/Sdb/Install.js' ] )
        }
      },
      { path: '/Deploy/OLTP-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/OLTP/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/OLTP/Mod.js' ] )
        }
      },
      { path: '/Deploy/ZKP-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Zookeeper/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Zookeeper/Mod.js' ] )
        }
      },
      { path: '/Deploy/SSQL-Conf',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Ssql/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Ssql/Conf.js' ] )
        }
      },
      { path: '/Deploy/SSQL-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Ssql/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Ssql/Mod.js' ] )
        }
      },
      { path: '/Deploy/SDB-Discover',
        options: {
           templateUrl: './app/template/Deploy/DiscoverModule/Sdb/Index.html',
           resolve: resolveFun( [ './app/controller/Deploy/DiscoverModule/Sdb/Index.js' ] )
        }
      },
      { path: '/Deploy/SDB-Sync',
        options: {
           templateUrl: './app/template/Deploy/SyncModule/Sdb/Index.html',
           resolve: resolveFun( [ './app/controller/Deploy/SyncModule/Sdb/Index.js' ] )
        }
      },
      { path: '/Deploy/SDB-ShrinkConf',
        options: {
           templateUrl: './app/template/Deploy/ShrinkModule/Sdb/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/ShrinkModule/Sdb/Conf.js' ] )
        }
      },
      { path: '/Deploy/Package',
        options: {
           templateUrl: './app/template/Deploy/DeployPackage/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/DeployPackage/Conf.js' ] )
        }
      }
   ] ;
   window.SdbSacManagerConf.defaultRoute = { redirectTo: '/Transfer' } ;
}());