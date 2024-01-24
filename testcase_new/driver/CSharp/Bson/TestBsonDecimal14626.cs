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
     *   
     *                 BsonDecimal (String value)
     *                 BsonDecimal (Decimal value)
     *                 1.插入一条带精度的decimal类型的数据
     *                 2.查询该条decimal类型数据，检查操作结果
     * testcase:     seqDB-14626 seqDB-14627 seqDB-14628
     * author:       chensiqin
     * date:         2019/03/13
    */

    [TestClass]
    public class TestBsonDecimal14626
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14626";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14626()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            
            BsonDecimal decimal1 = new BsonDecimal(new Decimal(123.456));
            BsonDecimal decimal2 = new BsonDecimal("123.456");
            Console.WriteLine(decimal1.ToString());

            BsonDecimal decimal3 = new BsonDecimal(new Decimal(123456));
            BsonDecimal decimal4 = new BsonDecimal("123456");

            Assert.AreEqual("{ \"$decimal\" : \"123.456\" }", decimal1.ToString());
            Assert.AreEqual("{ \"$decimal\" : \"123.456\" }", decimal2.ToString());
            Assert.AreEqual("{ \"$decimal\" : \"123456\" }", decimal3.ToString());
            Assert.AreEqual("{ \"$decimal\" : \"123456\" }", decimal4.ToString());

            BsonDocument record = new BsonDocument();
            record.Add("decimal1", decimal1);
            record.Add("decimal2", decimal2);
            record.Add("decimal3", decimal3);
            record.Add("decimal4", decimal4);
            cl.Insert(record);

            DBCursor cur = cl.Query();

            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                Assert.AreEqual("{ \"$decimal\" : \"123.456\" }", doc.GetElement("decimal1").Value.ToString());
                Assert.AreEqual("{ \"$decimal\" : \"123.456\" }", doc.GetElement("decimal2").Value.ToString());
                Assert.AreEqual("{ \"$decimal\" : \"123456\" }", doc.GetElement("decimal3").Value.ToString());
                Assert.AreEqual("{ \"$decimal\" : \"123456\" }", doc.GetElement("decimal4").Value.ToString());

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
