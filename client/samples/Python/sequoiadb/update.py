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

      # Prepare data
      record = {"name": "Tom", "age": 24}
      cl.insert(record)

      # update
      modifier = {'$set': {"age": 22}}
      cond = {'age': {'$et': 24}}
      result = cl.update(modifier, condition=cond)
      print(result)

      # clear
      cs.drop_collection(cl_name)
      db.drop_collection_space(cs_name)
   except SDBBaseError as e:
      print(e)
   finally:
      db.disconnect()
