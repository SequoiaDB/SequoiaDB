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

      # insert records
      records = []
      for idx in range(10):
         name = 'SequoiaDB' + str(idx)
         sport = {"Rank":idx, "Name":name}
         records.append(sport)

      cl.bulk_insert(1, [{'idx':i} for i in range(10)]) #records

      full_name = cl.get_full_name()
      sql1 = "select * from %s" % full_name
      sql2 = "insert into %s ( Rank, Name ) values( 10000, 'SequoiaDB' )"\
             % full_name

      # execute sql1
      cr = db.exec_sql(sql1)
      print("The result are below after execute sql:%s" % sql1)
      while True:
         try:
            record = cr.next()
         except SDBEndOfCursor :
            break
         except SDBBaseError:
            raise
         print(record)

      print('\n')

      # execute sql2
      db.exec_update(sql2)
      print("The result are below after execute sql:%s" % sql2)
      cr = cl.query()

      record = cr.next()
      while True:
         try:
            record = cr.next()
         except SDBEndOfCursor :
            break
         except SDBBaseError:
            raise
         print(record)

      # drop collection
      cs.drop_collection( cl_name )
      del cl

      # drop collection space
      db.drop_collection_space(cs_name)
      del cs

      db.disconnect()
      del db

   except (SDBTypeError, SDBBaseError) as e:
      print(e)
   except SDBBaseError as e:
      print(e.detail)
