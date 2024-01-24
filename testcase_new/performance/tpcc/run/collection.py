#!/usr/bin/env python
from collections import OrderedDict
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError
from pysequoiadb.error import SDBEndOfCursor 

import sys
def getAllDataGroup( db ):
   groupnames = []
   try: 
      cursor = db.get_list(7)
      while True:
         try:
            doc = cursor.next()
            name = doc['GroupName']
            if ( name != 'SYSCatalogGroup' and 
                 name != 'SYSCoord' and
                 name != 'SYSSpare'):
               groupnames.append( name )
         except SDBEndOfCursor,e:
            break
   except SDBBaseError,e:
      print "list groups failed, errmsg:%s"%(e.detail)
      return None, False
   return groupnames, True 

def createCS( db ):
   cs,ret = getCS( db )
   if ret == 0:
      dropCS( db )
   try:
      print "creating..."
      cs = db.create_collection_space('tpc_c')
   except SDBBaseError, e:
      print "db.create_collection_space(%s) falied,errmsg:%s"%('tpc_c',e.detail)
      return None, False
   return cs, True

def getCS( db ):
   try:
      cs = db.get_collection_space( 'tpc_c' )
   except SDBBaseError, e:
      print "getCS( " + 'tpc_c' + ") failed,errmsg:%s"%(e.detail)
      return None, e.code  
   return cs, 0
      
def dropCS( db ):
   try:
      db.drop_collection_space('tpc_c')
   except SDBBaseError, e:
      print "drop_collection_space(%s) failed,errmsg:%s"%('tpc_c',e.detail)
      return False
   return True

def createIndex( cl, name, key, unique ):
   try:
      cl.create_index( key, name, unique,False )   
   except SDBBaseError, e:
      print "create index %s failed,errmsg:%s"%(name, e.detail)
      return False
   return True

def createCLByName( db, cs, name, option, groups ):
   try:
      cl = cs.create_collection( name, option )
   except SDBBaseError, e:
      print "create collection %s failed,errmsg:%s"%(name, e.detail)
      dropCS( db )
      return False
   
   if name == 'customer':
      key = OrderedDict()
      key['c_w_id'] = 1
      key['c_d_id'] = 1
      key['c_last'] = 1
      key['c_first'] = 1
      createIndex( cl,'customer_idx1', key, False )
   elif name == 'oorder':
      key = OrderedDict()
      key['o_w_id'] = 1
      key['o_d_id'] = 1
      key['o_carrier_id'] = 1
      key['o_id'] = 1
      createIndex( cl, 'oorder_idx1', key, False )
      key = OrderedDict()
      key['o_w_id'] = 1
      key['o_d_id'] = 1
      key['o_c_id'] = 1
      createIndex( cl, 'o_customer_fkey', key, False )
   return splitCL( db, cl, groups )
      
def splitCL( db, cl, groups ):
   partitionPerGroup = 4096 / len( groups )
   remainingPartition = 4096 % len( groups )
   index = 1
   beginCondition = None
   endCondition = None
   offSet = 0
   while index < len( groups ):
      try:
         if endCondition == None:
            beginCondition = {'Partition' : index*partitionPerGroup}
         else:
            beginCondition = endCondition

         if remainingPartition != 0:
            endCondition = {'Partition': (index+1)*partitionPerGroup + 1 + offSet } 
            remainingPartition -= 1
            offSet += 1
         else:
            endCondition = {'Partition': (index+1)*partitionPerGroup + offSet}
         cl.split_by_condition( groups[0], groups[index], beginCondition, endCondition );
         index += 1
      except SDBBaseError, e:
         print "split from %s to %s failed,errmsg:%s"%(groups[0], groups[index], e.detail)
         dropCS( db )
         return False
   return True
      
def createAllCL( db, cs, groups ):
   option = OrderedDict()
   option['ShardingType'] = 'hash'
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['cfg_name'] = 1
   option['Compressed'] = True
   option['CompressionType'] = 'lzw'
   #option = {'ShardingType': 'hash', 'ShardingKey': {'cfg_name': 1}}
   option['Group'] = groups[0] 
   if not createCLByName(db, cs, 'config', option, groups ):
      return False

   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['w_id'] = 1  
   if not createCLByName( db, cs, 'warehouse', option, groups ):
      return False

   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['d_w_id'] = 1
   option['ShardingKey']['d_id'] = 1
   if not createCLByName( db, cs, 'district', option, groups ):
      return False
   
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['c_w_id'] = 1
   option['ShardingKey']['c_d_id'] = 1
   option['ShardingKey']['c_id'] = 1
   if not createCLByName( db, cs, 'customer', option, groups ):
      return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['hist_id'] = 1
   if not createCLByName( db, cs, 'history', option, groups ):
      return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['no_w_id'] = 1
   option['ShardingKey']['no_d_id'] = 1
   option['ShardingKey']['no_o_id'] = 1
   if not createCLByName( db, cs, 'new_order', option, groups ):
      return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['o_w_id'] = 1
   option['ShardingKey']['o_d_id'] = 1
   option['ShardingKey']['o_id'] = 1
   if not createCLByName( db, cs, 'oorder', option, groups ):
     return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['ol_w_id'] = 1
   option['ShardingKey']['ol_d_id'] = 1
   option['ShardingKey']['ol_o_id'] = 1
   option['ShardingKey']['ol_number'] = 1
   if not createCLByName( db, cs, 'order_line', option, groups ):
      return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['i_id'] = 1
   if not createCLByName( db, cs, 'item', option, groups ):
      return False
      
   option['ShardingKey'] = OrderedDict()
   option['ShardingKey']['s_w_id'] = 1
   option['ShardingKey']['s_i_id'] = 1
   if not createCLByName( db, cs, 'stock', option, groups ):
      return False
   return True

if len( sys.argv ) != 3:
   print "Usage: usage:" + sys.argv[0] + " coordIP:coordPort mode[0:create|1 drop]"
   sys.exit(1)

coordAddrs = sys.argv[1].split(",")
coordAddrPair = coordAddrs[0].split(":")
if len( coordAddrPair ) != 2:
   coordPort = 11810
else:
   coordPort = coordAddrPair[1];

try:
   db = client( coordAddrPair[0], coordPort )
except SDBBaseError,e:
   print "connect %s:%s failed, errmsg: %s"%(sys.argv[1], sys.argv[2], e.detail)
   sys.exit(1)

mode = int(sys.argv[2])
if mode == 0:
   
   groups, ret = getAllDataGroup( db )
   if not ret:
      print "get data group failed"
      sys.exit(1)

   if len( groups ) == 0:
      print "not exist datagroup"
      sys.exit(1)

   cs, ret = createCS( db )
   if not ret :
      print "create collectionspace failed"
      sys.exit(1)

   ret = createAllCL( db, cs, groups )
   if not ret :
      print "create collecion failed"
      sys.exit(1) 
else:
   dropCS( db )
