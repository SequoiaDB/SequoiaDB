using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Update
{
    /**
     * description: update all records
     *              test interface:  Update ( BsonDocument  matcher,BsonDocument  modifier,BsonDocument  hint)
     * testcase:    14534
     * author:      wuyan
     * date:        2018/3/13
    */
    [TestClass]
    public class Update14534
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "update14534";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            InsertDatas();
        }

        [TestMethod()]
        public void TestUpdate14534()
        {
            UpdateByArrayTypes();
            UpdateByReplace();
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);                
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }

        private List<BsonDocument> InsertDatas()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 10000; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("no", i).
                    Add("str", "test" + i).
                    Add("a", i).
                    Add("arr", new BsonArray() { i, 12.58 + i, "test" + i }).
                    Add("arr1", new BsonArray() { "test", i, "test" + i });
                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, 0);
            return insertor;
        }

        //update  using $operators of array types, eg:$push_all
        private void UpdateByArrayTypes()
        {
            BsonDocument mValue = new BsonDocument { { "arr", new BsonArray() { "testupdatebypull_all" } } };
            BsonDocument modifier = new BsonDocument { { "$push_all", mValue } };
            cl.Update(null, modifier, null);

            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            {"arr.3", "testupdatebypull_all" }                                         
                                        });
            Assert.AreEqual(10000, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(10000, count);
        }

        //update using $operators of doc types, eg:$replace
        private void UpdateByReplace()
        {
            BsonDocument mValue = new BsonDocument { { "arr", new BsonArray() { "testupdatebyreplae" } }, { "no", "testreplace" } };
            BsonDocument modifier = new BsonDocument { { "$replace", mValue } };
            BsonDocument matcherValue = new BsonDocument { { "$exists", 0 } };
            BsonDocument matcher = new BsonDocument { { "test", matcherValue } };
            cl.Update(matcher, modifier, null);

            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                           { 
                                               {"arr", new BsonArray() { "testupdatebyreplae" } }  ,
                                               { "no", "testreplace"}
                                           });
            Assert.AreEqual(10000, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(10000, count);
        }
    }
}
