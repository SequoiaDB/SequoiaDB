/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using SequoiaDB.Bson;
using System.Text;
using System.Collections;

namespace DriverTest
{
    
    
    [TestClass()]
    public class SequoiadbTest
    {
        private TestContext testContextInstance;
        private static Config config = null;
        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection coll = null;
        string csName = "testfoo";
        string cName = "testbar";

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if ( config == null )
                config = new Config();
        }
        [TestInitialize()]
        public void MyTestInitialize()
        {
            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            BsonDocument options = new BsonDocument();
            options.Add("ReplSize", 0);
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            cs = sdb.CreateCollectionSpace(csName);
            coll = cs.CreateCollection(cName, options);
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        #endregion
        
        [TestMethod()]
        public void ConnectTest()
        {
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            System.Console.WriteLine(config.conf.Coord.Address.ToString());
            // check whether it is in the cluster environment or not
            if (!Constants.isClusterEnv(sdb))
            {
                System.Console.WriteLine("ConnectWithAuth is for cluster environment only.");
                return;
            }
            sdb.CreateUser("testusr", "testpwd");
            sdb2.Connect("testusr", "testpwd");
            Assert.IsNotNull(sdb.Connection);
            sdb2.RemoveUser("testusr", "testpwd");
            sdb2.Disconnect();
            Assert.IsNull(sdb2.Connection);
            try
            {
                sdb2.Connect("testusr", "testpwd");
            }
            catch (BaseException e)
            {
                Assert.IsTrue(e.ErrorType == "SDB_AUTH_AUTHORITY_FORBIDDEN");
            }
        }

        [TestMethod()]
        public void Connect_With_Serval_Arg_Test()
        {
		    List<string> list = new List<string>();

            list.Add("192.168.20.35:12340");
            list.Add("192.168.20.36:12340");
            list.Add("123:123");
            list.Add("");
            list.Add("192.168.20.40");
            list.Add("192.168.30.161:11810");
            list.Add("localhost:50000");
            list.Add("192.168.20.42:50000");
            list.Add("192.168.20.42:11810");
            list.Add("192.168.20.165:11810");
            list.Add("localhost:12340");
            list.Add("192.168.20.40:12340");

	        ConfigOptions options = new ConfigOptions();
	        options.MaxAutoConnectRetryTime = 0;
	        options.ConnectTimeout = 100;
	        // connect
	        Sequoiadb sdb1 = new Sequoiadb(list);
            sdb1.Connect("", "", options);
	        // set option and change the connect
	        options.ConnectTimeout = 2000;
	        sdb1.ChangeConnectionOptions(options);
	        // check
	        DBCursor cursor = sdb1.GetList(4, null, null, null);
	        Assert.IsTrue(cursor != null);
	        sdb1.Disconnect();
        }

        [TestMethod()]
        public void Connect_With_Serval_Arg_Test2()
        {
            List<string> list = new List<string>();
            int conn_num = 100;
            Sequoiadb []dbs = new Sequoiadb[100] ;
            int i = 0;

            list.Add("192.168.20.35:12340");
            list.Add("192.168.20.165:11810");
            list.Add("192.168.20.35:12340");
            list.Add("192.168.20.42:50000");
            list.Add("192.168.20.42:11810");

            ConfigOptions options = new ConfigOptions();
            options.MaxAutoConnectRetryTime = 0;
            options.ConnectTimeout = 100;
            // connect
            for(i=0;i<conn_num;i++)
            {
                dbs[i] = new Sequoiadb(list);
                dbs[i].Connect("", "", options);
            }
            for(i=0;i<conn_num;i++)
            {
                dbs[i].Disconnect();
            }
        }

        [TestMethod()]
        [Ignore]
        public void ConnectWithSSLTest()
        {
            ConfigOptions cfgOpt = null ;
            CollectionSpace cs2 = null;
            DBCollection coll2 = null;
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            System.Console.WriteLine(config.conf.Coord.Address.ToString());

            // set connect using ssl
            cfgOpt = new ConfigOptions();
            cfgOpt.UseSSL = true;

            // connect to database
            sdb2.Connect("", "", cfgOpt);
            if (true == sdb2.IsCollectionSpaceExist("testSSL"))
                cs2 = sdb2.GetCollectionSpace("testSSL");
            else
                cs2 = sdb2.CreateCollectionSpace("testSSL");
            if (true == cs2.IsCollectionExist("testSSL"))
                coll2 = cs2.GetCollection("testSSL");
            else
                coll2 = cs2.CreateCollection("testSSL");

            sdb2.DropCollectionSpace("testSSL");
        }

        [TestMethod()]
        public void IsValidTest()
        {
            bool result = false;
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            System.Console.WriteLine(config.conf.Coord.Address.ToString());
            sdb2.Connect("", "");
            Assert.IsNotNull(sdb2.Connection);
            // check before disconnect
            result = sdb2.IsValid();
            Assert.IsTrue(result);
            // check after disconnect
            sdb2.Disconnect();
            result = sdb2.IsValid();
            Assert.IsFalse(result);
            /*
            // check after shutdown database manually
            sdb2 = new Sequoiadb(config.conf.Coord.Address);
            sdb2.Connect("", "");
            result = true;
            result = sdb2.IsValid();
            Assert.IsFalse(result);
             */
        }

        [TestMethod()]
        [Ignore]
        public void IsClosedTest()
        {
            //bool result = false;
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            System.Console.WriteLine(config.conf.Coord.Address.ToString());
            sdb2.Connect("", "");
            Assert.IsNotNull(sdb2.Connection);
            // TODO:
            //result = sdb2.IsClosed();
            Assert.IsFalse(false);
            // check
            sdb2.Disconnect();
            //result = sdb2.IsClosed();
            Assert.IsTrue(true);
        }

        [TestMethod()]
        public void CollectionSpaceTest()
        {
            string csName = "Test";
            string csName2 = "Test2";
            BsonDocument option = new BsonDocument();
            option.Add(SequoiadbConstants.FIELD_PAGESIZE, 4096);

            if (!sdb.IsCollectionSpaceExist(csName))
            {
                // default and with page size
                sdb.CreateCollectionSpace(csName);
            }
            Assert.IsTrue(sdb.IsCollectionSpaceExist(csName));
            sdb.DropCollectionSpace(csName);
            Assert.IsFalse(sdb.IsCollectionSpaceExist(csName));
            // with options
            sdb.CreateCollectionSpace(csName2, option);
            Assert.IsTrue(sdb.IsCollectionSpaceExist(csName2));
            sdb.DropCollectionSpace(csName2);
            Assert.IsFalse(sdb.IsCollectionSpaceExist(csName2));
        }

        [TestMethod()]
        public void DropCSTest()
        {
            string csName = "dropCSTest_CS";
            string clName = "testCL";

            // case 1
            BsonDocument option1 = new BsonDocument();
            option1.Add("EnsureEmpty", false);
            CollectionSpace cs1 = sdb.CreateCollectionSpace(csName);
            cs.CreateCollection(clName);
            sdb.DropCollectionSpace(csName, option1);

            // case 2
            try
            {
                BsonDocument option2 = new BsonDocument();
                option2.Add("EnsureEmpty", true);
                CollectionSpace cs2 = sdb.CreateCollectionSpace(csName);
                cs2.CreateCollection(clName);
                sdb.DropCollectionSpace(csName, option2);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(e.ErrorType == "SDB_DMS_CS_NOT_EMPTY");
                sdb.DropCollectionSpace(csName);
            }
        }

        [TestMethod()]
        public void ExecTest()
        {
            // insert English
            string sqlInsert = "INSERT INTO " + csName + "." + cName +
                " ( c, d, e, f ) values( 6.1, \"8.1\", \"aaa\", \"bbb\")";
            try
            {
                sdb.ExecUpdate(sqlInsert);
            }
            catch (BaseException e)
            {
                string errInfo = e.Message;
                Console.WriteLine("The error info is: " + errInfo);
                Assert.IsFalse(1 == 1); 
            }
            // insert Chinese
            string sqlInsert1 = "INSERT INTO " + csName + "." + cName +
                " ( 城市1, 城市2 ) values( \"广州\",\"上海\")";
            //string str = "INSERT into testfoo.testbar ( 城市1, 城市2 )  values( \"广州\",\"上海\" )";
            try
            {
                sdb.ExecUpdate(sqlInsert1);
            }
            catch (BaseException e)
            {
                string errInfo = e.Message;
                Console.WriteLine("The error info is: " + errInfo);
                Assert.IsFalse(1 == 1);
            }
            // select some 
            string sqlSelect = "SELECT 城市1 FROM " + csName + "." + cName;
            DBCursor cursor = sdb.Exec(sqlSelect);
            Assert.IsNotNull(cursor);
            while (cursor.Next() != null)
            {
                BsonDocument bson = cursor.Current();
                string temp = bson.ToString();
                Assert.IsNotNull(bson);
            }
            // select all
            string sqlSelect1 = "SELECT FROM " + csName + "." + cName;
            cursor = sdb.Exec(sqlSelect);
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                count++;
                BsonDocument bson = cursor.Current();
                string temp = bson.ToString();
                Assert.IsNotNull(bson);
            }
            Assert.AreEqual(count, 2);
            // remove all the record
            string sqlDel = "DELETE FROM " + csName + "." + cName;
            try
            {
                sdb.ExecUpdate(sqlDel);
            }
            catch (BaseException e)
            {
                string errInfo = e.Message;
                Console.WriteLine("The error info is: " + errInfo);
                Assert.IsFalse(1 == 1);
            }
        }

