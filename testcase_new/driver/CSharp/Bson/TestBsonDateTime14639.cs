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
     *                 BsonDateTime (DateTime value)    BsonDateTime (long millisecondsSinceEpoch)
     *                 1.构造BsonDataTime类型数据，通过C#端插入数据到sdb中
     *                 2.查询插入数据是否正确
     * testcase:    14639
     * author:      chensiqin
     * date:        2019/03/06
    */

    [TestClass]
    public class TestBsonDateTime14639
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14639";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14639()
        {
            DateTime date = new DateTime(2016, 9, 1);
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDateTime bsonDateTime = new BsonDateTime(date);
            BsonDocument record = new BsonDocument();
            record.Add("value1", bsonDateTime);
            record.Add("value2", new BsonDateTime(1482396787000));
            cl.Insert(record);

            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("value1").Value.CompareTo(date));
                Assert.AreEqual(0, doc.GetElement("value2").Value.CompareTo(new BsonDateTime(1482396787000)));
            }
            cur.Close();
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
