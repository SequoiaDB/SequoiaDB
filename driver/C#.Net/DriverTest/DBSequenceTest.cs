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

namespace DriverTest
{
    [TestClass()]
    public class DBSequenceTest
    {
        Sequoiadb sdb = null;
        private TestContext testContextInstance;
        private static Config config = null;
        private static String seqName = "test_seq";
        private static String seqNewName = "test_new_seq";

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
            if (config == null)
                config = new Config();
        }

        [ClassCleanup()]
        public static void MyClassCleanup()
        {
        }

        [TestInitialize()]
        public void MyTestInitialize()
        {
            try
            {
                sdb = new Sequoiadb(config.conf.Coord.Address);
                sdb.Connect(config.conf.UserName, config.conf.Password);
            }
            catch (BaseException e)
            {
                Console.WriteLine("Failed to Initialize in DBSequenceTest, ErrorType = {0}", e.ErrorType);
                Environment.Exit(0);
            }
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            clearSequence(seqName);
            clearSequence(seqNewName);
            sdb.Disconnect();
        }
        #endregion

        private void clearSequence(String seqName)
        {
            BsonDocument match = new BsonDocument("Name", seqName);
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, match, null, null);
            if (cursor.Next() != null)
            {
                sdb.DropSequence(seqName);
            }
            cursor.Close();
        }

        [TestMethod()]
        public void TestSequoiadbSeqAPI()
        {
            String key = "StartValue";
            long value = 100;
            BsonDocument options = new BsonDocument(key, value);
            BsonDocument match = new BsonDocument("Name", seqName);
            sdb.CreateSequence(seqName, options);
            BsonDocument obj = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, match, null, null).Next();
            BsonElement actualValue = obj.GetElement(key);
            Assert.AreEqual(value, actualValue.Value);

            sdb.RenameSequence(seqName, seqNewName);
            DBSequence seq = sdb.GetSequence(seqNewName);
            Assert.AreEqual(seqNewName, seq.Name);
            sdb.DropSequence(seqNewName);
        }

        [TestMethod()]
        public void TestSequenceAPI()
        {
            String key = "StartValue";
            long value = 100;
            BsonDocument options = new BsonDocument(key, value);
            DBSequence seq = sdb.CreateSequence(seqName, options);
            Assert.AreEqual(value, seq.GetNextValue());

            seq.SetCurrentValue(200);
            Assert.AreEqual(200, seq.GetCurrentValue());

            seq.Restart(300);
            Assert.AreEqual(300, seq.GetNextValue());

            options.Add("CurrentValue", 400);
            seq.SetAttributes(options);
            BsonDocument obj = seq.Fetch(3);
            Assert.AreEqual(401, (long)obj.GetElement("NextValue").Value);
            Assert.AreEqual(3, obj.GetElement("ReturnNum").Value);
            Assert.AreEqual(1, obj.GetElement("Increment").Value);
        }
    }
}
