from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
import time

def get_data_groups( db ):
   cursor = db.list_replica_groups()
   data_groups = []
   while True:
      try:
         record = cursor.next()
         group_name = record['GroupName']
         if(group_name != 'SYSCoord' and group_name != 'SYSCatalogGroup'):
            data_groups.append(group_name)       
      except SDBEndOfCursor:
         break
   cursor.close()
   return data_groups
         
def check_rg_master( rg ):
   i = 0
   while True:
      try:
         print("get master times: " + str(i))
         time.sleep(3)
         rg.get_master()
         break
      except SDBBaseError as e:
         i = i + 1
         continue         

def check_data_start_status( node ):
   flag = False
   for i in range(100):
      try:
         print("connect data times after group start: " + str(i))
         time.sleep(10)
         node_connect = node.connect()
         flag = True
         break
      except SDBBaseError as e:
         print(e.detail)
         continue
   return flag

def check_data_stop_status( node ):
   flag = False
   for i in range(100):
      try:
         print("connect data times after group stop: " + str(i))
         time.sleep(10)
         node_connect = node.connect()
      except SDBBaseError as e:
         print(e.detail)
         flag = True
         break
   return flag
