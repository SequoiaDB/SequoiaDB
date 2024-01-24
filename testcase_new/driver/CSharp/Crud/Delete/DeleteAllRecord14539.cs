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
     * description: delete truncate 
     *              delete (BsonDocument matcher)；truncate ()
     * testcase:    14539
     * author:      chensiqin
     * date:        2018/3/13
    */
    [TestClass]
    public class DeleteAllRecord14539
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14539";

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
        public void TestDelete14539()
        {
            DBCursor cursor = null;
            try
            {
                List<BsonDocument> insertRecords = InsertDatas(10);
                cl.Truncate();
                cursor = cl.Query();
                Assert.IsNull(cursor.Next(), "truncat not success, query not null");
                cursor.Close();
                cursor = cl.ListLobs();
                Assert.IsNull(cursor.Next(), "truncat not success, ListLobs is not null, query not null");
                cursor.Close();

                //test  delete all
                List<BsonDocument> insertRecords2 = InsertDatas(5);
                cl.Delete(null);
                cursor = cl.Query();
                Assert.IsNull(cursor.Next(), "Delete all not success, query not null");
                cursor.Close();
                cursor = cl.ListLobs();
                Assert.IsNotNull(cursor.Next(), "Delete all not success, ListLobs is not null, query not null");
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

        private List<BsonDocument> InsertDatas(int len)
        {
            List<BsonDocument> dataList = new List<BsonDocument>();
            for (int i = 0; i < len; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", i).
                    Add("str", "test" + i);
                dataList.Add(obj);
            }
            cl.BulkInsert(dataList, 0);

            //TODO:lob类型的数据未插入到集合中
            //insert lob
            DBLob dblob = null;
            dblob = cl.CreateLob();
            byte[] buf = new byte[1000];
            for (int i = 0; i < 1000; i++)
            {
                buf[i] = 65;
            }
            dblob.Write(buf);
            dblob.Close();
            return dataList;
        }
    }
}
