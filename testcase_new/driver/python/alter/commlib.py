from pysequoiadb import client
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

def get_collection_space_attrbutes(db, **kwargs):

   # get system collection spaces
   cata_host = db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
   cata_svc = db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_servicename()
   cata_db = client(cata_host, cata_svc)
   sys_collection_spaces = cata_db.get_collection('SYSCAT.SYSCOLLECTIONSPACES')
   
   # get origin attributes
   act_cs_attri = list()
   cursor = sys_collection_spaces.query(**kwargs)
   while(True):
      try:
         act_cs_attri.append(cursor.next())
      except SDBEndOfCursor:     		
         break
   
   # check useful key   
   need_check_keys = ['Domain', 'PageSize', 'LobPageSize']
   for x in act_cs_attri:
      for key in x.copy():
         if key not in need_check_keys:
            x.pop(key)            
   return act_cs_attri

def get_collection_attributes(db, **kwargs):

   # get origin attributes
   act_cl_attrs = list()
   cursor = db.get_snapshot(8, **kwargs)
   while(True):
      try:
         act_cl_attrs.append(cursor.next())
      except SDBEndOfCursor:     		
         break
      
   # check useful key
   do_check_keys = ['ShardingKey', 'ShardingType', 'Partition', 'EnsureShardingIndex',
                    'AutoSplit', 'AttributeDesc', 'CompressionType', 'CompressionTypeDesc',
                    'ReplSize', 'Max', 'OverWrite', 'Size']
   for x in act_cl_attrs:
      for key in x.copy():
         if key not in do_check_keys:
            x.pop(key)      
   return act_cl_attrs;
   
def get_domain_attrbutes(db, **kwargs):

   # get origin attributes
   domain_origin_attrs = list()
   cursor = db.list_domains(**kwargs)
   while(True):
      try:
         domain_origin_attrs.append(cursor.next())
      except SDBEndOfCursor:     		
         break
      
   # get new attributes
   domain_new_attrs = list()
   for x in domain_origin_attrs:
      items = dict()
      for key in x.copy():
         if 'Name' == key:
            items[key] = x[key] 
         if'AutoSplit' == key:
            items[key] = x[key]  
         elif 'Groups' == key:
            group_names = [y['GroupName'] for y in x[key]]
            items[key] = group_names
      domain_new_attrs.append(items)
   return domain_new_attrs

def get_sort_result(explain_result):
   new_result = list()
   for x in explain_result:
      item = sorted(x.items())
      new_result.append(item) 
   return new_result