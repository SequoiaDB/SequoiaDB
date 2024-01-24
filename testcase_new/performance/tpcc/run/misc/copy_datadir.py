#!/usr/bin/env python
import os
import sys
import shutil
import threading

class copyTask:
   def __init__( self, srcDir, destDir ):
      self.worker = threading.Thread(target=copyTask.copyFiles, args=(self,))
      self.srcDir = srcDir
      self.destDir = destDir
                                      
   def start(self):
      self.worker.start()

   def wait(self):      
      self.worker.join()
      
   def copyFiles( self ):
      print "copying..."
      oldSrcDir = self.srcDir
      if oldSrcDir[ len(oldSrcDir) -1 ] == os.path.sep:
         oldSrcDir = oldSrcDir.rstrip(os.path.sep)   
      print "old=%s"%oldSrcDir

      for root, dirNames, fileNames in os.walk( self.srcDir ):
         for fileName in fileNames:
            fileFullPath = os.path.join( root, fileName )
            extendDir = ""
            cnt = 0
            curSrcDir = root
            if curSrcDir[ len(curSrcDir) -1 ] == os.path.sep:
               curSrcDir = curSrcDir.rstrip(os.path.sep)

            while oldSrcDir != curSrcDir:
               baseName = os.path.basename( curSrcDir )
               curSrcDir = os.path.dirname( curSrcDir )
               extendDir = os.path.join( baseName, extendDir )
               print "curSrcDir=%s"%curSrcDir
            curDestDir = os.path.join( self.destDir, extendDir )
            if not os.path.exists( curDestDir ):
               os.mkdir( curDestDir )
            print "from %s to %s"%(fileFullPath, curDestDir)
            shutil.copy( fileFullPath, curDestDir )


if len(sys.argv) == 1:
   print "%s [srcDir,DestDir]| [srcDir,DestDir]..."%(sys.argv[0])
   sys.exit(1)

tasks = []
for arg in sys.argv:
   if arg == sys.argv[0]: 
      continue
   print arg
   dirPair = arg.split(",")
   print len(dirPair)
   if len( dirPair ) != 2:
      print "input parameters error"
      sys.exit(1)
   task = copyTask(dirPair[0], dirPair[1])
   tasks.append(task)

for task in tasks:
   task.start()

for task in tasks:
   task.wait()
