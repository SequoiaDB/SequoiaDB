/******************************************************************************
 *
 * Name: BulkInsert.cs
 * Description: This program demostrates how to use the C#.Net Driver
 *              This program will also populate some testing data and create
 *              indexes
 * Notice:
 *              Please add reference of 'sequoiadb.dll' in your project.
 *
 * ****************************************************************************/

// Please import namespace SequoiaDB and SequoiaDB.Bson
using SequoiaDB;
using SequoiaDB.Bson;
using System;
using System.Collections.Generic;


namespace Sample
{
    class BulkInsert
    {
        /* create english record */
        static BsonDocument CreateEnglisthRecord()
        {
            BsonDocument obj = new BsonDocument();
            try
            {
                obj.Add("name", "tom");
                obj.Add("age", 60);
                obj.Add("id", 2000);

                // an embedded bson object
                BsonDocument phone = new BsonDocument
                {
                    {"0", "1808835242"},
                    {"1", "1835923246"}
                };

                obj.Add("phone", phone);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to create record.");
                Console.WriteLine(e.Message);
                Environment.Exit(0);
            }

            return obj;
        }

        static BsonDocument CreateChineseRecord()
        {
            BsonDocument obj = new BsonDocument();
            try
            {
                obj.Add("姓名", "杰克");
                obj.Add("年龄", 70);
                obj.Add("id", 2001);

                // an embedded bson object
                BsonDocument phone = new BsonDocument
                {
                    {"0", "1808835242"},
                    {"1", "1835923246"}
                };

                obj.Add("电话", phone);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to create record.");
                Console.WriteLine(e.Message);
                Environment.Exit(0);
            }

            return obj;
        }

        static void BullInsertTest(DBCollection cl)
        {
            int times = 1;
            int bulkSize = 5000;
            for (int i = 0; i < times; i++)
            {
                List<BsonDocument> list = new List<BsonDocument>(bulkSize);
                for (int j = 0; j < bulkSize; j++)
                {
                    BsonDocument obj = new BsonDocument();
                    obj.Add("bbs", "725").
                            Add("csbh", 1817).
                            Add("cljg", "工作状态").
                            Add("sjym", "79H").
                            Add("wxbs", "WX1558").
                            Add("dmzbs", "DMZ2206").
                            Add("cxbz", 0).
                            Add("sjsj", new DateTime()).
                            Add("rksj", new DateTime());
                    list.Add(obj);
                }
                DateTime beginTime = DateTime.Now;
                cl.BulkInsert(list, 0);
                DateTime endTime = DateTime.Now;
                System.TimeSpan takes = endTime - beginTime;
                Console.WriteLine(String.Format("Times: {0}, tasks: {1}ms", i, takes.TotalMilliseconds));
            }
        }

        public static void Main(string[] args)
        {
            // The database server address
            string sdbIP = "192.168.30.54:11810";
            // The collection space name
            string csName = "test";
            // The collection name
            string cName = "test";

            Sequoiadb sdb = new Sequoiadb(sdbIP);

            Common.Connect(sdb);
            CollectionSpace cs = Common.GetCollecitonSpace(sdb, csName);
            DBCollection cl = Common.GetColleciton(cs, cName);

            BullInsertTest(cl);
          
            Console.WriteLine("Successfully inserted records into database");
            Common.Disconnect(sdb);
        }
    }
}
