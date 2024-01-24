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
     *                BsonString (int value)         CompareTo (BsonString other)
     *                CompareTo (BsonValue other)   Equals (BsonString rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonString类中所有接口,构造BsonString类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14645
     * author:      chensiqin
     * date:        2019/03/05
    */

    [TestClass]
    public class TestBsonString14645
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14645";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14645()
        {

            prepareData();
            checkData();

            //Compare接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较）

            //相同值比较
            BsonString bsonString1 = new BsonString("sequoiadb");
            BsonString bsonString2 = new BsonString("sequoiadb");
            Assert.AreEqual(0, bsonString1.CompareTo(bsonString2));
            Assert.AreEqual(true, bsonString1.Equals(bsonString2));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonString1.GetHashCode(), bsonString1.GetHashCode());
            Assert.AreEqual(bsonString1.GetHashCode(), bsonString2.GetHashCode());

            bsonString1 = new BsonString("sequoiadb1");
            bsonString2 = new BsonString("sequoiadb");
            Assert.AreEqual(1, bsonString1.CompareTo(bsonString2));
            Assert.AreEqual(false, bsonString1.Equals(bsonString2));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonString1.GetHashCode(), bsonString2.GetHashCode());

            //不同类型相同值比较
            bsonString1 = new BsonString("1");
            Assert.AreEqual(1, bsonString1.CompareTo(new BsonInt32(1)));
            Assert.AreEqual(false, bsonString1.Equals(new BsonInt32(1)));

            //执行tostring
            Assert.AreEqual("1", bsonString1.ToString());
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

        private void prepareData()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDocument record = new BsonDocument();
            record.Add("value", new BsonString("sequoiadb,sequoiadb-pg,sequoiadb-mysql,sequoiacm"));
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("value").Value.CompareTo("sequoiadb,sequoiadb-pg,sequoiadb-mysql,sequoiacm"));
            }
            cur.Close();
        }
    }
}
