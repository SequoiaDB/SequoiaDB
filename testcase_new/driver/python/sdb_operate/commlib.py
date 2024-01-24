from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb.error import SDBBaseError
import time

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
