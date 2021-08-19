#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)

function Usage()
{
    echo  "Usage: sequoias3 <subcommand> [options] [args]"
    echo  "Command options:"
    echo  "  help                      help informaion"
    echo  ""
    echo  "  start                     start sequoias3 with config"
    echo  "  start -p|--port arg       start sequoias3 with specified port and config"
#    echo  "  start -c|--confpath arg   start sequoias3 with config in confpath"
    echo  ""
    echo  "  stop  -p|--port arg       stop the sequoias3 process which listen the specified port"
    echo  "  stop  -a|--All            stop all sequoias3 process"
    echo  ""
    echo  "  status                    list all sequoias3 process and listening port"
}

function Large()
{
#new > old return 2
#new < old return 1
#new = old return 0
  new=$1
  old=$2

  if [ -z $old ]; then
    return 2
  else
    newversion=(${new//-/ })
    oldversion=(${old//-/ }) 

    newar=(${newversion[2]//./ })
    oldar=(${oldversion[2]//./ })

    if [ ${newar[0]} -gt ${oldar[0]} ]; then
      return 2
    elif [ ${newar[0]} -lt ${oldar[0]} ]; then
      return 1
    else
      if [ ${newar[1]} -gt ${oldar[1]} ]; then
        return 2
      elif [ ${newar[1]} -lt ${oldar[1]} ]; then
        return 1
      else
        if [ ${newar[2]} -gt ${oldar[2]} ]; then
          return 2
        elif [ ${newar[2]} -lt ${oldar[2]} ]; then
          return 1
        else
          newdebug=""
          olddebug=""
          if [ ${newversion[3]} ]; then
            newD=${newversion[3]##*r}
            newDe=(${newD[0]//./ })
            newdebug=${newDe[0]}
          else
            return 2
          fi
          if [ ${oldversion[3]} ]; then
            oldD=${oldversion[3]##*r}
            oldDe=(${oldD[0]//./ })
            olddebug=${oldDe[0]}
          else
            return 1
          fi
          if [ -z $newdebug ]; then
            return 2 
          fi
          if [ -z $olddebug ]; then
            return 1
          fi
          
          if [ $newdebug -gt $olddebug ]; then
            return 2
          elif [ $newdebug -lt $olddebug ]; then
            return 1
          else
            return 0 
          fi
         
        fi
      fi
    fi
  fi
}

function Start()
{
  package=""
  for file in `ls $BashPath`
  do
    if [[ $file = sequoia-s3-*.jar ]]; then
      Large "$file" "$package"
      if [ $? -eq 2 ]; then
        package=$file
      fi
    fi
  done

  if [ -z $package ]; then
    echo "can not find sequoias3 package"
    exit 1
  fi
  echo "start $package"

  port=$1

  packagepath="$BashPath/$package"
  confpath=$BashPath"/config"
  if [ -n "$2" ];then
    confpath=$2
  fi
  #configfile=$confpath"/application.properties"
  #logback=$confpath"/logback.xml"
 
  if [ "$port" != "" ]; then
    if [ $port -lt 0  -o  $port -gt 65535 ]; then
      echo -e "\033[31mthe port $port out of range:0-65535\033[0m"
      exit 1
    fi
    portpid=$(lsof -t -i:$port)                                                                                               
    if [ "$portpid" != "" ] ; then
      echo -e "\033[31mthe port $port already be used by pid:$portpid\033[0m"                                                                                   
      exit 1                                                                                                                          
    fi

    cd $BashPath

    configfile="$confpath/$port/application.properties"
    logback="$confpath/$port/logback.xml"
    nohup java -jar $packagepath --spring.config.location=$configfile --logging.config=$logback 2>nohup.out &
  else
    cd $BashPath
    configfile="$confpath/application.properties"
    logback="$confpath/logback.xml"
    nohup java -jar $packagepath --spring.config.location=$configfile --logging.config=$logback 2>nohup.out &
  fi

  cd $pwdpath  

  pid=$(jobs -l|awk '{print $2}')
  echo "pid:"$pid
  
  portfile="$BashPath/$pid.pid"

  sleep 5 

  listenport=""
  
  loop=0
  while(( $loop < 100 ))
  do
    let "loop++"
    if [ ! -e "$portfile" ]; then
      listenpid=$( ps -ef |grep $pid |grep -v grep)
      if [ "$listenpid" != "" ]; then
        sleep 1
        continue
      else
        echo -e "\033[31mstart failed. please check log/sequoias3.log and config/application.properties\033[0m "
        exit 1
      fi
    else
      listenport=$(cat "$portfile")
      echo "sequoias3($listenport) is started. pid: $pid"
      break
    fi
  done
  
  if [ -z $listenport ]; then
    if [ -n "$( ps -ef |grep $pid |grep -v grep)" ]; then
      echo -e "\033[31mprocess is started, but the LISTEN port is unknown.\033[0m"
      exit 1
    else
      echo -e "\033[31mstart failed. please check log/sequoias3.log and config/application.properties\033[0m "	
      exit 1
    fi
  fi
}

function Stop()
{
  prefix="sequoia-s3-"

  killall=$1
  port=$2

  if [ -z "$killall" ] && [ -z "$port" ]; then
    echo -e "\033[31mstop command must specified -p or -a\033[0m"
    exit 1
  fi

  if [ -n "$killall" ] && [ -n "$port" ]; then
    echo -e "\033[31mcan not specified -a and -p at the same time\033[0m"
    exit 1
  fi

  if [ -n "$killall" ]; then
    pidlist=$(ps -ef|grep $prefix | grep -v grep | awk '{print $2}')
    for pid in $pidlist
    do
      kill $pid
      echo "Terminating process: $pid"
    done
	loop=0
    while(( $loop < 3 ))
    do
      let "loop++"
      pidlist2=$(ps -ef|grep $prefix | grep -v grep )
      if [ -z "$pidlist2" ]; then
        break
      else
        sleep 3
        continue	  
      fi
    done  
    exit 0
  fi

  if [ -n "$port" ]; then
    portpid=$(lsof -t -i :$port)
    if [ -z "$portpid" ]; then
      echo "no such port: $port"
      exit 1
    fi

    pidlist=$(ps -ef|grep $prefix | grep -v grep | awk '{print $2}')
    for pid in $pidlist
    do  
      if [ "$pid" == "$portpid" ]; then
        echo "Terminating process $pid: sequoias3($port)"
        kill $pid
	    loop=0
        while(( $loop < 3 ))
        do
          let "loop++"
          pid2=$(ps -ef|grep $pid | grep -v grep )
          if [ -z "$pid2" ]; then
            break
          else
            sleep 3
            continue	  
          fi
        done 
        exit 0
      fi
    done
  fi
}

function List()
{
  prefix="sequoia-s3-"

  count=0
  pidlist=$( ps -ef|grep $prefix | grep -v grep | awk '{print $2}' )  
  #echo $pidlist
  echo -e "Name\t\t PID\t Port\t Version\t ConfPath"
  for pid in $pidlist
  do
    pidinfo=$(ps -ef|grep $pid|grep $prefix|grep -v grep)
    confinfo=${pidinfo##*--spring.config.location=}
    conf=(${confinfo// / })
    versioninfo=${pidinfo##*sequoia-s3-}
    version=${versioninfo%.jar*}
    port=$(lsof -p $pid | grep LISTEN | awk '{print $9}' | awk -F":" '{print $2}')
    echo -e "sequoias3\t $pid\t $port\t $version\t\t ${conf[0]}"
    let "count++"  
  done
  echo "Total: $count"
}


start=""
stop=""
list=""

killall=""
port=""
configpath=""

if [ "$1" == "start" ]; then
    start="true"
elif [ "$1" == "stop" ]; then
    stop="true"
elif [ "$1" == "status" ]; then
    list="true"
elif [ "$1" == "help" ]; then
    Usage
    exit 0
else
    Usage
    exit 0
fi

shift

while true
do
    case $1 in
    -a|--All)
      killall="true"
      ;;
    -p|--port)
      port=$2
      shift
      ;;
    -c|--confpath)
      configpath=$2
      shift
      ;;
    *)
      break
      ;;
    esac
shift
done

if [ "true" == "$start" ]; then
    Start "$port" "$configpath"
elif [ "true" == "$stop" ]; then
    Stop "$killall" "$port"
elif [ "true" == "$list" ]; then
    List 
else
    echo "do nothing"
fi

exit 0
