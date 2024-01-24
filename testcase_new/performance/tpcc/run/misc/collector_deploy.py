#!/usr/bin/env python

import sys
import traceback
from pysequoiadb import client
from pysequoiadb.error import SDBEndOfCursor
from pysequoiadb.error import SDBBaseError

if len(sys.argv) <> 2:
   print "%s coordaddr"%(sys.argv[0])
   sys.exit(1)

coordAddr = sys.argv[1]
addrPart = coordAddr.split(":")

coordIp = addrPart[0]
coordPort = 11810
if len(addrPart) == 2:
   coordPort = addrPart[1]
   
deploy = {}
try:
   db = client(coordIp, coordPort)
   cursor = db.list_replica_groups()
   while True:
      doc = cursor.next()
      groupname = doc['GroupName']
      group = doc['Group']
      for node in group:
         key = node['HostName']
         if not deploy.has_key(key):
            deploy[ key ] = {}
         if not deploy[key].has_key(groupname):
            deploy[ key ][groupname] = []
         deploy[ key ][groupname].append(node['Service'][0]['Name'])
except SDBEndOfCursor,e:
   None
except SDBBaseError,e:
   traceback.print_exc()


for key in deploy:
   print "%s"%(key)
   for subkey in deploy[key]:
      print "%s"%(subkey)
      for item in deploy[key][subkey]:
         print "(%s)"%item
   
