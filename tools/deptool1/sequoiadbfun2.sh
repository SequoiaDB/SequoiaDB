#!/bin/bash

#校验步骤1的函数

#检查端口是否被占用
#参数1 端口号 例如 "50000"
function checkLocalPort()
{
   portNum=0
   portNum=`netstat -tln|grep "\<$1\>"| wc -l`
   if [ ${portNum} -eq 0 ]; then
      return 0
   else
      return 1
   fi
}

#检查路径是否存在 返回1是不存在
#参数1 路径 例如 "/opt/sequoiadb/"
function checkPathExist()
{
   if [ ! -d "$1" ]; then
      return 1
   else
      return 0
   fi
}

#创建文件夹
#参数1 文件夹名 例如 "myfile"
function createFolder()
{
   mkdir -p $1
}

#执行指定用户权限的命令
#参数1 用户名   例如 sdbadmin
#参数2 执行命令 例如 "mkdir aa"
function userExec()
{
   su - $1 -c "$2"
}

#创建指定用户权限的文件夹
#参数1 用户名   例如 sdbadmin
#参数2 文件夹名 例如 "/opt/sequoiadb/"
function userCreateFolder()
{
   userExec $1 "mkdir -p $2"
}

#判断指定目录可用空间是否达到指定大小(MB)
#参数1 路径 例如 "/opt/sequoiadb/"
#参数2 大小 例如 256
function checkAvailable()
{
   available=`df -m $1 | tail -n1|awk '{print $4}'`
   if [ ${available} -lt $2 ]; then
      return 1
   else
      return 0
   fi
}

#判断文件是否存在 返回1:存在 0:不存在
#参数1 文件名 例如 "/opt/sequoiadb/bin/sequoiadb"
function checkFileisExist()
{
   if [ ! -f "${1}" ]; then
      return 0
   else
      return 1
   fi
}

#判断目录是否为空 返回1 空目录 0非空
#参数1 路径 例如 "/opt/sequoiadb"
function checkFileNull()
{
   if [ "`ls -A $1`" = "" ]; then
      return 1
   else
      return 0
   fi
}

#运行安装文件
#参数1 文件名   例如 "sequoiadb.run"
#参数2 安装路径 例如 "/opt/sequoiadb"
#参数3 用户名   例如 "sdbadmin"
#参数4 密码     例如 "sequoiadb"
#参数5 端口     例如 "50010"
function install()
{
   /tmp/$1 --mode unattended --prefix $2 --username $3 --userpasswd $4 --port $5
}

#检查用户组是否存在 返回0 存在 返回1 不存在
#参数1 用户组名 例如 "root"
function checkGroup()
{
   groupNum=0
   groupNum=`cat /etc/group | grep "\<$1\>"|wc -l`
   if [ ${groupNum} -ne 0 ]; then
      return 0
   else
      return 1
   fi
}

#检查用户是否存在 返回0 存在 返回1 不存在
#参数1 用户名 例如 "root"
function checkUser()
{
   userNum=0
   userNum=`cat /etc/passwd | grep "\<$1\>"|wc -l`
   if [ ${userNum} -ne 0 ]; then
      return 0
   else
      return 1
   fi
}

#替换指定文件的字符串
#参数1 文件名
#参数2 查找的字符串
#参数3 替换的字符串
function replaceStr()
{
   eval "sed -i '/${2}/c\\${3}' ${1}"
}

