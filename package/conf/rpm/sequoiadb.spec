Summary: SequoiaDB
Name: sequoiadb
Version: SEQUOIADB_VERISON.SEQUOIADB_SUBVERSIONSEQUOIADB_EDITION
Release: SEQUOIADB_RELEASE
License: AGPL
Source: sequoiadb-SEQUOIADB_VERISON.SEQUOIADB_SUBVERSIONSEQUOIADB_EDITION.tar.gz
Group: Applications/Databases
AutoReqProv: no
Prefix: /opt/sequoiadb   

%define InstallPath /opt/sequoiadb 

%description
NoSQL database.

%prep

%setup -q

%build

%install
mkdir -p $RPM_BUILD_ROOT%{InstallPath}
cp -rf * $RPM_BUILD_ROOT%{InstallPath}

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%post

# create group
grep -q "^sdbadmin_group:" /etc/group
hasGroup=`echo $?`
if [ $hasGroup -ne 0 ] ; then # group not exists
   groupadd sdbadmin_group
fi

# create user
id sdbadmin
hasUser=`echo $?`
if [ $hasUser -ne 0 ] ; then  # user not exists 
   useradd sdbadmin -m -p sdbadmin -g sdbadmin_group -s /bin/bash 
fi 
echo sdbadmin:sdbadmin | chpasswd 

ls /home/sdbadmin
hasFile=`echo $?`
if [ $hasFile -ne 0 ] ; then # file not exits
   chmod 0755 /home
fi
mkdir -p ~sdbadmin
touch ~sdbadmin/.bashrc
chown -R sdbadmin:sdbadmin_group ~sdbadmin

id sdbadmin
hasUser=`echo $?`
if [ $hasUser -ne 0 ] ; then  # user still not exists 
   exit 1
fi 

# set env
grep -q "^export PATH=\$PATH:${RPM_INSTALL_PREFIX}\/bin$" ~sdbadmin/.bashrc
hasEnv=`echo $?`
if [ $hasEnv -ne 0 ] ; then   # environment has not been set
   echo 'export PATH=$PATH:'${RPM_INSTALL_PREFIX}'/bin' >> ~sdbadmin/.bashrc
fi

# system tuning
ls /proc/sys/net/ipv4/tcp_retries2
hasFile=`echo $?`
if [ $hasFile -eq 0 ] ; then  # file exists
   echo 3 > /proc/sys/net/ipv4/tcp_retries2
fi

# link
ln -s ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so.2.9.0  \
      ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so
ln -s ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so.2.9.0  \
      ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so.2      
ln -s ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so.1.2.5     \
      ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so      
ln -s ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so.1.2.5     \
      ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so.1
ln -s ${RPM_INSTALL_PREFIX}/doc ${RPM_INSTALL_PREFIX}/www/doc

# change permission            
chown -R sdbadmin:sdbadmin_group ${RPM_INSTALL_PREFIX}
chown -R root:root ${RPM_INSTALL_PREFIX}/bin/sdbomtool
chmod -R 0755 ${RPM_INSTALL_PREFIX}/bin
chmod -R 0755 ${RPM_INSTALL_PREFIX}/tools/server/php/bin
chmod -R 0755 ${RPM_INSTALL_PREFIX}/java/jdk/bin
chmod -R 0755 ${RPM_INSTALL_PREFIX}/www/shell
chmod    0755 ${RPM_INSTALL_PREFIX}/install_om.sh
chmod -R 0755 ${RPM_INSTALL_PREFIX}/postgresql
chmod    6755 ${RPM_INSTALL_PREFIX}/bin/sdbomtool
chmod -R 0777 /tmp
 
# system configure
echo "NAME=sdbcm"                         > /etc/default/sequoiadb
echo "SDBADMIN_USER=sdbadmin"            >> /etc/default/sequoiadb
echo "INSTALL_DIR=${RPM_INSTALL_PREFIX}" >> /etc/default/sequoiadb #TODO: md5sum
  
# modify php.ini
sed -i "s#^extension_dir.*#extension_dir=${RPM_INSTALL_PREFIX}\/lib\/phplib#g"  \
       ${RPM_INSTALL_PREFIX}/tools/server/php/lib/php.ini

# add service
cp -f ${RPM_INSTALL_PREFIX}/sequoiadb /etc/init.d/sdbcm
chmod +x /etc/init.d/sdbcm
chkconfig --add sdbcm

# start process
service sdbcm start
   #${RPM_INSTALL_PREFIX}/install_om.sh normal ${RPM_INSTALL_PREFIX} ${installer_pathname} 11790 \
   #                                    11780 ${RPM_INSTALL_PREFIX}/database/sms/11780 8000
   #chown -R sdbadmin:sdbadmin_group ${RPM_INSTALL_PREFIX}/packet

%preun

# stop process
service sdbcm stop
%{RPM_INSTALL_PREFIX}/bin/sdbstop -t all

# remove service 
chkconfig --del sdbcm
rm -rf /etc/init.d/sdbcm

# remove link
rm -rf ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so
rm -rf ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libxml2.so.2
rm -rf ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so
rm -rf ${RPM_INSTALL_PREFIX}/tools/server/php/libxml2/lib/libz.so.1
rm -rf ${RPM_INSTALL_PREFIX}/www/doc


%postun

#remove system configure
rm -rf /etc/default/sequoiadb

# remove env
sed -i "s#^export PATH=\$PATH:$RPM_INSTALL_PREFIX\/bin\$##g" ~sdbadmin/.bashrc  

%files
%defattr(-,root,root)
/opt/sequoiadb/

%changelog
* Mon Jun 13 2016 yuting <yuting@sequoiadb.com>
- Second modify
