# @decription: save records
# @testlink:   seqDB-12502
# @interface:  save(self,doc)
# @author:     liuxiaoxuan 2017-8-29

from bson.objectid import ObjectId
from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError)

cs_name = "cs_12502"
class TestSave12502(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      if (2 > testlib.get_data_group_num()):
         self.skipTest('need at least 2 data groups')

      dataGroups = testlib.get_data_groups()
      srcGroup = dataGroups[0]
      destGroup = dataGroups[1]
      srcGroupName = srcGroup["GroupName"]
      destGroupName = destGroup["GroupName"]

      cl_option = {"ShardingKey": {'no': 1}, "ShardingType": 'hash', "Group": srcGroupName}
      testlib.drop_cs(self.db, cs_name, ignore_not_exist = True)
      self.cs = self.db.create_collection_space(cs_name)
      self.cl = self.cs.create_collection(self.cl_name,options = cl_option)
		
      self.insert_datas()
      self.split_cl(srcGroupName, destGroupName)

   def test_save_12502(self):
      try:
         # insert common field without match id
         doc_commNoMatchId = {"a": "newA_withNoMatchId"}
         self.cl.save(doc_commNoMatchId)
         self.assertFalse("_id" in doc_commNoMatchId)

         condition1 = doc_commNoMatchId
         expectCount1 = 1
         self.check_result(condition1, expectCount1)

         # insert common field and id not exsit
         doc_commNotexistId = {"a": "newA_withNotExistId", "_id": ObjectId("66bb5667c5d061d6f579d000")}
         self.cl.save(doc_commNotexistId)
         self.assertEqual(doc_commNotexistId["_id"], ObjectId("66bb5667c5d061d6f579d000"))

         condition2 = doc_commNotexistId
         expectCount2 = 1
         self.check_result(condition2, expectCount2)

         # upsert common field and id exsit
         doc_commExistId = {"a": "newA_withExistId", "_id": ObjectId("53bb5667c5d061d6f579d0bb")}
         self.cl.save(doc_commExistId)

         condition3 = doc_commExistId
         expectCount3 = 1
         self.check_result(condition3, expectCount3)

         # insert shared field without match id
         doc_shardNoMatchId = {"no": 9999}
         self.cl.save(doc_shardNoMatchId)

         condition4 = doc_shardNoMatchId
         expectCount4 = 1
         self.check_result(condition4, expectCount4)

         # insert shared field and id not exist
         doc_shardNotexistId = {"no": 1001, "_id": ObjectId("92bb5667c5d061d6f580d0ab")}
         self.cl.save(doc_shardNotexistId)

         condition5 = doc_shardNotexistId
         expectCount5 = 1
         self.check_result(condition5, expectCount5)

         # update shared field and id exist
         doc_shardExistId = {"no": 2002, "_id": ObjectId("53bb5667c5d061d6f579d0bb")}
         self.cl.save(doc_shardExistId)

         condition6 = doc_shardExistId
         expectCount6 = 0
         self.check_result(condition6, expectCount6)

         # insert new field
         doc_commNewField = {"newField": "upsertNewField"}
         self.cl.save(doc_commNewField)

         condition7 = doc_commNewField
         expectCount7 = 1
         self.check_result(condition7, expectCount7)
			
			#SEQUOIADBMAINSTREAM-2770,check match _id not ObjectId
         #_id : int			
         mthIntegerId = {"a": "Interger", "_id": 1}
         self.cl.save(mthIntegerId)

         condition1 = mthIntegerId
         expectCount1 = 1
         self.check_result(condition1, expectCount1)
			
         #_id : float
         mthFloatId = {"a": "Float", "_id": 2017.1019}
         self.cl.save(mthFloatId)

         condition2 = mthFloatId
         expectCount2 = 1
         self.check_result(condition2, expectCount2)
			
			#_id : string
         mthStringId = {"a": "String", "_id": 'testtesttesttesttest'}
         self.cl.save(mthStringId)

         condition3 = mthStringId
         expectCount3 = 1
         self.check_result(condition3, expectCount3)
			
			#_id : object
         mthObjId = {"a": "Object", "_id": {'object' : '123'}}
         self.cl.save(mthObjId)

         condition4 = mthObjId
         expectCount4 = 1
         self.check_result(condition4, expectCount4)
			
      except SDBError as e:
         self.fail('test save fail: ' + str(e))
			
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(cs_name)
         

   def insert_datas(self):
      objectIds = [ObjectId("53bb5667c5d061d6f579d0bb"), \
                   ObjectId("53bb5667c5d061d6f579d0bc"), \
                   ObjectId("53bb5667c5d061d6f579d0bd"), \
                   ObjectId("53bb5667c5d061d6f579d0be"), \
                   ObjectId("53bb5667c5d061d6f579d0bf")]
      flag = 0
      doc = []
      for i in range(0, len(objectIds)):
         doc.append({"_id": objectIds[i], "no": i, "a": "test" + str(i)})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def split_cl(self, srcGroupName, destGroupName):
      try:
         self.cl.split_by_percent(srcGroupName, destGroupName, 50.0)
      except SDBBaseError as e:
         self.fail('split fail: ' + str(e))

   def check_result(self, cond, expectCount):
      actCount = 0
      try:
         actCount = self.cl.get_count(condition=cond)
         self.assertEqual(actCount, expectCount)
      except SDBBaseError as e:
         self.fail('check result fail: ' + str(e))
