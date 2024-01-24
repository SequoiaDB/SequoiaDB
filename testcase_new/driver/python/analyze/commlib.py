from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
         
def get_common_explain(cursor):
   act_result = list()
   while True:
      try:
         item = cursor.next()
         act_result.append(dict(ScanType  = item['ScanType'],
                                IndexName = item['IndexName'],
                                ReturnNum = item['ReturnNum']))
      except SDBEndOfCursor:
         break
   cursor.close()      
   return act_result;         
      
def get_split_explain(cursor):
   act_result = list()
   while True:
      try:
         item = cursor.next()
         act_result.append(dict(GroupName = item['GroupName'],
                                ScanType  = item['ScanType'],
                                IndexName = item['IndexName'],
                                ReturnNum = item['ReturnNum']))
      except SDBEndOfCursor:
         break
   cursor.close() 
   return act_result;   

def get_sort_result(explain_result):
   new_result = list()
   for x in explain_result:
      item = sorted(x.items())
      new_result.append(item) 
   return new_result