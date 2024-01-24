# -- coding: utf-8 --
# @decription: lob opeartion
# @testlink:   seqDB-12478
# @author:     LaoJingTang 2017-8-30
import sys

from pysequoiadb import SDBInvalidArgument
from pysequoiadb import SDBTypeError
from pysequoiadb.lob import LOB_WRITE
from pysequoiadb.lob import LOB_READ
from lib import testlib
from lob import util


class LobRandoWrite(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.data_size=1024
      self.data = util.random_str(self.data_size)
      self.expect_data = self.data.encode()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   '''
    	1、创建lob
    	2、seek指定偏移写入lob数据
    	3、close关闭lob对象后，再次选择write模式打开lob
    	4、write模式下seek指定偏移写入lob数据
    	5、READ模式下open打开lob（ openLob(ObjectId id, int mode)，其中mode取值为DBLob.SDB_LOB_READ），读取写入lob数据；检查操作结果
   '''

   def test_lob_13438(self):
      lob1 = self.cl.create_lob()
      data_length=len(self.data)
      lob1.seek(0)
      lob1.write(self.data, data_length)
      lob1.close()

      oid = lob1.get_oid()
      lob2 = self.cl.open_lob(oid, LOB_WRITE)
      lob2.seek(data_length)
      lob2.write(self.data, data_length)
      lob2.close()

      actual = self.__read_lob(oid)
      self.assertEqual(actual[0:data_length],self.expect_data)
      self.assertEqual(actual[data_length:data_length*2],self.expect_data)


   '''
   1、write模式打开已存在lob对象
   2、未加锁，seek指定偏移写入lob数据
   3、检查操作结果
   1、写入lob成功，读取新写入lob数据正确（比较MD5值相同）
   '''
   def test_lob_13439(self):
      lob=self.cl.create_lob()
      lob.seek(100)
      lob.write(self.data,len(self.data))
      lob.close()

      actual=self.__read_lob(lob.get_oid())
      self.assertEqual(actual[100:100+len(self.data)],self.expect_data)

   def test_lob_13340(self):
      lob=self.cl.create_lob()
      lob.write(self.data,len(self.data))
      lob.close()

      actual=self.__read_lob(lob.get_oid())
      self.assertEqual(actual,self.expect_data)


   '''
    	1、重新打开已存在lob
    	2、lock锁定指定范围数据段，写入lob数据
    	3、close关闭lob对象后，再次打开lob
    	4、指定偏移和范围锁定数据段（lockAndSeek接口）,写入lob数据
    	5、检查操作结果
    	1、写入lob成功，查询lob按指定锁定范围写入数据，且写入数据信息正确（比较MD5值）
   '''
   def test_lob_13441(self):
      oid=self.__create_empty_lob()

      lob=self.cl.open_lob(oid,LOB_WRITE)
      data_size=len(self.data)
      lob.lock(0,data_size)
      lob.write(self.data,data_size)
      lob.close()

      actual=self.__read_lob(oid)
      self.assertEqual(actual,self.expect_data)

   """
   1、打开已存在lob对象
   2、lock锁定数据段，执行seek偏移量（如锁定切片为1-10、11）
   3、再次lockAndSeek指定不同范围（如锁定切片为12-20） 3、写入lob
   4、检查操作结果
   1、写入lob成功，查询lob信息和实际插入信息一致（比较MD5值）； 2、list查询lob size信息正确
   """

   def test_lob_13443(self):
      oid=self.__create_empty_lob()

      lob=self.cl.open_lob(oid,LOB_WRITE)
      lob.lock(0,1024)
      lob.seek(100)
      lob.lock_and_seek(0,1024)
      lob.write(self.data,len(self.data))
      lob.close()

      actual=self.__read_lob(oid)
      self.assertEqual(actual,self.expect_data)


   '''
   1、打开已存在lob对象
   2、seek指定偏移锁定多个数据段，其中锁定数据范围有交集（如锁定切片为1-10、5-20）
   3、写入lob
   4、检查操作结果
   1、写入lob成功，查询lob信息和实际插入信息一致（比较MD5值，写范围组合值如1-20）；
   2、list查询lob size信息正确
   '''
   def test_lob_13444(self):
      data = util.random_str(1024)
      expect_data = data.encode()
      oid=self.__create_empty_lob()

      lob=self.cl.open_lob(oid,LOB_WRITE)
      lob.lock_and_seek(0,500)
      lob.lock_and_seek(400,1024)
      lob.seek(0)
      lob.write(data,1024)
      lob.close()

      actual=self.__read_lob(oid)
      self.assertEqual(actual,expect_data)


   """
   1、重新打开lob
   2、指定范围锁定多个不连续数据段，其中存在空切片（如锁定写入切片1-4/6-7）
   3、写入lob
   4、读取lob，其中seek指定偏移范围读取lob，分别验证如下场景： a、读取范围全部为空切片数据（如指定5-6范围） b、读取范围中包含部分空切片数据
   5、检查读取lob结果
   1、a场景：读取lob空切片范围时返回空数据 b场景：读取指定范围内lob数据，查看lob信息正确
   """
   def test_lob_13445(self):
      oid=self.__create_empty_lob()
      data=util.random_str(100)
      expect_data=data.encode()

      lob=self.cl.open_lob(oid,LOB_WRITE)
      lob.lock_and_seek(0,100)
      lob.write(data,100)

      lob.lock_and_seek(200,300)
      lob.write(data,100)
      lob.close()

      actual=self.__read_lob(oid)
      self.assertEqual(actual[0:100],expect_data)
      self.assertEqual(actual[200:300],expect_data)


   '''
   1、重新打开lob
   2、指定范围锁定数据段（lockAndSeek）
   3、写入lob
   4、过程中读取lob（如在写入lob数据，未close前执行读取lob操作）
   5、检查写入和读取lob结果
   1、写入lob成功，读取lob失败，返回对应错误信息
   2、读取lob检查写入lob信息正确
   '''
   def test_lob_13446(self):
      oid=self.__create_empty_lob()

      lob=self.cl.open_lob(oid,LOB_WRITE)
      lob.lock_and_seek(0,self.data_size)
      lob.write(self.data,self.data_size)
      with self.assertRaises(SDBInvalidArgument):
         actual=lob.read(self.data_size)
      lob.close()

      actual=self.__read_lob(oid)
      self.assertEqual(actual,self.expect_data)

   '''
   1、指定oid执行truncateLob操作，删除超过指定长度部分的数据，验证接口中lobId参数合法值和非法值：
   合法值：指定存在的lobId 非法值： a、不存在的lobId b、不满足objectId格式
   验证接口中length参数合法值和非法值： 合法值：非负整数 非法值： a、超过边界值：-1、超过long类型最大值 b、非long类型
   2、检查操作结果（读取lob，查看lob对象长度，执行listLobs查看lobsize信息）
   1、输入参数合法值时truncatelob成功，读取lob数据为truncate后的数据一致
   2、非法值时trunatelob失败，返回对应错误信息（报-6错误）
   '''
   def test_lob_13450(self):
      data=util.random_str(1024)
      expect_data=data.encode()
      oid=self.__create_and_write_lob(data)

      self.cl.truncate_lob(oid,100)
      actual=self.__read_lob(oid)
      self.assertEqual(actual,expect_data[0:100])

      with self.assertRaises(SDBInvalidArgument):
         self.cl.truncate_lob("xxx", 10)

      with self.assertRaises(SDBInvalidArgument):
         self.cl.truncate_lob(oid,-1)

      with self.assertRaises(SDBTypeError):
         self.cl.truncate_lob(oid,"xxxxx")
      actual=self.__read_lob(oid)
      self.assertEqual(actual,expect_data[0:100])

   def test_lob_13409(self):
      oid = self.__create_and_write_lob(self.data)
      self.cl.truncate_lob(oid, 0)
      lob = self.cl.open_lob(oid)
      self.assertEqual(lob.get_size(), 0)
      lob.close()

      oid = self.__create_and_write_lob(self.data)
      self.cl.truncate_lob(oid, 100)
      self.assertEqual(self.__read_lob(oid), (self.data[0:100]).encode())

      oid = self.__create_and_write_lob(self.data)
      self.cl.truncate_lob(oid, 10000)
      self.assertEqual(self.__read_lob(oid), self.expect_data)

   def test_lob_oid(self):
      with self.assertRaises(SDBInvalidArgument):
         self.cl.truncate_lob("xxx", 10)
      with self.assertRaises(SDBInvalidArgument):
         self.cl.remove_lob("xxx")
      with self.assertRaises(SDBInvalidArgument):
         self.cl.open_lob("xxx")
      with self.assertRaises(SDBInvalidArgument):
         self.cl.get_lob("xxx")
      with self.assertRaises(SDBTypeError):
         self.cl.create_lob("xxx")

   def test_lob_13448(self):
      lob = self.cl.create_lob()
      lob.lock(0, 1024)
      lob.lock(sys.maxsize, 1024)
      lob.lock(0, -1);

      with self.assertRaises(SDBInvalidArgument):
         lob.lock(0, -2);

      with self.assertRaises(SDBInvalidArgument):
         lob.lock(0, 0)

      with self.assertRaises(SDBTypeError):
         lob.lock("1", "1")

   def test_lob_13449(self):
      lob = self.cl.create_lob()
      lob.lock_and_seek(0, -1)
      lob.lock_and_seek(sys.maxsize, -1)
      with self.assertRaises(SDBInvalidArgument):
         lob.lock_and_seek(-1, -1)
      with self.assertRaises(SDBTypeError):
         lob.lock_and_seek("1", -1)

      lob.lock_and_seek(0, 1)
      with self.assertRaises(SDBInvalidArgument):
         lob.lock_and_seek(0, 0)
      with self.assertRaises(SDBInvalidArgument):
         lob.lock_and_seek(0, -2)
      with self.assertRaises(SDBTypeError):
         lob.lock_and_seek(0, "1")

   def test_lob_13442(self):
      lob = self.cl.create_lob()
      lob.write(self.data, 1024)
      lob.close()
      createTime = lob.get_create_time()
      initModTime = lob.get_modification_time()
      self.assertTrue( createTime <= initModTime, "createTime:" + str(createTime) + ",initModTime:" + str(initModTime) )

      oid = lob.get_oid()
      lob = self.cl.open_lob(oid, LOB_WRITE)
      lob.seek(0, 0)
      lob.write(self.data, 100)
      lob.close()
      writeModTime = lob.get_modification_time()
      self.assertTrue( initModTime <= writeModTime, "initModTime:" + str(initModTime) + ",writeModTime:" + str(writeModTime) )

      lob = self.cl.open_lob(oid, LOB_READ)
      lob.read(100)
      lob.close()
      readModTime = lob.get_modification_time()
      self.assertTrue( readModTime == writeModTime, "readModTime:" + str(readModTime) + ",writeModTime:" + str(writeModTime)  )

   def __create_empty_lob(self):
      lob = self.cl.create_lob()
      lob.close()
      return lob.get_oid()

   def __create_and_write_lob(self, lob_data):
      lob = self.cl.create_lob()
      lob.write(lob_data, len(lob_data))
      lob.close()
      return lob.get_oid()

   def __read_lob(self, oid):
      lob = self.cl.open_lob(oid)
      return lob.read(lob.get_size())

