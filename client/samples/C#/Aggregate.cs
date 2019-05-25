/******************************************************************************
 *
 * Name: Aggregate.cs
 * Description: This program demostrates how to use the C#.Net Driver
 *              
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
    class Aggregate
    {
        public static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine("Please give the database server address <IP:Port>");
                Environment.Exit(0);
            }

            // The database server address
            string sdbIP = args[0];
            // The collection space name
            string csName = "sample";
            // The collection name
            string cName = "sample";

            Sequoiadb sdb = new Sequoiadb(sdbIP);

            // connect
            Common.Connect(sdb);
            CollectionSpace cs = Common.GetCollecitonSpace(sdb, csName);
            DBCollection coll = Common.GetColleciton(cs, cName);

            // delete all records from the collection
            BsonDocument bson = new BsonDocument();
            Common.DeleteRecords(coll, bson, bson);


            String[] command = new String[2];
            command[0] = "{$match:{status:\"A\"}}";
            command[1] = "{$group:{_id:\"$cust_id\",amount:{\"$sum\":\"$amount\"},cust_id:{\"$first\":\"$cust_id\"}}}";
            String[] record = new String[4];
            record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}";
            record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}";
            record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}";
            record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}";
            // insert record into database
            for (int i = 0; i < record.Length; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj = BsonDocument.Parse(record[i]);
                Console.WriteLine("Record is: " + obj.ToString());
                coll.Insert(obj);
            }
            List<BsonDocument> list = new List<BsonDocument>();
            for (int i = 0; i < command.Length; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj = BsonDocument.Parse(command[i]);
                list.Add(obj);
            }

            DBCursor cursor = coll.Aggregate(list);
            int count = 0;
            while (null != cursor.Next())
            {
                Console.WriteLine("Result is: " + cursor.Current().ToString());
                String str = cursor.Current().ToString();
                count++;
            }

            Common.Disconnect(sdb);
        }
    }
}
