#coding=utf-8

import threading
import shutil
import os
import commands
import sys

def copyData(dir, srcDir):
   for parent,dirnames, filenames in os.walk(srcDir):
      for filename in filenames:
         shutil.copy(os.path.join(parent, filename), dir) 

def stopSdbcm():
   print "stop sdbcm ..."
   installPath = getInstallPath()
   binPath = os.path.join(installPath, 'bin')
   sdbStop = os.path.join(binPath, 'sdbstop')
   (status,output) = commands.getstatusoutput(sdbStop)
   # (status,output) = commands.getstatusoutput(SDBSTOP)
   if status != 0:
      print output
      sys.exit(1)

def startSdbcm():
   print "start sdbcm..." 
   installPath = getInstallPath()
   binPath = os.path.join(installPath, 'bin')
   sdbStart = os.path.join(binPath, 'sdbstart')

   (status,output) = commands.getstatusoutput(sdbStart)
   #(status,output) = commands.getstatusoutput(SDBSTART)
   if status != 0:
      print output
      sys.exit(1)

def getSrcDir(destDir):
   (absDir, tmp) = os.path.split(destDir)
   tmpDir = absDir
   while True:
      curDir = os.path.basename(tmpDir)
      tmpDir = os.path.dirname(tmpDir)
      if (curDir == "sequoiadb"):
         return tmpDir

def getInstallPath():
   defaultInstallPath='/opt/sequoiadb/'
   defaultCfg = '/etc/default/sequoiadb' 
   if not os.access(defaultCfg, os.F_OK):
      return defaultInstallPath
   try:
      file = open(defaultCfg, 'r')
      lines = file.readlines()
      for line in lines:
         parts = line.split('=')
         if len(parts) == 2 and parts[0] == 'INSTALL_DIR':
            return parts[1].strip()
      return defaultInstallPath
   finally:
      file.close()
   
def main():
   if len(sys.argv) != 2:
      print '%s <warehouses 2000|6000>'%sys.argv[0]
      sys.exit(1)  
   try:
      if int(sys.argv[1]) != 2000 and int(sys.argv[1]) != 6000:
         print 'input error'
         sys.exit(1)
   except ValueError,e:
      print 'input error '
      print '%s <warehouses 2000|6000>'%sys.argv[0]
      sys.exit(1)
   datasize = '100g'
   if int(sys.argv[1]) == 6000:
      datasize = '300g'

   installPath = getInstallPath()

   binPath = os.path.join(installPath, 'bin')
   sdbList = os.path.join(binPath, 'sdblist')
   
   if not os.access( sdbList, os.F_OK) :
      print "%s not exist"%sdbList
      sys.exit(1)

   (status,output) = commands.getstatusoutput(sdbList + ' -l -r data|sed \'1d;$d\'|awk \'{print $10}\'')
   if status != 0:
      print "list failed!!!";
      sys.exit(1)

   threads = []
   dirs = output.split('\n')
   for dir in dirs:
      if not os.access(dir, os.F_OK):
         print "%s don't access"%(dir)
         sys.exit(2)

      srcDir = getSrcDir(dir)
      srcDir = os.path.join(srcDir, datasize)
      if not os.access(srcDir, os.F_OK):
         print "%s don't access"%(srcDir)
         sys.exit(2)

      t = threading.Thread(target=copyData , args=(dir, srcDir))
      threads.append(t) 
   stopSdbcm()
   for t in threads:
      #t.setDaemon(True)
      t.start()
   for t in threads:
      t.join()
   startSdbcm()
   
main()
