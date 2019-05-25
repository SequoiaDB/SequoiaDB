#! /usr/bin/python

import pysequoiadb
from pysequoiadb import client
from pysequoiadb.error import (SDBTypeError,
                               SDBBaseError,
                               SDBEndOfCursor)

if __name__ == "__main__":

   try:
      # connect to local db, using default args value.
      # host='localhost', port= 11810, user= '', password= ''
      db = client("192.168.20.48", 11810)
      # create collection space
      # try to get a collection space named by cs_name specified
      cs_name = 'subject'
      cs = db.create_collection_space( cs_name )

      # 1.get collection space
      # try to get a collection space named by cs_name specified
      cs = db.get_collection_space( cs_name )

      # 2.get collection space
      # try to get a collection space named by 'sports' use __getitem__
      cs = db[cs_name]
      print("get collection space:[%s] success" % cs_name)

      # 3.get collection space
      # try to get a collection space named by 'sports' use __getattri__
      cs = db.subject
      print("get collection space:[%s] success." % 'subject')

      # release
      cs_name = cs.get_collection_space_name()
      db.drop_collection_space(cs_name)
      del cs

      del db

   except (SDBTypeError, SDBBaseError) as e:
      print(e)
