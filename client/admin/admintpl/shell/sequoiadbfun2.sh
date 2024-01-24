#!/bin/bash

#输出删除列表
function echoDelList()
{
   for delPath in ${DELETE_PATH_ARR[@]}
   do
      echo "${delPath}" >> /tmp/sdbdel
   done
}

#输出撤销列表
function echoRevokeList()
{
   for tasks in ${REVOKE_TASK_ARR[@]}
   do
      echo "${tasks}" >> /tmp/sdbrevtask
   done
}

#遍历删除列表删除路径
function delListFile()
{
   local isfirst=1
   local localHostname=`hostname`
   while read delPath
   do
      if [ "${isfirst}" = "1" ]; then
         isfirst=0
      else
         #删除
         if [ -n "${delPath}" ]; then
            if [ -d "${delPath}" ]; then
               echo_r "Event" $FUNCNAME $LINENO "${localHostname} delete ${delPath}"
               rm -r ${delPath}
            fi
         fi
      fi
   done < /tmp/sdbdel
   echo "" > /tmp/sdbdel
}

#遍历撤销列表撤销任务
function revokeListTask()
{
   local isfirst=1
   local temp=""
   local localHostname=`hostname`
   while read tasks
   do
      if [ "${isfirst}" = "1" ]; then
         isfirst=0
      else
         if [ "${temp}" == "${tasks}" ]; then
            break
         fi
         temp="${tasks}"
         echo_r "Event" $FUNCNAME $LINENO "${localHostname} executive command ${tasks}"
         ${tasks}
      fi
   done < /tmp/sdbrevtask
   echo "" > /tmp/sdbrevtask
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

#校验步骤1的函数

#检查端口是否被占用
#参数1 端口号 例如 "50000"
function checkLocalPort()
{
   local portNum=0
   local portNum=`netstat -tln|grep "\<$1\>"| wc -l`
   if [ ${portNum} -eq 0 ]; then
      return 0
   else
      return 1
   fi
}

#检查是不是sdbcm使用了端口
#参数1 sdbcm的端口号
function checkSdbcmStart()
{
   local sdbcms=`ps -ef| grep "sdbcm(${1})"|grep -v '\<grep\>'|awk '{print $2}'`
   if [ -n "${sdbcms}" ]; then
      return 0
   else
      return 1
   fi
}

#检查是不是指定路径启动sdbcm
#参数1 路径
function checkSdbcmPath2()
{
   local sdbcms=`ps -ef| grep "\<sdbcm\>"|grep -v '\<grep\>'|awk '{print $2}'`
   local sdbcmPath=""
   if [ -n "${sdbcms}" ]; then
      sdbcmPath=`ls -l /proc/${sdbcms}/exe | awk '{print $11}'`
      if [ "${1}" = "${sdbcmPath}" ]; then
         return 0
      else
         return 1
      fi
   else
      return 0
   fi
}

#检查是不是指定路径启动sdbcm
#参数1 sdbcm端口
#参数2 路径
function checkSdbcmPath()
{
   local sdbcms=`ps -ef| grep "sdbcm(${1})"|grep -v '\<grep\>'|awk '{print $2}'`
   local sdbcmPath=""
   if [ -n "${sdbcms}" ]; then
      sdbcmPath=`ls -l /proc/${sdbcms}/exe | awk '{print $11}'`
      if [ "${2}" = "${sdbcmPath}" ]; then
         return 0
      else
         return 1
      fi
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

#创建文件夹 返回1是不存在
#参数1 文件夹名 例如 "myfile"
#参数2 用户名
#参数3 用户组
function createFolder2()
{
   local path="${1}"
   local user="${2}"
   local group="${3}"
   if [ -z "${path}" ]; then
      return 0
   fi
   checkPathExist "${path}"
   if [ "${?}" = "1" ]; then
      createFolder2 "${path%/*}" "${user}" "${group}"
      if [ "${?}" = "0" ]; then
         #如果上一层存在,那就从当前层创建路径
         createFolder "${path}"
         chown ${user}:${group} -R "${path}"
         return 0
      fi
      return 1
   else
      return 0
   fi
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
   userExec "${1}" "mkdir -p ${2}"
}

#判断指定目录可用空间是否达到指定大小(MB)
#参数1 路径 例如 "/opt/sequoiadb/"
#参数2 大小 例如 256
function checkAvailable()
{
   local available=`df -m ${1} | tail -n1|awk '{print $4}'`
   if [ "${available}" -lt "${2}" ]; then
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

#判断目录是否为空 返回 0空目录 1非空
#参数1 路径 例如 "/opt/sequoiadb"
function checkFileNull()
{
   local rootPath="${1}"
   local files=`find ${rootPath} -type f -print`
   
   if [ -z "${files}" ]; then
      return 0
   else
      return 1
   fi
}

#运行安装文件
#参数1 文件名   例如 "sequoiadb.run"
#参数2 安装路径 例如 "/opt/sequoiadb"
#参数3 用户名   例如 "sdbadmin"
#参数4 密码     例如 "sequoiadb"
#参数5 端口     例如 "50010"
#参数6 开机自动启动 例如 true
function install()
{
   local autostart="true"
   if [ $6 -eq 0 ]; then
      autostart="false"
   fi
   /tmp/$1 --mode unattended --prefix $2 --username $3 --userpasswd $4 --port $5 --processAutoStart ${autostart}
   if [ $? -ne 0 ]; then
      return 1
   else
      return 0
   fi
}

#检查用户组是否存在 返回0 存在 返回1 不存在
#参数1 用户组名 例如 "root"
function checkGroup()
{
   local groupNum=0
   local groupNum=`cat /etc/group | grep "\<$1\>"|wc -l`
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
   local userNum=0
   local userNum=`cat /etc/passwd | grep "\<$1\>"|wc -l`
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
#参数2 数据库版本
#参数3 安装后大小
function checkLocalEnv()
{
   local array_ele="${1}"
   local sdbVersion="${2}"
   local installSize="${3}"

   #获取本地hostname
   local thisHost=`hostname`

   #取得主机信息
   eval "host_array=(\"\${${array_ele}[@]}\")"

   local target="${host_array[0]}"

   #以下是主机检查

   #如果用的不是端口而是服务名，要做映射(未做)

   #检查sdbcm用的端口在本地是否被占用
   checkLocalPort "${SDBCM_PORT}"
   if [ $? -ne 0 ]; then
      #如果被占用，检查是不是sdbcm占用
      checkSdbcmStart "${SDBCM_PORT}"
      if [ $? -ne 0 ]; then
         #不是sdbcm占用
         echo_r "Error" $FUNCNAME $LINENO "${target} sdbcm ${SDBCM_PORT} port is already in use"
         return 1
      else
         checkSdbcmPath "${SDBCM_PORT}" "${host_array[1]}/bin/sdbcm"
         if [ $? -ne 0 ]; then
            #sdbcm启动了,但是不是指定安装路径下的sdbcm
            echo_r "Error" $FUNCNAME $LINENO "${target} sdbcm is not ${host_array[1]}/bin/sdbcm path of sdbcm"
            return 1
         fi
      fi
   else
      checkSdbcmPath2 "${host_array[1]}/bin/sdbcm"
      if [ $? -ne 0 ]; then
         #sdbcm启动了,但是不是指定安装路径下的sdbcm
         echo_r "Error" $FUNCNAME $LINENO "${target} sdbcm is not ${host_array[1]}/bin/sdbcm path of sdbcm"
         return 1
      fi
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
      #如果加-d命令,部分系统会自动创建路径,导致目录下有用户系统环境配置文件
      #useradd -d "${host_array[1]}" -g "${host_array[2]}" -p "${host_array[4]}" -s "/bin/bash" "${host_array[3]}"
      useradd -g "${host_array[2]}" -p "${host_array[4]}" -s "/bin/bash" "${host_array[3]}"
   else
      #用户存在，那么增加他的附加组
      usermod -G "${host_array[2]}" -s "/bin/bash" "${host_array[3]}"
   fi

   #检查安装路径是否存在
   checkPathExist "${host_array[1]}"
   if [ $? -ne 1 ]; then
      #检查安装路径中是否空
      checkFileNull "${host_array[1]}"
      if [ $? -ne 0 ]; then
         #获取本机数据库版本
         lVersion=`getLocalDBVersion`
         if [ "${2}" != "${lVersion}" ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${host_array[1]} is not empty"
            return 1
         fi
      fi
   else
      #路径不存在，创建目录
      #创建目录
      #createFolder "${host_array[1]}"
      createFolder2 "${host_array[1]}" "${host_array[3]}" "${host_array[2]}"
      DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${host_array[1]}")
   fi

   #修改安装路径所属用户和用户组
   chown ${host_array[3]}:${host_array[2]} -R "${host_array[1]}"

   #检查安装路径是否有足够大的空间
   checkAvailable "${host_array[1]}" "${installSize}"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "The ${target} ${host_array[1]} directory available space is less than ${INSTALL_SDB_SIZE}MB"
      return 1
   fi

   #以下是该主机中的节点检查

   for array_name in ${LIST_NODE[@]}
   do
      eval "node_array=(\"\${${array_name}[@]}\")"
      #只取该主机的节点
      if [ "${array_ele}" = "${node_array[2]}" ]; then
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
            if [ $? -ne 0 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            #createFolder "${path}"
            createFolder2 "${path}" "${host_array[3]}" "${host_array[2]}"
            DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${path}")
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
            if [ $? -ne 0 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            #createFolder ${path}
            createFolder2 "${path}" "${host_array[3]}" "${host_array[2]}"
            DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${path}")
         fi
         #修改所属用户 和 用户组
         chown ${host_array[3]}:${host_array[2]} -R "${path}"

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
            if [ $? -ne 0 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            #createFolder ${path}
            createFolder2 "${path}" "${host_array[3]}" "${host_array[2]}"
            DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${path}")
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
            if [ $? -ne 0 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            #createFolder ${path}
            createFolder2 "${path}" "${host_array[3]}" "${host_array[2]}"
            DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${path}")
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
            if [ $? -ne 0 ]; then
               echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${path} is not empty"
               return 1
            fi
         else
            #路径不存在，创建目录
            #创建目录
            #createFolder ${path}
            createFolder2 "${path}" "${host_array[3]}" "${host_array[2]}"
            DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${path}")
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
   
   return 0
}

#安装数据库文件
#参数1 节点列表的元素名
function installsdb()
{
   local array_ele="${1}"
   local sdbcmport=""
   local localHostname=`hostname`
   INSTALL_DIR=""
   eval "host_array=(\"\${${array_ele}[@]}\")"

   #检查安装路径
   checkPathExist "${host_array[1]}"
   if [ $? -ne 1 ]; then
      checkPathExist "${host_array[1]}/bin"
      if [ $? -ne 1 ]; then
         #路径已经存在，检查路径目录内是否有文件
         checkFileNull "${host_array[1]}/bin"
         if [ $? -ne 0 ]; then
            #如果已经有文件，证明已经安装了
            #如果安装了，那么要检查sdbcm配置，看看端口是否跟配置的一样
            sdbcmport=`grep -i "defaultPort=" ${host_array[1]}/conf/sdbcm.conf | awk '{print $1}'|cut -d "=" -f 2`
            if [ "${sdbcmport}" != "${SDBCM_PORT}" ]; then
               echo_r "Error" $FUNCNAME $LINENO "The ${localHostname} sdbcm.conf defaultPort and sdbconfig.sh SDBCM_PORT is not the same"
               return 1
            fi
            
            #然后检查sdbcm起了没
            checkSdbcmStart "${SDBCM_PORT}"
            if [ $? -ne 0 ]; then
               #没有起,那么启动sdbcm
               userExec "${host_array[3]}" "${host_array[1]}/bin/sdbcmart"
               if [ $? -ne 0 ]; then
                  #启动失败 报错
                  echo_r "Error" $FUNCNAME $LINENO "The ${localHostname} sdbcm start error"
                  return 1
               fi
               REVOKE_TASK_ARR=("${REVOKE_TASK_ARR[@]}" "${host_array[1]}/bin/sdbcmtop")
            fi
            return 0
         fi
      fi
   fi

   #安装数据库
   install "${INSTALL_NAME}" "${host_array[1]}" "${host_array[3]}" "${host_array[4]}" "${SDBCM_PORT}" "${IS_AUTOSTART}"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to install SequoiaDB"
      return 1
   fi
   REVOKE_TASK_ARR=("${REVOKE_TASK_ARR[@]}" "${host_array[1]}/bin/sdbcmtop")
   return 0
}

#启动coord节点
#参数1 节点列表元素名
#参数2 是否第一次创建 0:是,1:否
function startCoord()
{
   local array_ele="${1}"
   
   eval "node_array=(\"\${${array_ele}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   local target="${host_array[0]}"

   num=`get_SDBCONF_num "svcname"`

   local confpath="${host_array[1]}/conf/local/${node_conf[${num}]}"
   #检查配置文件路径是否存在
   checkPathExist ${confpath}
   if [ $? -ne 1 ]; then
      #路径已经存在
      echo_r "Error" $FUNCNAME $LINENO "${target} The installation path ${confpath} is exist"
   else
      #路径不存在，创建目录
      #创建目录
      #createFolder "${confpath}"
      createFolder2 "${confpath}" "${host_array[3]}" "${host_array[2]}"
      DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${confpath}")
   fi
   #修改所属用户 和 用户组
   chown ${host_array[3]}:${host_array[2]} -R "${confpath}"


   #复制出配置文件
   local samconfpath="${host_array[1]}/conf/samples/sdb.conf.coord"
   local confpath="${host_array[1]}/conf/local/${node_conf[${num}]}"
   cp "${samconfpath}" "${confpath}/sdb.conf"
   chown ${host_array[3]}:${host_array[2]} -R "${confpath}/sdb.conf"

   #修改配置文件
   for((i=0;i<${#node_conf[@]};i++))
   do
      if [ -n "${node_conf[${i}]}" ]; then
         replaceStr "${confpath}/sdb.conf" "${SDB_CONFIG[${i}]}=" "${SDB_CONFIG[${i}]}=${node_conf[${i}]}"
      fi
   done

   #如果不是首个coord
   if [ ${2} -ne 0 ]; then
      local catalogaddr=""
      local thisHost=""
      local isfirst=1
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
                  thisport="11813"
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

   local target="${host_array[0]}"

   local isfirst=1
   local jsonconf=""
   for((i=0;i<${#node_conf[@]};i++))
   do
      if [ -n "${node_conf[${i}]}" ]; then
         if [ ${isfirst} -eq 1 ]; then
            isfirst=0
            jsonconf="{"
         else
            jsonconf="${jsonconf},"
         fi
         jsonconf="${jsonconf}\"${SDB_CONFIG[${i}]}\":\"${node_conf[${i}]}\""
      fi
   done
   if [ -n "${jsonconf}" ]; then
      jsonconf="${jsonconf} }"
   fi

   #sdb连接coord
   rc=0
   ${6} "var db = new Sdb(\"${4}\",\"${5}\")"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to connect coord ${4} ${5}"
      return 1
   fi
   if [ ${3} -eq 0 ]; then
      #如果是首次创建数据节点，那么先遍历创建分区组
      for group_name in ${LIST_GROUP[@]}
      do
         ${6} "db.createRG(\"${group_name}\")"
         if [ $? -ne 0 ]; then
            echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create group ${group_name}"
            return 1
         fi
      done
   fi

   #选择分区组
   ${6} "var datarg = db.getRG(\"${node_array[1]}\")"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to get group ${node_array[1]}"
      return 1
   fi

   num=`get_SDBCONF_num "svcname"`
   data_port="${node_conf[${num}]}"
   if [ -z "${data_port}" ]; then
      data_port="11810"
   fi
   num=`get_SDBCONF_num "dbpath"`
   data_path="${node_conf[${num}]}"
   if [ -z "${data_path}" ]; then
      data_path="${host_array[1]}/bin"
   fi

   #创建数据节点
   if [ -n "${jsonconf}" ]; then
      ${6} "datarg.createNode(\"`hostname`\",\"${data_port}\",\"${data_path}\",${jsonconf})"
   else
      ${6} "datarg.createNode(\"`hostname`\",\"${data_port}\",\"${data_path}\")"
   fi
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create data"
      return 1
   fi
   DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${host_array[1]}/conf/local/${data_port}")

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

   local target="${host_array[0]}"

   local isfirst=1
   local jsonconf=""
   for((i=0;i<${#node_conf[@]};i++))
   do
      if [ -n "${node_conf[${i}]}" ]; then
         if [ ${isfirst} -eq 1 ]; then
            isfirst=0
            jsonconf="{"
         else
            jsonconf="${jsonconf},"
         fi
         jsonconf="${jsonconf}\"${SDB_CONFIG[${i}]}\":\"${node_conf[${i}]}\""
      fi
   done
   if [ -n "${jsonconf}" ]; then
      jsonconf="${jsonconf} }"
      echo "cata配置： ${jsonconf}"
   fi

   #sdb连接coord
   rc=0
   ${6} "var db = new Sdb(\"${4}\",\"${5}\")"
   if [ $? -ne 0 ]; then
      echo_r "Error" $FUNCNAME $LINENO "${target} Failed to connect coord ${4} ${5}"
      return 1
   fi

   local temp=""

   localhostname=`hostname`
   num=`get_SDBCONF_num "svcname"`
   cata_port="${node_conf[${num}]}"
   if [ -z "${cata_port}" ]; then
      cata_port="11810"
   fi
   num=`get_SDBCONF_num "dbpath"`
   cata_path="${node_conf[${num}]}"
   if [ -z "${cata_path}" ]; then
      cata_path="${host_array[1]}/bin"
   fi
   if [ ${3} -eq 0 ]; then
      if [ -n "${jsonconf}" ]; then
         ${6} "db.createCataRG(\"${localhostname}\",\"${cata_port}\",\"${cata_path}\",${jsonconf})"
      else
         ${6} "db.createCataRG(\"${localhostname}\",\"${cata_port}\",\"${cata_path}\")"
      fi
      if [ $? -ne 0 ]; then
         echo_r "Error" $FUNCNAME $LINENO "${target} Failed to create catalog"
         return 1
      fi
      while [ "1" = "1" ]
      do
         sleep 5
         temp=`${6} "db.listReplicaGroups()"`
         if [ $? -eq 0 ]; then
            break
         fi
      done
      sleep 10
   else
      ${6} "var catarg = db.getRG(\"SYSCatalogGroup\")"
      if [ -n "${jsonconf}" ]; then
         ${6} "var node=catarg.createNode(\"${localhostname}\",\"${cata_port}\",\"${cata_path}\",${jsonconf})"
      else
         ${6} "var node=catarg.createNode(\"${localhostname}\",\"${cata_port}\",\"${cata_path}\")"
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
   DELETE_PATH_ARR=("${DELETE_PATH_ARR[@]}" "${host_array[1]}/conf/local/${cata_port}")

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
   local target
   local node_array
   local host_array
   local node_conf
   local isfirst="${1}"
   local coordaddr="${2}"
   local coordport="${3}"
   local array_ele="${4}"

   eval "node_array=(\"\${${array_ele}[@]}\")"
   eval "host_array=(\"\${${node_array[2]}[@]}\")"
   eval "node_conf=(\"\${${node_array[3]}[@]}\")"

   target="${host_array[0]}"

   echo "test ${target} ${node_array[0]}"
   if [ ${node_array[0]} = "coord" ]; then
      startCoord "${array_ele}" "${isfirst}"
      if [ $? -ne 0 ]; then
         return 1
      fi
   elif [ ${node_array[0]} = "cata" ]; then
      startCata "${array_ele}" "${target}" "${isfirst}" "${coordaddr}" "${coordport}" "${host_array[1]}/bin/sdb"
      if [ $? -ne 0 ]; then
         return 1
      fi
   elif [ ${node_array[0]} = "data" ]; then
      startData "${array_ele}" "${target}" "${isfirst}" "${coordaddr}" "${coordport}" "${host_array[1]}/bin/sdb"
      if [ $? -ne 0 ]; then
         return 1
      fi
   else
      return 1
   fi
   REVOKE_TASK_ARR=("${REVOKE_TASK_ARR[@]}" "${host_array[1]}/bin/sdbstop")
   return 0
}


