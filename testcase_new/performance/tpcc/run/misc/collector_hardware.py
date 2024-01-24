#!/usr/bin/env python
import os
import commands
import string

def getMemInfo():
   mem = {}
   if not os.access('/proc/meminfo', os.R_OK):
       print "could not access /proc/meminfo"
       return
   
   f = open("/proc/meminfo")
   lines = f.readlines()
   f.close()
   for line in lines:
      if len(line) < 2: continue
      name = line.split(':')[0]
      var = line.split(':')[1].strip()
      if name != 'MemTotal':
         continue
      mem[name] = var
   return mem 

def getCpuInfo():
   cpu = {}
   if not os.access('/proc/cpuinfo', os.R_OK):
       print "could not access /proc/meminfo"
       return

   f = open("/proc/cpuinfo")
   lines = f.readlines()
   f.close()
   coreNum = 0
   physicCpuNum = 1
   physicId=[]
   logicCpuNum = 0  
   for line in lines:
     if len(line) < 2: continue
     name = line.split(':')[0].rstrip()
     var = line.split(':')[1].strip()
     if name == "model name":
         cpu[name] = var
     elif name == "physical id" :
         physicId.append(var) 
     elif name == "cpu cores":
         cpu['Core(s) per socket'] = var    
     elif name == "processor":
         logicCpuNum += 1
   ids = list(set(physicId))
   cpu['Physical number'] = len(ids)
   cpu['Thread(s) per core'] = logicCpuNum / (int)(cpu['Core(s) per socket']) 
   return cpu

def getNicInfo():
   nic = {}
   output = commands.getoutput("lspci|grep 'Ethernet controller'")
   lines  = output.split("\n")
   prevNic = None
   i = 0
   for line in lines:
      curNic = line.split(":")[2].strip()
      if curNic != prevNic:
         nic['NIC'+ str(i)] = curNic
         prevNic = curNic
         i += 1
   return nic

def getDiskCap():
   disk2cap = {}
   installPath = ''
   sdbInstallCfgFile = '/etc/default/sequoiadb' 
   if os.access(sdbInstallCfgFile,os.R_OK):
      cfg = open( sdbInstallCfgFile, 'r')
      lines = cfg.readlines()
      for line in lines:
         parts = line.split( '=' )
         if parts[0].strip() == 'INSTALL_DIR':
            installPath = parts[1].strip()
   s = os.sep
   find = False;
   for rt,dirs,file in os.walk('/opt/sequoiadb/conf/local'):
      for dir in dirs:
         cfgfile = open('/opt/sequoiadb/conf/local/' + dir + os.sep+'sdb.conf')
         lines = cfgfile.readlines()
         for line in lines:
            linepart = line.split('=')
            if linepart[0].strip() == 'dbpath':
               dbpath = linepart[1].strip()
            elif (linepart[0].strip() == 'role' and linepart[1].strip() == 'data') :
               find = True
               break
         if find:
            break

   st = os.statvfs(dbpath)
   disk2cap['Disk capacity'] = "%dGB"%(st.f_frsize * st.f_blocks / 1000 /1000/1000)
   return disk2cap
   
cpuInfo = getCpuInfo()
for key in cpuInfo:
   print "%s=%s"%(key, cpuInfo[key])

memInfo = getMemInfo()
for key in memInfo:
   print "%s=%s"%(key, memInfo[key])

nicInfo = getNicInfo()
for key in nicInfo:
   print "%s=%s"%(key, nicInfo[key])

diskCap = getDiskCap()
for key in diskCap:
   print "%s=%s"%(key, diskCap[key])
