#! /usr/bin/python

import pysequoiadb
from pysequoiadb import client
from pysequoiadb.error import (SDBTypeError,
                               SDBBaseError,
                               SDBEndOfCursor)

from bson.objectid import ObjectId

if __name__ == "__main__":

   try:
      # connect to local db, using default args value.
      # host= '192.168.20.48', port= 11810, user= '', password= ''
      db = client("192.168.20.48", 11810)

      cs_name = "gymnasium"
      cs = db.create_collection_space(cs_name)

      cl_name = "sports"
      cl = cs.create_collection(cl_name, {"ReplSize":0})

      # insert single record
      basketball = {"Item":"basketball", "id":0}
      oid = cl.insert(basketball)

      # insert records
      records = []
      for idx in range(2, 10):
         sport = {"sport id":idx}
         records.append(sport)
      cl.bulk_insert(1, records)

      # drop collection
      cs.drop_collection( cl_name )
      del cl
      # drop collection space
      db.drop_collection_space(cs_name)
      del cs

      db.disconnect()
      del db

      print("Success")

   except (SDBTypeError, SDBBaseError) as e:
      print(e)
