#!/usr/bin/env python
import platform
import os
import subprocess
import commands
def getOsInfo():
   os={}
   os['OS Release'] = ''.join(platform.linux_distribution())
   os['kernel'] = platform.uname()[2]
   return os

def getSequoiadbVerInfo():
   sequoiadb = {}
   exeFullPath = None
   if os.access('/etc/default/sequoiadb', os.R_OK):
      f = open('/etc/default/sequoiadb')
      lines = f.readlines()
      f.close()
      for line in lines:
         name = line.split('=')[0]
         var = line.split('=')[1].strip()
         if name == 'INSTALL_DIR':
             exeFullPath = var + '/bin/sequoiadb'
   if exeFullPath == None:
      sequoiadb['SequoiaDB'] = 'could not find SequoiaDB' 
      return sequoiadb
   child=subprocess.Popen([exeFullPath,"--version"],stdout=subprocess.PIPE)
   child.wait()
   ret=child.stdout.read()
   sequoiadb['SequoiaDB'] = ret.replace('\n',',')
   return sequoiadb   
   
def getPostgreSQLVerInfo():
   postgresql = {}
   lines = commands.getoutput("ps -ef|grep postgres|grep -v grep").split("\n")
   exeFullPath = None
   for line in lines:
      parts = line.split()
      for part in parts:   
        if os.access(part, os.R_OK):
           exeFullPath = part
           break
      if exeFullPath != None:
         break
   if exeFullPath == None:
      postgresql['PostgreSQL'] = "could not find PostgreSQL"
   else:
      postgresql['PostgreSQL'] = commands.getoutput(exeFullPath + " --version ").replace('\n',',') 

   return postgresql

osInfo = getOsInfo()
for key in osInfo:
   print "%s=%s"%(key, osInfo[key])
sequoiadbVer = getSequoiadbVerInfo()
for key in sequoiadbVer:
   print "%s=%s"%(key, sequoiadbVer[key])
pgVer = getPostgreSQLVerInfo()
for key in pgVer:
   print "%s=%s"%(key, pgVer[key])
