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

      # insert single record for query()
      basketball = {"Item":"basketball", "id":0}
      oid = cl.insert(basketball)

      cond = {"id":{'$et':0}}
      print("query one record, using condition=%s" % cond)

      cr = cl.query(condition=cond)
      while True:
         try:
            record = cr.next()
         except SDBEndOfCursor :
            break
         except SDBBaseError:
            raise
         print(record)

      # bulk_insert
      records = []
      for idx in range(2, 10):
         sport = {"sport id":idx}
         records.append(sport)
      cl.bulk_insert(1, records)

      print("query all records:")
      cr = cl.query()
      while True:
         try:
            record = cr.next()
         except SDBEndOfCursor :
            break
         except SDBBaseError:
            raise
         print(record)

      cs.drop_collection( cl_name )
      del cl

      db.drop_collection_space(cs_name)
      del cs

      db.disconnect()
      del db

   except (SDBTypeError, SDBBaseError) as  e:
      print(e)
