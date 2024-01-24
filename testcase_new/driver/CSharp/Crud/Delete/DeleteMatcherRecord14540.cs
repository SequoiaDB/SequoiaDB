using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Delete
{
    /**
     * description: delete 
     *              delete (BsonDocument matcher, BsonDocument hint)
     * testcase:    14540
     * author:      chensiqin
     * date:        2018/3/13
    */
    [TestClass]
    public class DeleteMatcherRecord14540
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14540";
        private BsonDocument nestDoc = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);

        }
       
        [TestMethod()]
        public void TestDelete14540() 
        {
            DBCursor cursor = null; 
            try
            {
                List<BsonDocument> insertRecords = InsertDatas();
                BsonDocument mathcer = new BsonDocument();
                mathcer.Add("str", "test1");
                //matcher is String
                cl.Delete(mathcer,null);
                cursor = cl.Query(mathcer, null, null, null);
                Assert.IsNull(cursor.Next(), "Delete(mathcer,null) not success, query not null");
                Assert.AreEqual(6, cl.GetCount(null));
                cursor.Close();

                //matcher is arr
                int[] arrValue = { 1, 2, 3, 4, 5 };
                mathcer = new BsonDocument();
                mathcer.Add("arr", new BsonArray(arrValue));
                cl.Delete(mathcer, null);
                cursor = cl.Query(mathcer, null, null, null);
                Assert.IsNull(cursor.Next(), "Delete mathcer arr not success, query not null");
                Assert.AreEqual(5, cl.GetCount(null));
                cursor.Close();

                //matcher is nest
                cl.Delete(nestDoc, null);
                cursor = cl.Query(mathcer, null, null, null);
                Assert.IsNull(cursor.Next(), "Delete mathcer nest type not success, query not null");
                Assert.AreEqual(4, cl.GetCount(null));
                cursor.Close();

                //matcher is null
                cl.Delete(null, new BsonDocument("","$id"));
                cursor = cl.Query();
                Assert.IsNull(cursor.Next(), "delete all not success, query not null");
                Assert.AreEqual(0, cl.GetCount(null));
                cursor.Close();
            }
            catch (BaseException e)
            {
                Assert.Fail("Failed to Truncate:" + e.ToString());
            }
            finally
            {
                if (cursor != null)
                {
                    cursor.Close();
                }
            }

        }
        
        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            catch (BaseException e)
            {
                Assert.Fail("Failed to clearup:", e.ErrorCode + e.Message);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end: " + this.GetType().ToString());
            }
        }

        private List<BsonDocument> InsertDatas()
        {
            List<BsonDocument> dataList = new List<BsonDocument>();
            for (int i = 0; i < 5; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("num", i).
                    Add("str", "test" + i);
                dataList.Add(obj);
            }
            cl.BulkInsert(dataList, 0);
            int[] arrValue = {1, 2, 3, 4, 5};
            BsonDocument doc = new BsonDocument();
            doc.Add("arr", new BsonArray(arrValue));
            cl.Insert(doc);
            nestDoc = new BsonDocument
            {
                {"Name","xiaoming"},
                {"Age",112},
                {"Address",
                    new BsonDocument
                    {
                        {"StreetAddress","212ndStreet"},
                        {"City","NewYork"},
                        {"State","NY"},
                        {"PostalCode","10021"}
                    }
                }
            };
            cl.Insert(nestDoc); 
            
            return dataList;
        }
    }
}
