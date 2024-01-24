from pysequoiadb import *
from lib import testlib

def check_autoincrement(db, cl_full_name, field, autoincrement_info):
   sequence_name = get_sequence_name(db, cl_full_name, field)
   if(sequence_name == None):
      return False
   cursor = db.get_snapshot(15, condition = {"Name":sequence_name})
   sequence_info = testlib.get_all_records(cursor)[0]
   for item in autoincrement_info:
      if(autoincrement_info[item] != sequence_info[item]):
         print("autoincrement_info : " + str(autoincrement_info[item]) + "  sequence_info : " + str(sequence_info[item]))
         return False
   return True

def get_sequence_name(db, cl_full_name, field):
   cursor = db.get_snapshot(8, condition = {"Name":cl_full_name})
   cata_info = testlib.get_all_records(cursor)[0]
   autoincrement = cata_info["AutoIncrement"]
   for item in autoincrement:
      if(field == item["Field"]):
         return item["SequenceName"]
         
def check_autoincrement_exist(db, cl_full_name, field):
   sequence_name = get_sequence_name(db, cl_full_name, field)
   if(sequence_name == None):
      return False
   cursor = db.get_snapshot(15, condition = {"Name":sequence_name})
   try:
      sequence_info = cursor.next()
      if (sequence_info["Name"] == sequence_name):
         return True
   except SDBEndOfCursor:
      return False