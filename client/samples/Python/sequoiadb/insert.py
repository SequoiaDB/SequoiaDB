#! /usr/bin/python

from pysequoiadb import *

if __name__ == "__main__":

   host = "localhost"
   port = 11810
   cs_name = 'sample'
   cl_name = 'employee'

   db = client(host, port)
   try:
      cs = db.create_collection_space(cs_name)
      cl = cs.create_collection(cl_name)

      record1 = {"name": "Tom", "age": 24}
      record2 = {"name": "Jack", "age": 22}
      docs = [record1, record2]

      # case 1: insert
      result1 = cl.insert(record1)
      print(result1)

      # case 2: insert_with_flag
      result2 = cl.insert_with_flag(record2)
      print(result2)

      # case 3: bulk_insert
      result3 = cl.bulk_insert(collection.INSERT_FLG_REPLACEONDUP, docs)
      print(result3)

      # clear
      cs.drop_collection(cl_name)
      db.drop_collection_space(cs_name)
   except SDBBaseError as e:
      print(e)
   finally:
      db.disconnect()
