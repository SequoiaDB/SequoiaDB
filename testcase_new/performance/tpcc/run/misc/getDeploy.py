#!/bin/env python

import socket
import sys
import os
from properties import Properties
from collections import OrderedDict
from pysequoiadb import client
from pysequoiadb.error import SDBEndOfCursor
from pysequoiadb.error import SDBBaseError 

def parseArgv():
   if len( sys.argv ) != 2:
      print '%s props'%sys.argv[0]
      sys.exit( 1 )
   props = Properties( sys.argv[1] )
   props.load()
   url = props.getProp( 'sdburl' )
   if url == '':
      url = 'localhost:11810'
   urls = url.split(',');
   if len(urls) > 1:
      url = urls[0]
   pair = url.split(':')
   if len(pair) == 1:
      pair.append(str(11810))
      
   if len(pair) != 2:
      print "the properties sdburl config error"
      sys.exit( 1 )
   return pair 

def getDeployInfo( urlPair ):
   try:
      print urlPair
      db = client(urlPair[0], int(urlPair[1]))
      cursor = db.list_replica_groups()
      deployInfo = OrderedDict() 
      while True:
         try:
            doc = cursor.next()
            groupName = doc['GroupName']
            for node in doc['Group']:
               hostName = node['HostName'] ;
               ipAddr = socket.gethostbyname(hostName)
               svcName = node['Service'][0]['Name'] 
               if not deployInfo.has_key( hostName ):
                  deployInfo[hostName] = OrderedDict()
                  deployInfo[hostName]['IP'] = []
                  deployInfo[hostName]['IP'].append(ipAddr)
               if not deployInfo[hostName].has_key( groupName ) :
                  deployInfo[hostName][groupName] = []    
               deployInfo[hostName][groupName].append( svcName )
         except SDBEndOfCursor,e:
            break;
   except SDBBaseError, e:
      print e.detail
      sys.exit( 1 ) 
   return deployInfo

def writeDeployPerHost( deployInfo ):
   props = Properties( sys.argv[1] )
   props.load()
   resultDir = props.getProp( 'resultDirectory' )
   resultDir = os.path.join( os.getcwd(),resultDir )
   if not os.access(resultDir, os.F_OK):
      os.mkdir( resultDir )
   resultDir = os.path.join( resultDir, 'deploy' )
   if not os.access( resultDir, os.F_OK ):
      os.mkdir( resultDir )

   for key in deployInfo:
      try:
         filePath = os.path.join( resultDir,key)
         file = open( filePath, 'w' )
         for subkey in deployInfo[key]:
            file.write( str(subkey) + ': ' )
            for i in range( len( deployInfo[key][subkey] ) ):
               file.write( deployInfo[key][subkey][i] )
               if  i != len( deployInfo[key][subkey]  ) - 1:
                  file.write(',')
            file.write( '\n' )
      finally:
         file.close()

def main():
  urlPair = parseArgv() 
  deployInfo = getDeployInfo( urlPair )
  writeDeployPerHost( deployInfo )

main()
