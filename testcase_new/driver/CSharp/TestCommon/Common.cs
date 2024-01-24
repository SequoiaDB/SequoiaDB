using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.TestCommon
{
    public class Common
    {
        public static int CompareBson(BsonDocument x, BsonDocument y)
        {
            return x.CompareTo(y);
        }

        public static bool IsStandalone(Sequoiadb sdb)
        {
            try
            {
             sdb.ListReplicaGroups();
          }
            catch (BaseException e)
            {
                if (e.ErrorCode == -159) // -159: The operation is for coord node only
                    return true;
         }
            return false;
      }

        public static List<string> getDataGroupNames(Sequoiadb sdb)
        {
            List<string> list = new List<string>();
            BsonDocument matcher = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            matcher.Add("Role", 0);
            selector.Add("GroupName", "");
            DBCursor cursor = sdb.GetList(7, matcher, selector, null);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                BsonElement element = doc.GetElement("GroupName");
                list.Add(element.Value.ToString());
            }
            cursor.Close();
            return list;
        }

        /// <summary>
        /// judge whether two BsonDocuments equal. 
        /// this func can adapt disorder key.
        /// </summary>
        public static bool IsEqual(BsonDocument a, BsonDocument b)
        {
            if (a.ElementCount != b.ElementCount)
                return false;
            for (int i = 0; i < a.ElementCount; ++i)
            {
                String name = a.ElementAt(i).Name;
                if (!b.Contains(name))
                    return false;
                if (!a.GetElement(name).Equals(b.GetElement(name)))
                    return false;
            }
            return true;
        }

        public static bool IsEqual(List<BsonDocument> a, List<BsonDocument> b)
        {
            if (a.Count != b.Count)
                return false;
            for (int i = 0; i < a.Count; ++i)
            {
                if (!IsEqual(a.ElementAt(i), b.ElementAt(i)))
                    return false;
            }
            return true;
        }
    }
}
