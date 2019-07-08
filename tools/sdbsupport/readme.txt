【sdbsupport工具说明】


    功能：sdbsupport工具用于收集SequoiaDB数据库的日志信息,配置信息，数据库集群所在环境的硬件信息以及操作系统信息。当SequoiaDB在运行的时候，还能够获取snapshot快照的信息。


一、注意事项
    1、使用sdbsupport工具前，最好执行一下./sdbsupport.sh --help查看相关说明
    2、sdbsupport工具参数分为四大类：日志、快照、硬件信息、操作系统;四大类下又划分成为小类；
   日志(-p [--svcport]):此处直接带参数，如：50000：30000
   快照(-n [--snapshot]):--catalog --group --context --session --collection --collectionspace --database --system
   硬件信息(-h [--hardware]):--cpu --memory --disk --netcard --mainboard
   操作系统(-o [--osinfo]):--diskmanage --basicsys --module --env --network --process --login --limit --vmstat
    3、当收集非本机的信息时，要输入密码，务必要一次成功，否则将会导致日志收集失败。



二、使用指南
    1、使用sdbsupport工具
       >./sdbsupport.sh --help   "参数--help:显示sdbsupport工具的各类参数信息"
       >./sdbsupport.sh          "不带参数:收集本机的所有信息[日志信息/快照信息/硬件信息/操作系统信息]"
       >./sdbsupport.sh --all    "参数--all:收集集群所有主机的所有信息[日志信息/快照信息/硬件信息/操作系统信息]"
       >./sdbsupport.sh -s hostname1   "参数-s:收集主机hostname1的所有信息"

       >./sdbsupport.sh -s hostname1:hostname2 -p 50000:30000 --snapshot  --hardware --osinfo
               "带如此参数收集主机：hostname1和hostname2的快照、硬件信息、操作系统信息及他们的50000和30000端口日志"

       >./sdbsupport.sh -s hostname1:hostname2 -p 50000:30000 --group --memory --basicsys
               "带此参数收集主机hostname1和hostname2的50000/30000端口日志、group快照、硬件信息中的内存信息[--memory]以及操作系统的基本信息[--basicsys]"

