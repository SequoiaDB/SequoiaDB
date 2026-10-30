'''
Description   : seqDB-24057:创建/列取索引
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2023.06.29
LastEditors   : liuli
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
from pymongo import IndexModel, ASCENDING, DESCENDING
import utils
import unittest

class TestIndex24057( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24057'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      self.idxName1 = 'idx1'
      self.idxName2 = 'idx2'
      self.idxName3 = 'idx3'
      # 清理环境
      self.cl.drop()
      
   def test_index( self ):
      self.create_drop_list()
      self.create_drop_indexes()
      self.unique_index()
      self.create_index_IndexModel
   
   def tearDown( self ):
      self.cl.drop() 
      
   def create_drop_list( self ):
      self.cl.drop()

      # create normal index, not set name
      self.cl.create_index( self.idxName1 );
      self.cl.create_index( [ ( 'a', 1 ) ], name = self.idxName2 );
      self.cl.create_index( [ ( 'b', -1 ) ], name = self.idxName3 );

      # index_information
      self.result = self.cl.index_information()
      self.assertRegex( str( self.result ), self.idxName1 + '_1' )   
      self.assertRegex( str( self.result ), self.idxName2 )   
      self.assertRegex( str( self.result ), self.idxName3 )   
      self.assertRegex( str( self.result ), r'_id_' )

      # list_indexes
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 4 )

      # drop_index
      # drop normal index
      self.cl.drop_index( 'idx1_1' )
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )

      # drop $id index (mongodb is _id_ index)
      try:
         self.cl.drop_index( '$id' )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
         
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )

   def create_drop_indexes( self ): 
      self.cl.drop()

      # create_indexes
      self.index1 = IndexModel( [ ( "name", DESCENDING ) ], name = self.idxName1 ) 
      self.index2 = IndexModel( [ ( "age", ASCENDING ) ], name = self.idxName2 ) 
      self.cl.create_indexes( [ self.index1, self.index2 ] ) 

      # list_indexes
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )
      
      ''' SEQUOIADBMAINSTREAM-6979
      # drop_indexes
      self.cl.drop_indexes()

      # list_indexes again
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )
      '''

   def unique_index( self ):
      # ready data          
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 2, "a": 2, "b": 2 },
         { "_id": 3, "a": 3, "b": 2 }] )

      # create_indexes
      self.cl.drop_index( self.idxName1 )
      self.cl.drop_index( self.idxName2 )

      # exist unique index, insert duplicate key
      self.cl.create_index( [( 'a', 1 )], unique = True, backgroud = True )
      try:
         self.cl.insert_one({'a': 1})
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
         
      # exist duplicate key, create unique index
      try:
         self.cl.create_index( [( 'b', 1 )], unique = True, backgroud = True )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise

   def create_index_IndexModel( self ):
      self.cl.drop()

      self.cl.create_index( [ IndexModel( [ ( "a", ASCENDING ) ], name = self.idxName1 )  ] );
      self.cl.create_index( [ IndexModel( [ ( "a", DESCENDING ) ], name = self.idxName2 )  ] );

      # list_indexes
      self.indexes = []
      for self.idx in self.cl.list_indexes():
         self.indexes.append( self.idx )
      self.assertEqual( len( self.indexes ), 3 )