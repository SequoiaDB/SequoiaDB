using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Insert
{
    /**
     * description: insert record,then update / query / delete 
     *              test interface:  insert (BsonDocument record)/Update (DBQuery query)/delete (BsonDocument matcher)/Query（DBQuery query）
     * testcase:    14528
     * author:      wuyan
     * date:        2018/3/9 
    */
    [TestClass]
    public class Insert14528
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14528";

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
        public void TestInsert14528()
        {
            TestInsert();
            TestUpdate();
            TestDelete();
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

        public void TestInsert()
        {
            BsonDocument insertor = new BsonDocument();
            string date = DateTime.Now.ToString();
            insertor.Add("operation", "insert");
            insertor.Add("date", date);
            insertor.Add("no", 1);
            insertor.Add("test", "test001");
            cl.Insert(insertor);

            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("no", 2);
            cl.Insert(insertor1);

            //test Query() 
            BsonDocument matcher = new BsonDocument();
            BsonDocument mvalue = new BsonDocument();
            mvalue.Add("$exists", 1);
            matcher.Add("no", mvalue);
            DBQuery query = new DBQuery();
            query.Matcher = matcher;
            DBCursor cursor = cl.Query(query);
            BsonDocument actObj = null;
            List<BsonDocument> querylist = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                actObj = cursor.Current();
                querylist.Add(actObj);
            }
            cursor.Close();

            //check the insert record    
            List<BsonDocument> explist = new List<BsonDocument>();
            explist.Add(insertor);
            explist.Add(insertor1);

            explist.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            querylist.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            Assert.AreEqual(querylist.ToJson(), explist.ToJson());
        }

        //test update(Query query)
        public void TestUpdate()
        {
            DBQuery query = new DBQuery();
            BsonDocument matcher = new BsonDocument();
            BsonDocument mValue = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            matcher.Add("test", "test001");
            mValue.Add("no", 10);
            modifier.Add("$inc", mValue);
            query.Matcher = matcher;
            query.Modifier = modifier;
            cl.Update(query);

            //test update result
            condition.Add("no", 11);
            condition.Add("test", "test001");
            long count = cl.GetCount(condition);
            Assert.AreEqual(1, count);
        }

        //test delete(BsonDocument matcher)
        public void TestDelete()
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            matcher.Add("no", 11);
            cl.Delete(matcher);
            //test delete result  ;
            long count = cl.GetCount(matcher);
            long allCount = cl.GetCount(null);
            Assert.AreEqual(0, count);
            //cl exists 1 record after delete
            Assert.AreEqual(1, allCount);
        }
    }
}
