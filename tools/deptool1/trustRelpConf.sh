#!/bin/bash 
#config the trust relationship between the hosts 
host=$1 
hostlen=`awk 'BEGIN{print split("'$host'",svcArr,"+")}'`
hlen=$((hostlen-1))
hostName=`hostname`
Cname=`whoami`
Cpath=`ssh $Cname@$hostName "pwd"` 
Chost="$Cname@$hostName"

if [ -z "$host" ] ; then 
	echo "======================================================================================="
	echo "Execute Mode :"
	echo "		./trustReplConf username@IP+username@IP+...+username@IP " 
	echo "e.g:"
	echo "		./trustReplConf root@192.168.20.43+huxiaojun@192.168.20.188"  
	echo "======================================================================================="
	exit 
fi  

rm -rf /roo/.ssh/
#the super login 
#create id_rsa ,id_rsa.pub and copy to authorized_keys 
echo "Config trust relationship begin ..." 
for i in $( seq 1 $hostlen ) 
do 
	hostArr[$i]=`awk 'BEGIN{split("'$host'",svcArr,"+");print svcArr['$i']}'`
	Path=`ssh ${hostArr[$i]} "pwd"`
	ssh ${hostArr[$i]} "rm -rf $Path/.ssh" 	
	#config the /etc/hosts 
	echo "${hostArr[$i]}"|awk 'gsub(/@/," ",$1)' > txt  
	hostIp=`awk '{print $2}' txt `
	hostName=`ssh ${hostArr[$i]} "hostname"`
	echo -e $hostIp"\t"$hostName >> /etc/hosts 
 
	hosts=`awk 'BEGIN{split("'$host'",svcArr,"+");print svcArr['$i']}'`	
	if [ ${hostArr[$i]} = $Chost ] ; then 
#		echo $Path 
		ssh-keygen -t rsa
		cp $Path/.ssh/id_rsa.pub $Cpath/.ssh/authorized_keys.$i	
		echo "local host " 
	else 
#		echo $Path
		ssh ${hostArr[$i]} "ssh-keygen -t rsa" 
		scp ${hostArr[$i]}:$Path/.ssh/id_rsa.pub $Cpath/.ssh/authorized_keys.$i  
	fi
done 
cat $Cpath/.ssh/authorized_keys.* > $Cpath/.ssh/authorized_keys

#copy current autorized_keys to the other host 
for i in $( seq 1 $hostlen )
do 
	hostArr[$i]=`awk 'BEGIN{split("'$host'",svcArr,"+");print svcArr['$i']}'`
#	cat /root/.ssh/authorized_keys.* > /root/.ssh/authorized_keys
	Path=`ssh ${hostArr[$i]} "pwd"`
	scp $Cpath/.ssh/authorized_keys  ${hostArr[$i]}:$Path/.ssh/
	scp /etc/hosts ${hostArr[$i]}:/etc/hosts 
#	rm -rf /root/.ssh/authorized_keys.*
	echo "Config trust relationship over"
done 



