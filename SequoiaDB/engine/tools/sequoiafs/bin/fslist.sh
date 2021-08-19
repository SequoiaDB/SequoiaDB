#!/bin/bash
BashPath=$(dirname $(readlink -f $0))

pwdpath=$(pwd)

colonstr=":"
sepline=0x9527
    
function Usage()
{
    echo  "Usage: fslist [options] [args]"
    echo  "Command options:"
    echo  "  -h [ --help ]             help information"
    echo  "  -l [ --long ]             show long style "
    echo  "  -m [ --mode ] arg         mode type: run/local, default: run"
    echo  "  --detail                  show details"
}

function List_run()
{
    prefix="sequoiafs"
    fusetype="fuse.sequoiafs"

    showlong=$1
    showdetail=$2

    count=0
    mountlist=$(mount |grep $fusetype | grep -v grep |  awk '{print $3}' )  

    if [ "$showlong" == "true" ]; then
        echo -e "Alias\t $nullstr\t Mountpoint\t PID\t Collection\t ConfPath" >> /tmp/22.$$
    else
        echo -e "Mountpoint\t PID"     >> /tmp/22.$$
    fi

    for mountinfo in $mountlist
    do
        aliasinfo="-"
        pidinfo="-"
        collectioninfo="-"
        confpathinfo="-"
        aliasinfo=$( mount -t $fusetype |grep $mountinfo" " |  awk '{print $1}' | awk -F"(" '{print $1}')
        pidinfo=$( mount -t $fusetype|grep $mountinfo" " |  awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')
        if [ "$pidinfo" != "" ]; then
            cmdconfpathinfo=$( ps -ef| grep  $pidinfo" " | grep " -c " |grep -v grep | awk -F" -c " '{print $2}' | awk -F" " '{print $1}')
            if [ -z $cmdconfpathinfo ]; then
               cmdconfpathinfo=$( ps -ef| grep  $pidinfo" " | grep " --confpath " |grep -v grep | awk -F" --confpath " '{print $2}' | awk -F" " '{print $1}')
            fi
            
            if [ "$cmdconfpathinfo" != "" ]; then 
                confpathinfo=$cmdconfpathinfo
            else
                if [ -f "$BashPath/sequoiafs.conf" ]; then
                    confpathinfo=$BashPath
                fi                
            fi
        fi
        if [ "$showlong" == "true" ]; then
            if [ "$pidinfo" != "" ]; then    
                if [ "$confpathinfo" != "-" ]; then 
                    if [ -f "$confpathinfo/sequoiafs.conf" ]; then
                        source "$confpathinfo/sequoiafs.conf"
                        collectioninfo=$collection
                    fi    
                fi
                
                cmdclinfo=""
                cmdclinfo=$( ps -ef| grep  $pidinfo | grep " -l " |grep -v grep | awk -F" -l " '{print $2}' | awk -F" " '{print $1}')
                if [ -z $cmdclinfo ]; then
                    cmdclinfo=$( ps -ef| grep  $pidinfo | grep " --collection " |grep -v grep | awk -F" --collection " '{print $2}' | awk -F" " '{print $1}')
                fi
                
                if [ "$cmdclinfo" != '' ]; then
                    collectioninfo=$cmdclinfo
                fi
            fi
            
            echo -e "$aliasinfo\t $mountinfo\t $pidinfo\t $collectioninfo\t $confpathinfo"  >> /tmp/22.$$
        else           
            echo -e "$mountinfo($aliasinfo)\t $pidinfo"  >> /tmp/22.$$
        fi
        
        if [ "$detail" == "true" ]; then
            if [ -f "$confpathinfo/sequoiafs.conf" ]; then
                cat "$confpathinfo/sequoiafs.conf" | while read line
                do 
                    if [[ "$line" != \#* ]] && [[ "$line" = *=* ]]; then
                        key=$(echo $line | awk -F"=" '{print $1}') 
                        value=$(echo $line | awk -F"=" '{print $2}')
                        echo -e "$key\t $colonstr $value" >> /tmp/11.$$
                    fi
                done
            fi
            echo $sepline >> /tmp/11.$$
        fi
        
        let "count++"  
    done
    
    return $count
}

function List_local()
{
    confrootpath="$BashPath/../conf/local"
    fusetype="fuse.sequoiafs"
    
    showlong=$1
    showdetail=$2
    count=0
    
    if [ "$showlong" == "true" ]; then
        echo -e "Alias\t Mountpoint\t PID\t Collection\t ConfPath" >> /tmp/22.$$
    else
        echo -e "Mountpoint\t PID"     >> /tmp/22.$$    
    fi
    
    if [ -d "$BashPath/../conf/local" ]; then
      confrootpath=$(cd "$BashPath/../conf/local"; pwd)
    else
      return $count    
    fi
    
    
    for aliasinfo in `ls "$confrootpath"`
    do
        mountinfo="-"
        pidinfo="-"
        collectioninfo="-"
        confpathinfo="-"
        mountpid=""
        cmdconfpathinfo=""
        confpathinfo="$confrootpath/$aliasinfo"
        if [ -f "$confpathinfo/sequoiafs.conf" ]; then
            if [ "$detail" == "true" ]; then
                cat "$confpathinfo/sequoiafs.conf" | while read line
                do 
                    if [[ "$line" != \#* ]] && [[ "$line" = *=* ]]; then
                        key=$(echo $line | awk -F"=" '{print $1}') 
                        value=$(echo $line | awk -F"=" '{print $2}')
                        echo "$key $colonstr $value" >> /tmp/11.$$            
                    fi
                done
            fi
            echo $sepline >> /tmp/11.$$
            
            source "$confpathinfo/sequoiafs.conf"
            if [ "$collection" != "" ]; then
                collectioninfo=$collection
            fi
            if [ "$mountpoint" != "" ]; then
                mountinfo=$(cd "$mountpoint"; pwd)
                mountpid=$( mount -t $fusetype |grep $mountinfo" " | awk '{print $1}' | awk -F"(" '{print $2}' | awk -F")" '{print $1}')    
                if [ -n "$mountpid" ]; then
                  cmdconfpathinfo=$( ps -ef| grep  $mountpid" " | grep " -c " |grep -v grep | awk -F" -c " '{print $2}' | awk -F" " '{print $1}')
                  if [ -z $cmdconfpathinfo ]; then
                    cmdconfpathinfo=$( ps -ef| grep  $mountpid" " | grep " --confpath " |grep -v grep | awk -F" --confpath " '{print $2}' | awk -F" " '{print $1}')
                  fi
                  if [ -n $cmdconfpathinfo ]; then
                    cmdconfpathinfo=$(cd "$cmdconfpathinfo"; pwd)    
                    if [ "$confpathinfo" == "$cmdconfpathinfo" ]; then
                       pidinfo=$mountpid
                    fi        
                  fi
                fi  
            fi    
            
            if [ "$showlong" == "true" ]; then
                echo -e "$aliasinfo\t $mountinfo\t $pidinfo\t $collectioninfo\t $confpathinfo" >> /tmp/22.$$
            else        
                echo -e "$mountinfo($aliasinfo) $pidinfo"  >> /tmp/22.$$
            fi
            
            let "count++"             
        fi    
    done

    return $count
}


slong=""
mode="run"
detail=""

while [ -n "$1" ]
do
    case $1 in
    -h|--help)
        Usage
        exit 0
        ;;
    -m|--mode)
        mode=$2
        shift
        ;;
    -l|--long)
        slong="true"
        ;;
    --detail)
        detail="true"
        ;;
    *)
        Usage
        exit 127
        ;;
    esac
