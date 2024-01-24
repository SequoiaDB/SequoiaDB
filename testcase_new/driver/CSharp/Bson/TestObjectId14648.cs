using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{
    /**
     * description:  
     *               ObjectId 转换类型验证 
     * testcase:     14648
     * author:       chensiqin
     * date:         2019/03/12
    */

    [TestClass]
    public class TestObjectId14648
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14648";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }
        
        [TestMethod]
        public void Test14648()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            ObjectId oid = new ObjectId("123456789012345678901234");

            BsonDocument record = new BsonDocument();
            record.Add("oid1", oid.ToString());
            record.Add("oid2", oid.ToByteArray());
            cl.Insert(record);

            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                //doc.Remove("_id");
                Assert.AreEqual(record, doc);
            }
            cur.Close();
            Assert.AreEqual(1, count);

            string expected = "{ \"oid1\" : \"string\", \"oid2\" : \"bindata\" }";
            DBQuery query = new DBQuery();
            BsonDocument selector = new BsonDocument();
            selector.Add("oid1", new BsonDocument("$type", 2));
            selector.Add("oid2", new BsonDocument("$type", 2));
            query.Selector = selector;
            cur = cl.Query(query);
            count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                doc.Remove("_id");
                Assert.AreEqual(expected, doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(1, count);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
