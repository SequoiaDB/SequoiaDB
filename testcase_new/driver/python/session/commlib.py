from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
import time
import string
from random import Random
         
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
         
def get_explain_nodename(cursor):
   act_node = []
   while True:
      try:
         rec = cursor.next()
         act_node.append({'NodeName' : rec['NodeName']}) 
         print('query node: ' + rec['NodeName'])
      except SDBEndOfCursor:
         break
   cursor.close()
   return act_node;    
  
def get_session_attri( db ):
   cursor = db.get_session_attri()
   act_session_attr = []
   while True:
      try:
         rec = cursor.next()
         act_session_attr.append(rec) 
      except SDBEndOfCursor:
         break
   cursor.close()  
   return act_session_attr
   
def random_str(length):
   s = ""
   r = Random()
   for i in range(length):
      s += r.choice(string.ascii_letters + string.digits)
   return s  