shift
done

if [[ "$mode" == "run" || "$mode" == "local" ]]; then 
    touch /tmp/22.$$
    touch /tmp/11.$$
    echo $sepline> /tmp/11.$$
    touch /tmp/111.$$
    touch /tmp/222.$$
    if [ "$mode" == "run" ]; then 
      List_run "$slong" "$detail"
    else
      List_local "$slong" "$detail"
    fi    
    count=`echo $?` 
    
    cat /tmp/11.$$ | column -t >> /tmp/111.$$
    cat /tmp/22.$$ | column -t >> /tmp/222.$$
    
    param=()
    param_count=0
    while read line
    do 
        param[$param_count]=$line
        let param_count++                    
    done < /tmp/111.$$
    
    current=0
    while read line 
    do 
        echo "$line"
        if [ "$detail" = "true" ]; then
          for((i=$current;i<${#param[@]};i++)) do
            var=${param[$i]}
            if [[ "$var" == "$sepline" ]];then
                let current++    
                break;
             else
                echo "   ""$var"
                let current++    
            fi
          done
        fi
    done < /tmp/222.$$
    
    rm /tmp/11.$$
    rm /tmp/111.$$
    rm /tmp/22.$$
    rm /tmp/222.$$
    
    echo "Total: $count"
    if [ $count == 0 ]; then
      exit 1
    fi
else
    echo "mode invalid"
    Usage 
    exit 127  
fi    

exit 0