        [TestMethod()]
        public void ExecTest2()
        {
            // insert English
            string sqlInsert = "update " + csName + "." + cName +
                " set GoodCode='{0}', Count={1} where SaleRID='{2}'";
            sqlInsert = string.Format(sqlInsert, DateTime.Now.ToLongTimeString(), DateTime.Now.Millisecond, "1234567");
            try
            {
                sdb.ExecUpdate(sqlInsert);
            }
            catch (BaseException e)
            {
                Assert.Fail();
                string errInfo = e.Message;
                Console.WriteLine("The error info is: " + errInfo);
                Assert.IsFalse(1 == 1);
            }
        }

        [TestMethod()]
        public void GetSnapshotTest()
        {
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            sdb2.Connect();
            BsonDocument dummy = new BsonDocument();
            DBCursor cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_CONTEXTS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            BsonDocument bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_CONTEXTS_CURRENT, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_SESSIONS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_SESSIONS_CURRENT, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONSPACES, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_DATABASE, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            cursor = sdb2.GetSnapshot(SDBConst.SDB_SNAP_SYSTEM, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            if (Constants.isClusterEnv(sdb))
            {
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                BsonDocument obj = cursor.Next();
                Assert.IsNotNull(obj);
            }
            sdb2.Disconnect();
            // snapshot transation
            sdb.TransactionBegin();
            try
            {
                BsonDocument o = null;
                coll.Insert(new BsonDocument());
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_TRANSACTIONS, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_SNAP_TRANSACTIONS is: ");
                while (null != (o = cursor.Next()))
                {
                    Console.WriteLine(o);
                }
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_SNAP_TRANSACTIONS_CURRENT is: ");
                while (null != (o = cursor.Next()))
                {
                    Console.WriteLine(o);
                }
            }
            catch (BaseException e)
            {
                Console.WriteLine("The error info is: " + e.ErrorType + ", " + e.ErrorCode + ", " + e.Message);
                Assert.IsTrue(e.ErrorType == "SDB_DPS_TRANS_DIABLED");
            }
            finally
            {
                sdb.TransactionCommit();
            }

            // snapshot accessplans
            {
                BsonDocument o = null;
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_ACCESSPLANS, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_SNAP_TRANSACTIONS is: ");
                while (null != (o = cursor.Next()))
                {
                    Console.WriteLine(o);
                }
            }

            // node health 
            {
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_HEALTH, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_SNAP_HEALTH is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
            }

            // 14 svntasks
            {
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SVCTASKS, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_SVCTASKS is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
            }

            // sequences
            {
                cursor.Close();
                cursor = null;
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_SEQUENCES is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
                cursor.Close();
            }

            // SDB_SNAP_QUERIES
            {
                cursor.Close();
                cursor = null;
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_QUERIES, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_QUERIES is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
                cursor.Close();
            }

            // SDB_SNAP_LATCHWAITS
            {
                cursor.Close();
                cursor = null;
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_LATCHWAITS, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_LATCHWAITS is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
                cursor.Close();
            }

            // SDB_SNAP_LOCKWAITS
            {
                cursor.Close();
                cursor = null;
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_LOCKWAITS, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_LOCKWAITS is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
                cursor.Close();
            }

            // indexstats
            {
                cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_INDEXSTATS, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_SNAP_INDEXSTATS is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
                cursor.Close();
            }

        }

        [TestMethod()]
        public void RestSnapshot()
        {
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect();

            sdb.ResetSnapshot(null);

            BsonDocument options = new BsonDocument();
            sdb.ResetSnapshot(options);

            options.Add("Type", "database");
            sdb.ResetSnapshot(options);
        }

        [TestMethod()]
        public void GetListTest()
        {
            BsonDocument dummy = new BsonDocument();
            BsonDocument bson = null;
            DBCursor cursor = null;
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect();

            // list transation
            sdb.TransactionBegin();
            try
            {
                BsonDocument o = null;
                coll.Insert(new BsonDocument());
                cursor = sdb.GetList(SDBConst.SDB_LIST_TRANSACTIONS, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_LIST_TRANSACTIONS is: ");
                while (null != (o = cursor.Next()))
                {
                    Console.WriteLine(o);
                }
                cursor = sdb.GetList(SDBConst.SDB_LIST_TRANSACTIONS_CURRENT, dummy, dummy, dummy);
                Console.WriteLine("the result of SDB_LIST_TRANSACTIONS_CURRENT is: ");
                while (null != (o = cursor.Next()))
                {
                    Console.WriteLine(o);
                }
            }
            catch (BaseException e)
            {
                Assert.AreEqual("SDB_DPS_TRANS_DIABLED", e.ErrorType);
            }
            finally
            {
                sdb.TransactionCommit();
            }

            // list sequences
            {
                cursor = db.GetList(SDBConst.SDB_LIST_SEQUENCES, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                Console.WriteLine("the result of SDB_LIST_SEQUENCES is: ");
                BsonDocument rec = null;
                while (null != (rec = cursor.Next()))
                {
                    Console.WriteLine(rec);
                }
            }


            // list cs
            cursor = db.GetList(SDBConst.SDB_LIST_COLLECTIONSPACES, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            // list cl
            cursor = db.GetList(SDBConst.SDB_LIST_COLLECTIONS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            BsonDocument obj = new BsonDocument("test", "test");
            coll.Insert(obj);

            // list groups
            // check whether it is in the cluster environment or not
            if (Constants.isClusterEnv(db))
            {
                cursor = db.GetList(SDBConst.SDB_LIST_GROUPS, dummy, dummy, dummy);
                Assert.IsNotNull(cursor);
                bson = cursor.Next();
                Assert.IsNotNull(bson);
            }

            // list task
            cursor = db.GetList(SDBConst.SDB_LIST_TASKS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);

            // list domains
            if (Constants.isClusterEnv(db))
            {
                string dmName = "testListDomain";
                Domain dm = db.CreateDomain(dmName, null);
                cursor = null;
                cursor = db.ListDomains(null, null, null, null);
                Assert.IsNotNull(cursor);
                Assert.IsNotNull(cursor.Next());
                db.DropDomain(dmName);
            }

            // list stored procedure
            cursor = db.GetList(SDBConst.SDB_LIST_STOREPROCEDURES, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);

            // list all the contexts
            cursor = db.GetList(SDBConst.SDB_LIST_CONTEXTS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            // list current context
            cursor = db.GetList(SDBConst.SDB_LIST_CONTEXTS_CURRENT, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            // list all the sessions 
            cursor = db.GetList(SDBConst.SDB_LIST_SESSIONS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            // list current session
            cursor = db.GetList(SDBConst.SDB_LIST_SESSIONS_CURRENT, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            // list 16
            cursor = db.GetList(SDBConst.SDB_LIST_USERS, dummy, dummy, dummy, null, 0, -1);
            Assert.IsNotNull(cursor);
            while ((bson = cursor.Next()) != null)
            {
                Console.WriteLine("Result of SDB_LIST_USERS: " + base.ToString());
            }
            
            // list 17
            cursor = db.GetList(SDBConst.SDB_LIST_BACKUPS, dummy, dummy, dummy, null, 0, -1);
            Assert.IsNotNull(cursor);
            while ((bson = cursor.Next()) != null)
            {
                Console.WriteLine("Result of SDB_LIST_BACKUPS: " + base.ToString());
            }

            // list 14
            cursor = db.GetList(SDBConst.SDB_LIST_SVCTASKS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            while ((bson = cursor.Next()) != null)
            {
                Console.WriteLine("Result of SDB_LIST_SVCTASKS: " + base.ToString());
            }
           

            if (Constants.isClusterEnv(db))
            {
                db.Disconnect();
                db = new Sequoiadb(config.conf.Data.Address);
                db.Connect(config.conf.UserName, config.conf.Password);
            }
            // list storge units
            cursor = db.GetList(SDBConst.SDB_LIST_STORAGEUNITS, dummy, dummy, dummy);
            Assert.IsNotNull(cursor);
            bson = cursor.Next();
            Assert.IsNotNull(bson);

            db.Disconnect();

        }

        [TestMethod()]
        [Ignore]
        public void CreateReplicaCataSetTest()
        {
            try
            {
                System.Console.WriteLine(config.conf.Groups[2].Nodes[0].HostName.ToString());
                System.Console.WriteLine(config.conf.Groups[2].Nodes[0].Port.ToString());
                System.Console.WriteLine(config.conf.Groups[2].Nodes[0].DBPath.ToString());
                string str1 = config.conf.Groups[2].Nodes[0].HostName.ToString();
                string str2 = config.conf.Groups[2].Nodes[0].Port.ToString();
                string str3 = config.conf.Groups[2].Nodes[0].DBPath.ToString();
                sdb.CreateReplicaCataGroup(config.conf.Groups[2].Nodes[0].HostName,
                                            config.conf.Groups[2].Nodes[0].Port,
                                            config.conf.Groups[2].Nodes[0].DBPath,
                                            null);
            }
            catch (BaseException)
            {
            }
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Groups[2].Nodes[0].HostName,
                                        config.conf.Groups[2].Nodes[0].Port);
            sdb2.Connect();
            Assert.IsNotNull(sdb2.Connection);
            sdb2.Disconnect();
        }

        [TestMethod()]
        public void Transaction_Begin_Commit_Insert_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // transction begin
            sdb.TransactionBegin();
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // commit
            sdb.TransactionCommit();
            // check up
            DBCursor cursor = cl.Query();
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 2);
            //sdb.TransactionRollback();
        }

        [TestMethod()]
        public void Transaction_Begin_Rollback_Insert_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // transction begin
            sdb.TransactionBegin();
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // rollback
            sdb.TransactionRollback(); 
            // check up
            DBCursor cursor = cl.Query();
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 0);
        }

        [TestMethod()]
        public void Transaction_Begin_Commit_update_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // transction begin
            sdb.TransactionBegin();
            // update
            BsonDocument matcher = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            matcher.Add("name","sam");
            modifier.Add("$set",new BsonDocument("age", 50));
            cl.Update(matcher, modifier, null);
            // commit
            sdb.TransactionCommit();
            // check up
            BsonDocument matcher1 = new BsonDocument("age", 27);
            DBCursor cursor = cl.Query(matcher1, null, null, null);
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 0);
            // check up
            cursor = cl.Query();
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 2);
        }

        [TestMethod()]
        public void Transaction_Begin_Rollback_update_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // transction begin
            sdb.TransactionBegin();
            // update
            BsonDocument matcher = new BsonDocument();
            BsonDocument modifier = new BsonDocument();
            matcher.Add("name", "sam");
            modifier.Add("$set", new BsonDocument("age", 50));
            cl.Update(matcher, modifier, null);
            // rollback
            sdb.TransactionRollback();
            // check up
            BsonDocument matcher1 = new BsonDocument("age", 27);
            DBCursor cursor = cl.Query(matcher1, null, null, null);
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 1);
            // check up
            cursor = cl.Query();
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 2);
        }

        [TestMethod()]
        public void Transaction_Begin_Rollback_delete_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // transction begin
            sdb.TransactionBegin();
            // delete
            BsonDocument matcher = new BsonDocument();
            matcher.Add("name", "sam");
            cl.Delete(matcher);
            // check up
            DBCursor cursor = cl.Query();
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 1);
            // rollback
            sdb.TransactionRollback();
            // check up
            cursor = cl.Query(matcher, null, null, null);
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 1);
            // check up
            cursor = cl.Query();
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 2);
        }

        [TestMethod()]
        public void Transaction_Begin_Commit_delete_Test()
        {
            // create cs, cl
            string csName = "testfoo";
            string cName = "testbar";
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            DBCollection cl = cs.CreateCollection(cName);
            // insert record
            BsonDocument insertor1 = new BsonDocument();
            insertor1.Add("name", "tom");
            insertor1.Add("age", 25);
            insertor1.Add("addr", "guangzhou");
            BsonDocument insertor2 = new BsonDocument();
            insertor2.Add("name", "sam");
            insertor2.Add("age", 27);
            insertor2.Add("addr", "shanghai");
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            // transction begin
            sdb.TransactionBegin();
            // delete
            BsonDocument matcher = new BsonDocument();
            //matcher.Add("name", new BsonDocument("$et","sam"));
            matcher.Add("name", "sam");
            cl.Delete(matcher);
            // check up
            DBCursor cursor = cl.Query();
            Assert.IsNotNull(cursor);
            int count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 1);
            // commit
            sdb.TransactionCommit();
            // check up
            cursor = cl.Query(matcher, null, null, null);
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 0);
            // chech up
            cursor = cl.Query();
            Assert.IsNotNull(cursor);
            count = 0;
            while (cursor.Next() != null)
            {
                ++count;
                BsonDocument bson = cursor.Current();
                Assert.IsNotNull(bson);
            }
            Assert.IsTrue(count == 1);
        }

        ///** 
        // * need to wait for the slave node to sync
        // * i don't know how long it will take,
        // * so, i ignore this test. But, this api works.
        // */

        [TestMethod()]
        [Ignore]
        public void setSessionAttrTest()
        {
            // create another node
            string host = "192.168.20.42";
            int port = 55555;
            string dataPath = "/opt/sequoiadb/database/data/55555";
            string groupName = "group1";
            ReplicaGroup rg = null;
            try
            {
                // get the exist group
                rg = sdb.GetReplicaGroup(groupName);
                // remove the node we going to use
                SequoiaDB.Node node = rg.GetNode(host, port);
                if (node != null)
                {
                    rg.RemoveNode(host, port, new BsonDocument());
                }
                // create node
                Dictionary<string, string> opt = new Dictionary<string, string>();
                rg.CreateNode(host, port, dataPath, opt);
                rg.Start();
                // insert some records first
                int num = 10;
                List<BsonDocument> insertor = new List<BsonDocument>();
                for (int i = 0; i < num; i++)
                {
                    BsonDocument obj = new BsonDocument();
                    obj.Add("id", i);
                    insertor.Add(obj);
                }
                coll.BulkInsert(insertor, 0);
                // begin a new session
                Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
                sdb2.Connect(config.conf.UserName, config.conf.Password);
                Assert.IsNotNull(sdb2.Connection);
                // TODO:
                BsonDocument conf = new BsonDocument("PreferedInstance", "m");
                sdb2.SetSessionAttr(conf);
                // check
                // record the slave note "TotalDataRead" before query
                Sequoiadb sddb = new Sequoiadb(host, port);
                sddb.Connect(config.conf.UserName, config.conf.Password);
                DBCursor cur1 = sddb.GetSnapshot(6, null, null, null);
                BsonDocument status1 = cur1.Next();
                long count1 = status1.GetValue("TotalDataRead").AsInt64;
                // query
                DBCursor cursor = coll.Query(null, null, null, null, 0, -1);
                BsonDocument o = new BsonDocument();
                long count = 0;
                while ((o = cursor.Next()) != null)
                    count++;
                // record the slave note "TotalRead" after query
                DBCursor cur2 = sddb.GetSnapshot(6, null, null, null);
                BsonDocument status2 = cur2.Next();
                long count2 = status2.GetValue("TotalDataRead").AsInt64;
                //Assert.IsTrue(num == count2 - count1);
                long temp = count2 - count1;
                Console.WriteLine("count2 is " + count2 + ", count1 is " + count1);
                DBCursor cur3 = sddb.GetSnapshot(6, null, null, null);
                BsonDocument status3 = cur3.Next();
                long count3 = status3.GetValue("TotalRead").AsInt64;
            }
            finally
            {
                // remove the newly build node
                SequoiaDB.Node node = rg.GetNode(host, port);
                if (node != null)
                {
                    rg.RemoveNode(host, port, new BsonDocument());
                }
            }
        }

        [TestMethod()]
        public void setSessionAttr_Arguments_Test()
        {
            // begin a new session
            Sequoiadb sdb2 = new Sequoiadb(config.conf.Coord.Address);
            sdb2.Connect(config.conf.UserName, config.conf.Password);
            Assert.IsNotNull(sdb2.Connection);
            // TODO:
            BsonDocument conf = null;
            string[] str = { "M", "m", "S", "s", "A", "a" };
            int[] nodeNumber = { 1, 2, 3, 4, 5, 6, 7 };
            Random rnd = new Random();
            int r = rnd.Next(2);
            int n = -1;
            if (r == 0)
            {
                n = rnd.Next(6);
                conf = new BsonDocument("PreferedInstance", str[n]);
            }
            else
            {
                n = rnd.Next(7);
                conf = new BsonDocument("PreferedInstance", nodeNumber[n]);
            }
            try
            {
                sdb2.SetSessionAttr(conf);
            }
            catch (BaseException e) 
            {
                Console.WriteLine(e.ErrorType);
                Assert.Fail();
            }
        }

        [TestMethod()]
        public void setSessionAttr_ingorecase_Test()
        {
            string originalKey = "PreferedInstance";
            string lowercaseKey = "preferedinstance";
            string uppercaseKey = "PREFEREDINSTANCE";
            string useKey;

            Dictionary<BsonValue, BsonValue> caseDic = new Dictionary<BsonValue, BsonValue>();

            string value1 = "M";
            string value2 = "a";
            string value3 = "-A";
            string value4 = "-s";
            int value5 = 1;
            BsonArray value6 = new BsonArray();
            value6.Add(1);
            value6.Add("A");
            BsonArray value7 = new BsonArray();
            value7.Add(1);
            value7.Add("m");
            BsonArray value8 = new BsonArray();
            value8.Add(1);
            value8.Add("-s");
            BsonArray value9 = new BsonArray();
            value9.Add(1);
            value9.Add("-A");

            string expected1 = "M";
            string expected2 = "A";
            string expected3 = "-A";
            string expected4 = "-S";
            int expected5 = 1;
            BsonArray expected6 = new BsonArray();
            expected6.Add(1);
            expected6.Add("A");
            BsonArray expected7 = new BsonArray();
            expected7.Add(1);
            expected7.Add("M");
            BsonArray expected8 = new BsonArray();
            expected8.Add(1);
            expected8.Add("-S");
            BsonArray expected9 = new BsonArray();
            expected9.Add(1);
            expected9.Add("-A");

            caseDic.Add(value1, expected1);
            caseDic.Add(value2, expected2);
            caseDic.Add(value3, expected3);
            caseDic.Add(value4, expected4);
            caseDic.Add(value5, expected5);
            caseDic.Add(value6, expected6);
            caseDic.Add(value7, expected7);
            caseDic.Add(value8, expected8);
            caseDic.Add(value9, expected9);

            int n = 1;
            foreach(KeyValuePair<BsonValue,BsonValue> caseObj in caseDic)
            {
                useKey = ((n++) % 2 == 0) ? lowercaseKey : uppercaseKey;
                BsonDocument attribute = new BsonDocument(useKey, caseObj.Key);
                sdb.SetSessionAttr(attribute);
                BsonElement result = sdb.GetSessionAttr(false).GetElement(originalKey);
                Assert.IsTrue(result.Value == caseObj.Value);
            }
        }

        [TestMethod()]
        public void getSessionAttr_Test()
        {
            // case 1:
            BsonDocument attribute = sdb.GetSessionAttr();
            BsonDocument attribute2 = sdb.GetSessionAttr();
            Assert.IsTrue(attribute == attribute2);
            BsonDocument attribute3 = sdb.GetSessionAttr(false);
            Assert.IsTrue(attribute == attribute3);
            Console.WriteLine(attribute.ToString());
            // case 2:
            sdb.SetSessionAttr(new BsonDocument());
            BsonDocument attribute4 = sdb.GetSessionAttr();
            Assert.IsTrue(attribute3 == attribute4);
        }

        [TestMethod()]
        public void getSessionAttr_data_Test()
        {
            Sequoiadb data = new Sequoiadb(config.conf.Data.Address);
            data.Connect(config.conf.UserName, config.conf.Password);
            Assert.IsNotNull(data.Connection);
            BsonDocument attribute = data.GetSessionAttr();
            Console.WriteLine("attribute is: " + attribute.ToString());
            Assert.IsNotNull(attribute);
            data.Disconnect();
        }

        [TestMethod()]
        public void Sync_DB_Test()
        {
            BsonDocument options = new BsonDocument();
            options.Add("Deep", 1);
            options.Add("Block", true);
            coll.Insert(new BsonDocument("a", 1));
            sdb.Sync(options);
            coll.Insert(new BsonDocument("b", 1));
            sdb.Sync();
        }

        [TestMethod()]
        [Ignore]
        public void KeepAlive_Test()
        {
            Sequoiadb db = null;
            ReplicaGroup rg = null;
            SequoiaDB.Node node = null;
                
            db = new Sequoiadb("192.168.30.121", 11810);
            db.Connect();
            rg = db.GetReplicaGroup("group2");
            node = rg.GetNode("ubuntu-hs03", 11840);
            node.Start();
        }

        [TestMethod()]
        public void Analyze_Test()
        {
            sdb.Analyze();
        }

        [TestMethod()]
        public void Analyze_CL_Test()
        {
            BsonDocument options = new BsonDocument();
            options.Add("Collection", csName + "." + cName);
            sdb.Analyze(options);
        }

        [TestMethod]
        public void Rename_CS_CL_Test()
        {
            String csName = "rename_cs";
            String clName = "rename_cl";

            String newCSName = "new_rename_cs";
            String newCLName = "new_rename_cl";

            if (sdb.IsCollectionSpaceExist(csName)) {
                sdb.DropCollectionSpace(csName);
            }
            if (sdb.IsCollectionSpaceExist(newCSName)) {
                sdb.DropCollectionSpace(newCSName);
            }

            try
            {

                Assert.IsFalse(sdb.IsCollectionSpaceExist(csName));
                Assert.IsFalse(sdb.IsCollectionSpaceExist(newCSName));

                sdb.CreateCollectionSpace(csName).CreateCollection(clName);
                Assert.IsTrue(sdb.IsCollectionSpaceExist(csName));
                sdb.RenameCollectionSpace(csName, newCSName);
                Assert.IsFalse(sdb.IsCollectionSpaceExist(csName));
                Assert.IsTrue(sdb.IsCollectionSpaceExist(newCSName));

                CollectionSpace cs = sdb.GetCollectionSpace(newCSName);
                Assert.IsTrue(cs.IsCollectionExist(clName));
                cs.RenameCollection(clName, newCLName);
                Assert.IsFalse(cs.IsCollectionExist(clName));
                Assert.IsTrue(cs.IsCollectionExist(newCLName));
            }
            finally
            {
                try
                {
                    sdb.DropCollectionSpace(csName);
                }
                catch (Exception e) { }
                try
                {
                    sdb.DropCollectionSpace(newCSName);
                }
                catch (Exception e) { }
            }


        }

        [TestMethod]
        public void Exception_Test()
        {
            string csName = "exception_test";
            try
            {
                CollectionSpace cs = sdb.GetCollectionSpace(csName);
            }
            catch (BaseException e)
            {
                BsonDocument errobj = e.ErrorObject;
                Console.WriteLine("error obj is: {0}", errobj.ToString());
            }

            string clName = "exception_test";
            try
            {
                DBCollection cl = cs.GetCollection(clName);
            }
            catch (BaseException e)
            {
                BsonDocument errobj = e.ErrorObject;
                Console.WriteLine("error obj is: {0}", errobj.ToString());
            }
        }

        [TestMethod()]
        public void GetCollectionSpaceTest()
        {
            string csName = "testCS1";
            CollectionSpace cs = null;
            Sequoiadb sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if (!sdb.IsCollectionSpaceExist(csName))
                cs = sdb.CreateCollectionSpace(csName);
            cs = sdb.GetCollectionSpace(csName);
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
    }
}
