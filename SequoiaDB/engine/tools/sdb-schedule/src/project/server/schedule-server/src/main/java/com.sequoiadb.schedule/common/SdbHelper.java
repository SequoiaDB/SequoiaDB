/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = SdbHelper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.model.SdbCLFullInfo;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class SdbHelper {
    private static Logger logger = LoggerFactory.getLogger(SdbHelper.class);

    public static void truncateCL(Sequoiadb sdb, String clFullName) {
        if (isClExist(sdb, clFullName)) {
            SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clFullName);
            sdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName())
                    .truncate(new BasicBSONObject("SkipRecycleBin", true));
        }
    }

    public static Set<String> getCSList(Sequoiadb sdb, List<String> csRegexList) {
        Set<String> res = new HashSet<>();
        String regexStr = String.join("|", csRegexList);
        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_COLLECTIONSPACES,
                new BasicBSONObject("Name", new BasicBSONObject("$regex", regexStr)),
                new BasicBSONObject("Name", 1), null)) {
            while (cursor.hasNext()) {
                BasicBSONObject obj = (BasicBSONObject) cursor.getNext();
                String csName = obj.getString("Name");
                res.add(csName);
            }
        }
        return res;
    }

    public static void attachCl(Sequoiadb sdb, String mainCL, String subClName, BSONObject lowBound,
            BSONObject upBound) throws ScheduleServerException {
        BSONObject options = new BasicBSONObject();
        options.put("LowBound", lowBound);
        options.put("UpBound", upBound);
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(mainCL);
        DBCollection mainCollection = sdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                .getCollection(sdbCLFullInfo.getClName());
        try {
            mainCollection.attachCollection(subClName, options);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_RELINK_SUB_CL.getErrorCode()) {
                throw new ScheduleSystemException("failed to attach collection [" + subClName + "] to main collection: [" + mainCL + "]", e);
            }
        }
    }

    public static void renameCl(Sequoiadb sdb, String clName, String newClName) {
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clName);
        CollectionSpace collectionSpace = sdb.getCollectionSpace(sdbCLFullInfo.getCsName());
        SdbCLFullInfo newClFullInfo = new SdbCLFullInfo(newClName);
        collectionSpace.renameCollection(sdbCLFullInfo.getClName(), newClFullInfo.getClName());
    }

    public static Set<String> getCLList(Sequoiadb sdb, List<String> clRegexList) {
        Set<String> res = new HashSet<>();
        String regexStr = String.join("|", clRegexList);
        BSONObject matcher = new BasicBSONObject("Name", new BasicBSONObject("$regex", regexStr));
        BasicBSONList notList = new BasicBSONList();
        notList.add(new BasicBSONObject("Name", new BasicBSONObject("$regex", "_data_switch_bak_\\d+$")));
        matcher.put("$not", notList);

        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_COLLECTIONS, matcher,
                new BasicBSONObject("Name", 1), null)) {
            while (cursor.hasNext()) {
                BasicBSONObject obj = (BasicBSONObject) cursor.getNext();
                String fullName = obj.getString("Name");
                res.add(fullName);
            }
        }
        return res;
    }

    public static BSONObject getCataSnapshotByClName(Sequoiadb sdb, String clName) {
        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject("Name", clName), null, null)) {
            if (cursor.hasNext()) {
                return cursor.getNext();
            }
            return null;
        }
    }

    public static BasicBSONList getCLSnapshot(Sequoiadb sdb, String clFullName) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put("RawData", true);
        matcher.put("Name", clFullName);

        BSONObject selector = new BasicBSONObject();
        selector.put("Name", 1);
        selector.put("UniqueID", 1);
        selector.put("CollectionSpace", 1);
        selector.put("Details.NodeName", 1);
        selector.put("Details.GroupName", 1);
        // record
        selector.put("Details.TotalRecords", 1);
        selector.put("Details.TotalDataFreeSpace", 1);
        selector.put("Details.TotalDataPages", 1);
        selector.put("Details.DataCommitLSN", 1);
        // lob
        selector.put("Details.TotalLobs", 1);
        selector.put("Details.TotalLobPages", 1);
        selector.put("Details.LobCommitLSN", 1);
        selector.put("Details.TotalValidLobSize", 1);

        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS, matcher, selector,
                null)) {
            if (!cursor.hasNext()) {
                return null;
            }
            BasicBSONList snapshots = new BasicBSONList();
            while (cursor.hasNext()) {
                snapshots.add(cursor.getNext());
            }
            return snapshots;
        }
    }

    public static void detachCl(Sequoiadb sdb, String mainCL, String subClName) {
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(mainCL);
        DBCollection mainCollection = sdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                .getCollection(sdbCLFullInfo.getClName());
        try {
            mainCollection.detachCollection(subClName);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_INVALID_SUB_CL.getErrorCode()) {
                throw e;
            }
        }
    }

    public static BSONObject getAttachInfo(Sequoiadb sdb, String mainCl, String subClName) {
        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject("Name", mainCl), null, null)) {
            if (cursor.hasNext()) {
                BSONObject mainClCataInfo = cursor.getNext();
                BasicBSONList cataInfo = BsonUtils.getArray(mainClCataInfo, "CataInfo");
                for (Object o : cataInfo) {
                    BSONObject cata = (BSONObject) o;
                    String subCl = BsonUtils.getString(cata, "SubCLName");
                    if (subClName.equals(subCl)) {
                        return cata;
                    }
                }
            }
            return null;
        }
    }

    public static boolean isClExist(Sequoiadb sdb, String clName) {
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clName);
        if (!sdb.isCollectionSpaceExist(sdbCLFullInfo.getCsName())) {
            return false;
        }
        return sdb.getCollectionSpace(sdbCLFullInfo.getCsName()).isCollectionExist(sdbCLFullInfo.getClName());
    }

    public static void setClRepairCheck(DBCollection cl) throws ScheduleServerException {
        try {
            cl.alterCollection(new BasicBSONObject("RepairCheck", true));
        }
        catch (Exception e) {
            throw new ScheduleSystemException("failed to set RepairCheck for collection: " + cl.getFullName(), e);
        }
    }

    public static BSONObject getCSSnapshot(Sequoiadb sdb, String csName) {
        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject("Name", csName), null, null)) {
            if (cursor.hasNext()) {
                return cursor.getNext();
            }
            return null;
        }
    }

    public static void unSetClRepairCheck(DBCollection cl) throws ScheduleServerException {
        try {
            cl.alterCollection(new BasicBSONObject("RepairCheck", false));
        }
        catch (Exception e) {
            throw new ScheduleSystemException("failed to unset RepairCheck for collection: " + cl.getFullName(), e);
        }
    }

    public static BSONObject getSequencesSnapshot(Sequoiadb sdb, String sequenceName) {
        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SEQUENCES,
                new BasicBSONObject("Name", sequenceName), null, null)) {
            if (cursor.hasNext()) {
                return cursor.getNext();
            }
            return null;
        }
    }
}