#步骤1 子系统检查本地环境
#参数1 主机列表元素名 例如：ARRAY_HOST_1
function checkLocalEnv()
{
   list_host_name="${1}"

   #获取本地hostname
   thisHost=`hostname`

   #取得主机信息
   eval "host_array=(\"\${${list_host_name}[@]}\")"

   target="${host_array[0]}"

   #以下是主机检查

   #如果用的不是端口而是服务名，要做映射(未做)

   #检查sdbcm用的端口在本地是否被占用
   checkLocalPort "${SDBCM_PORT}"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} sdbcm ${SDBCM_PORT} port is already in use"
      return 1
   fi

   #检查用户组是否存在
   checkGroup "${host_array[2]}"
   if [ $? -ne 0 ]; then
      #用户组不存在,那么创建
      groupadd "${host_array[2]}"
   fi

   #检查用户是否存在
   checkUser "${host_array[3]}"
   if [ $? -ne 0 ]; then
      #用户不存在,那么创建
      useradd -d "${host_array[1]}" -g "${host_array[2]}" -p "${host_array[4]}" -s "/bin/bash" "${host_array[3]}"
   else
      #用户存在，那么增加他的附加组
      usermod -G "${host_array[2]}" -s "/bin/bash" "${host_array[3]}"
   fi

   #检查安装路径是否存在
   checkPathExist "${host_array[1]}"
   if [ $? -ne 1 ]; then
      #检查安装路径中是否空
      checkFileNull "${host_array[1]}"
      if [ $? -ne 1 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${host_array[1]} is not empty"
         return 1
      fi
   else
      #路径不存在，创建目录
      #创建目录
      createFolder ${host_array[1]}
   fi

   #修改安装路径所属用户和用户组
   chown ${host_array[3]}:${host_array[2]} -R "${host_array[1]}"

   #检查安装路径是否有足够大的空间
   checkAvailable "${host_array[1]}" ${INSTALL_SDB_SIZE}
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "The ${target} ${host_array[1]} directory available space is less than ${INSTALL_SDB_SIZE}MB"
      return 1
   fi

   #以下是该主机中的节点检查

   for array_name in ${LIST_NODE[@]}
   do
      eval "node_array=(\"\${${array_name}[@]}\")"
      #只取该主机的节点
      if [ "${list_host_name}" = "${node_array[2]}" ]; then
         #获取该节点的配置信息
         eval "conf_array=(\"\${${node_array[3]}[@]}\")"

         #检查端口

         #svcname
         num=`get_SDBCONF_num "svcname"`
         scvport=${conf_array[${num}]}
         checkLocalPort "${scvport}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} svcname ${scvport} port is already in use"
            return 1
         fi

         #replname
         num=`get_SDBCONF_num "replname"`
         port=${conf_array[${num}]}
         if [ -z "{port}" ]; then
            let "port=scvport+1"
         fi
         checkLocalPort "${port}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} replname ${port} port is already in use"
            return 1
         fi

         #shardname
         num=`get_SDBCONF_num "shardname"`
         port=${conf_array[${num}]}
         if [ -z "{port}" ]; then
            let "port=scvport+2"
         fi
         checkLocalPort "${port}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} shardname ${port} port is already in use"
            return 1
         fi

         #catalogname
         num=`get_SDBCONF_num "catalogname"`
         port=${conf_array[${num}]}
         if [ -z "{port}" ]; then
            let "port=scvport+3"
         fi
         checkLocalPort "${port}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} catalogname ${port} port is already in use"
            return 1
         fi

         #httpname
         num=`get_SDBCONF_num "httpname"`
         port=${conf_array[${num}]}
         if [ -z "{port}" ]; then
            let "port=scvport+4"
         fi
         checkLocalPort "${port}"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} httpname ${port} port is already in use"
            return 1
         fi

         #检查数据库存储路径

         #dbpath
         num=`get_SDBCONF_num "dbpath"`
         path=${conf_array[${num}]}
         if [ -z "${path}" ]; then
            #如果路径空，那么采用默认路径
            path="${host_array[1]}/bin"
         fi
         dbpath="${path}"
         #检查路径是否存在
         checkPathExist ${path}
         if [ $? -ne 1 ]; then
            #路径已经存在，检查路径目录内是否有文件
            checkFileNull ${path}
            if [ $? -ne 1 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            createFolder ${path}
         fi
         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"
         #检查路径是否有足够256MB的空间
         checkAvailable "${path}" "256"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "The ${target} ${path} directory available space is less than 256MB"
            return 1
         fi

         #indexpath
         num=`get_SDBCONF_num "indexpath"`
         path=${conf_array[${num}]}
         if [ -z "${path}" ]; then
            #如果路径空，那么采用默认路径
            path="${dbpath}"
         fi
         #检查路径是否存在
         checkPathExist ${path}
         if [ $? -ne 1 ]; then
            #路径已经存在，检查路径目录内是否有文件
            checkFileNull ${path}
            if [ $? -ne 1 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            createFolder ${path}
         fi

         #logpath
         num=`get_SDBCONF_num "logpath"`
         path=${conf_array[${num}]}
         if [ -z "${path}" ]; then
            #如果路径空，那么采用默认路径
            path="${dbpath}/replicalog"
         fi
         #检查路径是否存在
         checkPathExist ${path}
         if [ $? -ne 1 ]; then
            #路径已经存在，检查路径目录内是否有文件
            checkFileNull ${path}
            if [ $? -ne 1 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            createFolder ${path}
         fi
         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"
         #检查路径是否有足够256MB的空间
         checkAvailable "${path}" "256"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "The ${target} ${path} directory available space is less than 256MB"
            return 1
         fi

         #diagpath
         num=`get_SDBCONF_num "diagpath"`
         path=${conf_array[${num}]}
         if [ -z "${path}" ]; then
            #如果路径空，那么采用默认路径
            path="${dbpath}/diaglog"
         fi
         #检查路径是否存在
         checkPathExist ${path}
         if [ $? -ne 1 ]; then
            #路径已经存在，检查路径目录内是否有文件
            checkFileNull ${path}
            if [ $? -ne 1 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            createFolder ${path}
         fi
         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"
         #检查路径是否有足够256MB的空间
         checkAvailable "${path}" "256"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "The ${target} ${path} directory available space is less than 256MB"
            return 1
         fi

         #bkuppath
         num=`get_SDBCONF_num "bkuppath"`
         path=${conf_array[${num}]}
         if [ -z "${path}" ]; then
            #如果路径空，那么采用默认路径
            path="${dbpath}/bakfile"
         fi
         #检查路径是否存在
         checkPathExist ${path}
         if [ $? -ne 1 ]; then
            #路径已经存在，检查路径目录内是否有文件
            checkFileNull ${path}
            if [ $? -ne 1 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            createFolder ${path}
         fi
         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"
         #检查路径是否有足够256MB的空间
         checkAvailable "${path}" "256"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "The ${target} ${path} directory available space is less than 256MB"
            return 1
         fi

         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"
         #检查路径是否有足够256MB的空间
         checkAvailable "${path}" "256"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "The ${target} ${path} directory available space is less than 256MB"
            return 1
         fi
      fi
   done

   echo_r "Event" $FUNCNAME $LINENO "The ${target} all clear"
   return 0
}

#安装数据库文件
#参数1 节点列表的元素名
function installsdb()
{
   eval "host_array=(\"\${${1}[@]}\")"

   #检查安装路径
   checkPathExist "${host_array[1]}"
   if [ $? -ne 1 ]; then
      checkPathExist "${host_array[1]}/bin"
      if [ $? -ne 1 ]; then
         #路径已经存在，检查路径目录内是否有文件
         checkFileNull "${host_array[1]}/bin"
         if [ $? -ne 1 ]; then
            #如果已经有文件，证明已经安装了
            return 0
         fi
      fi
   fi

   #安装数据库
   install "${INSTALL_NAME}" "${host_array[1]}" "${host_array[3]}" "${host_array[4]}" "${SDBCM_PORT}"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to install SequoiaDB"
      return 1
   fi
}

#启动coord节点
#参数1 节点列表元素名
#参数2 是否第一次创建 0:是,1:否
function startCoord()
{
   eval "node_array=(\"\${${1}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   target="${host_array[0]}"

   num=`get_SDBCONF_num "svcname"`

   confpath="${host_array[1]}/conf/local/${node_conf[${num}]}"
   #检查配置文件路径是否存在
   checkPathExist ${confpath}
   if [ $? -ne 1 ]; then
      #路径已经存在，检查路径目录内是否有文件
      checkFileNull ${confpath}
      if [ $? -ne 1 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${confpath} is not empty"
         return 1
      fi
   else
      #路径不存在，创建目录
      #创建目录
      createFolder ${confpath}
   fi
   #修改所属用户 和 用户组
   chown ${host_array[3]}:${host_array[2]} -R "${confpath}"


   #复制出配置文件
   samconfpath="${host_array[1]}/conf/samples/sdb.conf.coord"
   confpath="${host_array[1]}/conf/local/${node_conf[${num}]}"
   cp "${samconfpath}" "${confpath}/sdb.conf"

   #修改配置文件
   coord_array=${node_conf}
   for((i=0;i<${#coord_array[@]};i++))
   do
      if [ -n "${coord_array[${i}]}" ]; then
         replaceStr "${confpath}/sdb.conf" "${SDB_CONFIG[${i}]}=" "${SDB_CONFIG[${i}]}=${coord_array[${i}]}"
      fi
   done

   #如果不是首个coord
   if [ ${2} -ne 0 ]; then
      catalogaddr=""
      thisHost=""
      isfirst=1
      #那么收集catalog信息
      for array_name in ${LIST_NODE[@]}
      do
         eval "tempchild=(\"\${${array_name}[@]}\")"
         if [ "${tempchild[0]}" = "cata" ]; then
            eval "temphost_arr=(\"\${${tempchild[2]}[@]}\")"
            eval "tempnode_conf=(\"\${${tempchild[3]}[@]}\")"
            #取得catalog的hostname
            thisHost=${temphost_arr[0]}
            if [ ${isfirst} -eq 1 ]; then
               isfirst=0
            else
               catalogaddr="${catalogaddr},"
            fi
            #取得catalog的通讯端口
            thisport=""
            num=`get_SDBCONF_num "catalogname"`
            thisport=${tempnode_conf[${num}]}
            if [ -z "${thisport}" ]; then
               #如果没有，那么获取svc端口
               num=`get_SDBCONF_num "svcname"`
               thisport=${tempnode_conf[${num}]}
               if [ -n "${thisport}" ]; then
                  #取得svc端口，默认+3就是catalog通讯端口
                  let "thisport=thisport+3"
               else
                  #都获取失败，那么使用默认50003端口
                  thisport="50003"
               fi
            fi
            catalogaddr="${catalogaddr}${thisHost}:${thisport}"
         fi
      done
      #写入到coord配置文件中
      if [ -n "${catalogaddr}" ]; then
         replaceStr "${confpath}/sdb.conf" "catalogaddr=" "catalogaddr=${catalogaddr}"
      fi
   fi

   #启动节点
   userExec "${host_array[3]}" "${host_array[1]}/bin/sdbstart -c ${confpath}"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to start coord"
      return 1
   else
      num=`get_SDBCONF_num "svcname"`
      echo_r "Event" $FUNCNAME $LINENO "${target} ${node_conf[${num}]} coord is start"
      return 0
   fi
}

#通过sdb启动data
#参数1 节点列表的元素名
#参数2 IP或hostname
#参数3 是否第一次创建 0:是,1:否
#参数4 coord的地址
#参数5 coord的端口
#参数6 sdb的路径
function startData()
{
   eval "node_array=(\"\${${1}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   target="${host_array[0]}"

   isfirst=1
   jsonconf=""
   data_array=${node_conf}
   for((i=0;i<${#data_array[@]};i++))
   do
      if [ -n "${data_array[${i}]}" ]; then
         if [ ${isfirst} -eq 1 ]; then
            isfirst=0
            jsonconf="{"
         else
            jsonconf="${jsonconf},"
         fi
         jsonconf="${jsonconf}${SDB_CONFIG[${i}]}:\"${data_array[${i}]}\""
      fi
   done
   if [ -n "${jsonconf}" ]; then
      jsonconf=${jsonconf}"}"
   fi

   #sdb连接coord
   rc=0
   ${6} "var db = new Sdb('${4}','${5}')"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to connect data"
      return 1
   fi
   if [ ${3} -eq 0 ]; then
      #如果是首次创建数据节点，那么先遍历创建分区组
      for group_name in ${LIST_GROUP[@]}
      do
         ${6} "db.createRG('${group_name}')"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create group ${group_name}"
            return 1
         fi
      done
   fi

   #选择分区组
   ${6} "var datarg = db.getRG('${node_array[1]}')"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to get group ${node_array[1]}"
      return 1
   fi

   num=`get_SDBCONF_num "svcname"`
   data_port="${node_conf[${num}]}"
   if [ -z "${data_port}" ]; then
      data_port="50000"
   fi
   num=`get_SDBCONF_num "dbpath"`
   data_path="${node_conf[${num}]}"
   if [ -z "${data_path}" ]; then
      data_path="${host_array[1]}/bin"
   fi

   #创建数据节点
   if [ -n "${jsonconf}" ]; then
      ${6} "datarg.createNode('`hostname`','${data_port}','${data_path}','${jsonconf}')"
   else
      ${6} "datarg.createNode('`hostname`','${data_port}','${data_path}')"
   fi
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create data"
      return 1
   fi
   echo_r "Event" $FUNCNAME $LINENO "${target} ${data_port} data is create"
   return 0
}

#通过sdb启动catalog
#参数1 节点列表的元素名
#参数2 IP或hostname
#参数3 是否第一次创建 0:是,1:否
#参数4 coord的地址
#参数5 coord的端口
#参数6 sdb的路径
function startCata()
{
   eval "node_array=(\"\${${1}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   target="${host_array[0]}"

   isfirst=1
   jsonconf=""
   coord_array=${node_conf}
   for((i=0;i<${#coord_array[@]};i++))
   do
      if [ -n "${coord_array[${i}]}" ]; then
         if [ ${isfirst} -eq 1 ]; then
            isfirst=0
            jsonconf="{"
         else
            jsonconf="${jsonconf},"
         fi
         jsonconf="${jsonconf}${SDB_CONFIG[${i}]}:\"${coord_array[${i}]}\""
      fi
   done
   if [ -n "${jsonconf}" ]; then
      jsonconf=${jsonconf}"}"
      #echo "cata配置： ${jsonconf}"
   fi

   #sdb连接coord
   rc=0
   ${6} "var db = new Sdb('${4}','${5}')"
   rc=$?
   if [ ${rc} -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to connect coord, rc=${rc}"
      return 1
   fi

   isstart1=`${6} "db.listReplicaGroups()"`

   localhostname=`hostname`
   num=`get_SDBCONF_num "svcname"`
   cata_port="${node_conf[${num}]}"
   if [ -z "${cata_port}" ]; then
      cata_port="50000"
   fi
   num=`get_SDBCONF_num "dbpath"`
   cata_path="${node_conf[${num}]}"
   if [ -z "${cata_path}" ]; then
      cata_path="${host_array[1]}/bin"
   fi
   if [ ${3} -eq 0 ]; then
      if [ -n "${jsonconf}" ]; then
         ${6} "db.createCataRG('${localhostname}','${cata_port}','${cata_path}','${jsonconf}')"
      else
         ${6} "db.createCataRG('${localhostname}','${cata_port}','${cata_path}')"
      fi
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create catalog"
         return 1
      fi
   else
      ${6} "var catarg = db.getRG('SYSCatalogGroup')"
      if [ -n "${jsonconf}" ]; then
         ${6} "var node=catarg.createNode('${localhostname}','${cata_port}','${cata_path}','${jsonconf}')"
      else
         ${6} "var node=catarg.createNode('${localhostname}','${cata_port}','${cata_path}')"
      fi
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create catalog node"
         return 1
      fi
      ${6} "node.start()"
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} Failed to start catalog node"
         return 1
      fi
   fi

   isstart2=${isstart1}
   while [ "${isstart2}" = "${isstart1}" ]
   do
      sleep 30
      isstart2=`${6} "db.listReplicaGroups()"`
   done
   echo_r "Event" $FUNCNAME $LINENO "${target} ${cata_port} catalog is start"
   return 0
}

#启动节点
#参数1 是否第一次创建 0:是,1:否
#参数2 coord的地址
#参数3 coord的端口
#参数4 节点列表元素名
function SDBstart()
{
   #获取本地IP和hostname
   thisIP=`ifconfig|grep "inet addr"|awk '{print $2}'|cut -d ":" -f 2|grep -v "127.0.0.1"`
   thisHost=`hostname`
   #找出属于本机的配置记录
   child=""
   target=""
   eval "node_array=(\"\${${4}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   target="${host_array[0]}"

   if [ ${node_array[0]} = "coord" ]; then
      startCoord "${4}" "${1}"
      if [ $? -ne 0 ]; then
         return 1
      fi
   elif [ ${node_array[0]} = "cata" ]; then
      startCata "${4}" "${target}" "${1}" "${2}" "${3}" "${host_array[1]}/bin/sdb"
      if [ $? -ne 0 ]; then
         return 1
      fi
   elif [ ${node_array[0]} = "data" ]; then
      startData "${4}" "${target}" "${1}" "${2}" "${3}" "${host_array[1]}/bin/sdb"
      if [ $? -ne 0 ]; then
         return 1
      fi
   fi
}

#获取配置参数属于第几个
#参数1 配置参数的名字
function get_SDBCONF_num()
{
   num=0
   for array_name in ${SDB_CONFIG[@]}
   do
      if [ "${1}" = "${array_name}" ]; then
         echo "${num}"
         break
      fi
      let "num+=1"
   done
}


