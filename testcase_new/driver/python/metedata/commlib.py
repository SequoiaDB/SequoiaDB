from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

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

def is_standalone( db ):
   try:
      db.list_replica_groups()
   except SDBBaseError as e:
      if (-159 == e.code):
         return True
      else:
         print("execute list_replica_groups happen error , e =" + e )
         raise e

   
