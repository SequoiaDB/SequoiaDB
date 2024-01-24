using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Cluster
{
    public class Common
    {
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

        public static bool isStandalone(Sequoiadb sdb)
        {
            bool flag = false;
            try
            {
                sdb.ListReplicaGroups();
            }
            catch (BaseException e)
            {
                if (e.ErrorCode == -159)
                {
                    flag = true;
                }
            }
            return flag;
        }
    }
}
