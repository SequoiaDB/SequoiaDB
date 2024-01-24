#!/bin/bash 
#config the trust relationship between the hosts 
#path :sequoiadb/tools/
declare -A PASSWD 
locPath=`pwd` 
roLen=`cat host.conf|wc -l` 
#echo $roLen
echo "*******Please check your host.conf,is write correct or not !****"
echo "*******Waiting ......*******************************************"
sleep 5
echo "**********************************trust relationship certificate********************************"
for i in $(seq 2 $roLen) 
do 
	USER[$i]=`sed -n ''$i'p' host.conf|cut -d ':' -f 2 ` 	
#	PASSWD[$i]=`sed -n ''$i'p' host.conf|cut -d ':' -f 3 ` 
	HOST[$i]=`sed -n ''$i'p' host.conf|cut -d ':' -f 3 ` 
#	echo $i ${USER[$i]} ${PASSWD[$i]} ${HOST[$i]} 
	#collect user's password ,put it in variable 
	echo "The host is ${HOST[$i]}"
	echo "Please input it's password:"
	read -s PASSWD[$i] 

done

for i in $(seq 2 $roLen)
do
	#create the id_rsa.pub in the every host of group 
	../bin/expect -c	"
		set timeout 10 ;
		spawn ssh ${USER[$i]}@${HOST[$i]} ; 
		expect {
			\"*yes/no*\" ;{send \"yes\r\" ;exp_continue}
			\"assword\" ;{send \"${PASSWD[$i]}\r\" ; exp_continue}
			\"*Documentation*\" ;{send \"cd .ssh/\r\" ;send \"mkdir -p cpfolder\r\" ;send \"ssh-keygen -t rsa\r\" ;exp_continue}
			\"*Enter file in which to save the key*\" ;{send \"\r\" ;exp_continue}
			\"Overwrite (y/n)\" ; {send \"y\r\" ;exp_continue}
			\"*Enter passphrase*\" ;{send \"\r\" ;exp_continue}
			\"*Enter same passphrase again*\" ;{send \"\r\" ;exp_continue}
			\"*The key's randomart image is*\" ;{send \"hostname >cpfolder/hostfile.$i\r\" ;send \"cp id_rsa.pub cpfolder/authorized_keys.$i\r\" ;exp_continue}
			eof
			{
				send_user \"eof\n\" ; 
			}
		}
					"
done

echo "******************************trust relationship certificate OVER*************************************"
echo "******************************Copy authorized_keys to localhost***************************************"

#copy the authorized_keys to localhost
for i in $(seq 2 $roLen)
do
	 ../bin/expect -c    	"
		set timeout 10 ;
		spawn scp -r ${USER[$i]}@${HOST[$i]}:~/.ssh/cpfolder $locPath ;
		expect {
			\"assword\" ;{send \"${PASSWD[$i]}\n\";exp_continue}
			eof       
			{
				send_user \"eof\n\" ;
			}
		}
					"
	cat $locPath/cpfolder/authorized_keys.* > /root/.ssh/authorized_keys	
done 

echo "******************************Copy authorized_keys to localhost OVER*************************************"
echo "******************************Copy localhost authorized_keys to remote hosts******************************"

#copy the authorized_keys to other hosts 
for i in $(seq 2 $roLen)
do
	../bin/expect -c	"
		set timeout 5 ;
		spawn scp /root/.ssh/authorized_keys ${USER[$i]}@${HOST[$i]}:~/.ssh/ ; 
		expect {
			\"yes\";{send \"yes\r\";exp_continue}
			\"assword\";{send \"${PASSWD[$i]}\n\";exp_continue}
			eof
			{	
				send_user \"eof\n\" ; 	
			}
		}
					"
done

echo "******************************Copy localhost authorized_keys to remote hosts OVER*************************"
echo "******************************SSH hosts and remove cpfolder***********************************************"

#ssh the hosts and remove cpfolder
for i in $(seq 2 $roLen)
do
	../bin/expect -c 	"
		set timeout 5 ;
		spawn ssh ${USER[$i]}@${HOST[$i]} ;  
		expect {
			\"yes\";{send \"yes\r\";exp_continue}
			\"assword\";{send \"${PASSWD[$i]}\n\";send \"rm -rf ~/.ssh/cpfolder/\" ;exp_continue}
			eof
			{
				send_user \"eof\n\" ;
			}
			
		}					
					"
	rm -rf ./cpfolder/
done   

echo "******************************SSH hosts and remove cpfolder OVER******************************************"

