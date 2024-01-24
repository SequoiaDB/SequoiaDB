from pysequoiadb import SDBBaseError
from pysequoiadb import SDBError
from pysequoiadb import client


def __drop_cs_if_exist(self, cs_name):
   try:
      self.drop_collection_space(cs_name)
   except SDBBaseError as e:
      if -34 != e.code:
         raise e


def __is_standalone(self):
   try:
      self.list_replica_groups()
      return False
   except SDBError as e:
      if e.code == -159:
         return True
      else:
         raise e


client.drop_cs_if_exist = __drop_cs_if_exist
client.is_standalone = __is_standalone
