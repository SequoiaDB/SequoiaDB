using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Sql
{
    /**
     * description:  
     *                Exec(string sql)/ExecUpdate(string sql)
     *                1、执行sql创建CS、CL，并检查结果正确性 覆盖sql操作： create collectionspace; list collectionspaces; create collection; list collections 
     *                2、执行sql做数据操作 覆盖操作： insert; select; update; delete;其中select带join、函数（如avg()）、关键字（如group by）等，检查结果正确性
     *                3、执行sql清理环境，覆盖操作： drop collectionspace; drop collection; 并检查删除后的结果正确性 
     * testcase:     seqDB-14580
     * author:       chensiqin
     * date:         2019/03/26
    */

    [TestClass]
    public class TestSQL14580
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string localCSName = "cs14580";
        private string clName = "cl14580";
        private string clName2 = "cl14580_2";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            
        }

        [TestMethod]
        public void Test14580()
        {
            //1、执行sql创建CS、CL，并检查结果正确性 覆盖sql操作： create collectionspace; list collectionspaces; create collection; list collections 
            string sql = "create collectionspace " + localCSName;
            sdb.ExecUpdate(sql);
            sql = "create collection " + localCSName + "." + clName;
            sdb.ExecUpdate(sql);
            sql = "create collection " + localCSName + "." + clName2;
            sdb.ExecUpdate(sql);
            DBCursor cur = sdb.Exec("list collectionspaces");
            BsonDocument expected = new BsonDocument("Name", localCSName);
            List<BsonDocument> actual = new List<BsonDocument>();
            while (cur.Next() != null)
            {
                actual.Add( cur.Current() );
            }
            cur.Close();
            Assert.IsTrue(actual.Contains(expected));

            cur = sdb.Exec("list collections");
            expected = new BsonDocument("Name", localCSName+"."+clName);
            actual = new List<BsonDocument>();
            while (cur.Next() != null)
            {
                actual.Add(cur.Current());
            }
            cur.Close();
            Assert.IsTrue(actual.Contains(expected));

            //2、执行sql做数据操作 覆盖操作： insert; select; update; delete;其中select带join、函数（如avg()）、关键字（如group by）等，检查结果正确性
            List<BsonDocument> expectedList = new List<BsonDocument>();
            BsonDocument record = null;
            for (int i = 1; i <= 10; i++)
            {
                string name = "sequoiadb" + i;
                sql = "insert into " + localCSName + "." + clName + "(age,name) values(" + i + ",\"" + name + "\")";
                sdb.ExecUpdate(sql);
                sql = "insert into " + localCSName + "." + clName2 + "(age,name) values(" + i + ",\"" + name + "\")";
                sdb.ExecUpdate(sql);

                record = new BsonDocument();
                record.Add("age", i );
                record.Add("name", name );
                expectedList.Add(record);
            }
            //select带关键字
            sql = "select age, name from " + localCSName + "." + clName + " order by age asc";
            cur = sdb.Exec(sql);
            int k = 0;
            while (cur.Next() != null)
            {
                Assert.AreEqual(expectedList[k++].ToString(), cur.Current().ToString());
            }
            cur.Close();

            sql = "update " + localCSName + "." + clName + " set age=30 where age < 5";
            sdb.ExecUpdate(sql);
            cs = sdb.GetCollectionSpace(localCSName);
            cl = cs.GetCollection(clName);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("age", new BsonDocument("$et", 30));
            Assert.AreEqual(4, cl.GetCount(matcher));

            //select带join select带关键字 
            sql = "select t1.age, t1.name from " + localCSName + "." + clName + " as t1 inner join " + localCSName + "." + clName2 + " as t2 on t1.age=t2.age order by t1.age desc limit 1";
            cur = sdb.Exec(sql);
            int count = 0;
            while (cur.Next() != null)
            {
                Assert.AreEqual("{ \"age\" : 10, \"name\" : \"sequoiadb10\" }", cur.Current().ToString());
                count++;
            }
            cur.Close();
            Assert.AreEqual(1, count);

            //select带函数
            sql = "select avg(age) as avgage from " + localCSName + "." + clName;
            cur = sdb.Exec(sql);
            count = 0;
            while (cur.Next() != null)
            {
                Assert.AreEqual("{ \"avgage\" : 16.5 }", cur.Current().ToString());
                count++;
            }
            cur.Close();
            Assert.AreEqual(1, count);

            //delete
            sql = "delete from " + localCSName + "." + clName + " where age <= 30";
            sdb.ExecUpdate(sql);
            Assert.AreEqual(0, cl.GetCount(null));

            //drop collection
            sdb.ExecUpdate("drop collection " + localCSName + "." + clName);
            Assert.AreEqual(false, cs.IsCollectionExist(clName));
            //drop collectionspace
            sdb.ExecUpdate("drop collectionspace  " + localCSName);
            Assert.AreEqual(false, sdb.IsCollectionSpaceExist(localCSName));

        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
