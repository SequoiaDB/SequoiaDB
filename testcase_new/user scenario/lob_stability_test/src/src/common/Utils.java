package common;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;

public class Utils {
    
    public static List<String> getListFromString(String namesStr) {
        String[] nameArr = namesStr.split(",");
        List<String> nameList = new ArrayList<String>();
        for (int i = 0; i < nameArr.length; ++i) {
            nameList.add(nameArr[i]);
        }
        return nameList;
    }
    
    private static List<String> csNameList = getListFromString(MyProps.get("csNames"));
    private static List<String> clNameList = getListFromString(MyProps.get("clNames"));
    private static Random random = new Random();
    
    public static DBCollection getRandomCL(Sequoiadb db) {
        int csRandIdx = random.nextInt(csNameList.size());
        String csName = csNameList.get(csRandIdx);
        int clRandIdx = random.nextInt(clNameList.size());
        String clName = clNameList.get(clRandIdx);
        return db.getCollectionSpace(csName).getCollection(clName);
    }
    
    private static List<String> norFilePathList = null;
    private static List<String> bigFilePathList = null;
    
    public synchronized static List<String> getUploadFilePathList(boolean isBig) {
        if (null == norFilePathList) {
            final String uploadFilePath = MyProps.get("norUploadFileDir");
            final int writeFileNum = MyProps.getInt("writeFileBulkSize");
            
            norFilePathList = new ArrayList<String>();
            File file = new File(uploadFilePath);
            
            String[] localFileArr = file.list();
            if (localFileArr.length != writeFileNum) {
                throw new RuntimeException("upload file count is wrong. "
                        + "that should be [" + writeFileNum + "]. please check in " + uploadFilePath);
            }
        
            for (int i = 0; i < writeFileNum; ++i) {
                norFilePathList.add(uploadFilePath + localFileArr[i]);
            }
        }
        if (null == bigFilePathList) {
            final String uploadFilePath = MyProps.get("bigUploadFileDir");
            bigFilePathList = new ArrayList<String>();
            File file = new File(uploadFilePath);
            String[] localFileArr = file.list();
            for (int i = 0; i < localFileArr.length; ++i) {
                bigFilePathList.add(uploadFilePath + localFileArr[i]);
            }
        }
        
        if (isBig) {
            return bigFilePathList;
        } else {
            return norFilePathList;
        }
    }
    
    public static List<ObjectId> selectLobOid(DBCollection cl, boolean isBig) {
        long returnRows;
        if (isBig) {
            returnRows = 1;
        } else {
            returnRows = MyProps.getInt("readFileBulkSize");
        }
        BSONObject cond = new BasicBSONObject("isBig", isBig);
        long lobCount = cl.getCount(cond);
        long skipRows = random.nextInt((int) (lobCount - returnRows + 1));
        
        DBCursor cursor = cl.query(cond, null, null, null, skipRows, returnRows);
        List<ObjectId> oidList = new ArrayList<ObjectId>();
        while (cursor.hasNext()) {
            oidList.add((ObjectId) (cursor.getNext().get("oid")));
        }
        cursor.close();
        return oidList;
    }
    
    public static void deleteDir(final String dirStr) {
        File dirFile = new File(dirStr);
        File[] files = dirFile.listFiles();
        for (int i = 0; i < files.length; ++i) {
            files[i].delete();
        }
        dirFile.delete();
    }
    
    public static void putNorLobs(DBCollection cl, List<String> filePathList) throws Exception {
        for (String filePath : filePathList) {
            FileInputStream fis = new FileInputStream(filePath);
            DBLob lob = null;
            try {
                lob = cl.createLob();
                lob.write(fis);
            } finally {
                if (lob != null) {
                    lob.close();
                }
                if (fis != null) {
                    fis.close();
                }
            }
            BSONObject meta = new BasicBSONObject();
            meta.put("isBig", false);
            meta.put("oid", lob.getID());
            cl.insert(meta);
        }
    }
    
    public static void putBigLob(DBCollection cl, List<String> filePathList) throws Exception {
        DBLob lob = cl.createLob();
        ObjectId oid = lob.getID();
        lob.close();
        
        for (int i = 0; i < 3; ++i) {
            int randIdx = random.nextInt(filePathList.size());
            String filePath = filePathList.get(randIdx);
            lobAppend(cl, oid, filePath);
        }
        
        BSONObject meta = new BasicBSONObject();
        meta.put("isBig", true);
        meta.put("oid", lob.getID());
        cl.insert(meta);
    }
    
    private static void lobAppend(DBCollection cl, ObjectId oid, String filePath) throws IOException {
        DBLob lob = null;
        FileInputStream fis = null;
        try {
            lob = cl.openLob(oid, DBLob.SDB_LOB_WRITE);
            fis = new FileInputStream(filePath);
            int fileSize = fis.available();
            lob.lockAndSeek(lob.getSize(), fileSize);
            lob.write(fis);
        } finally {
            if (lob != null) {
                lob.close();
            }
            if (fis != null) {
                fis.close();
            }
        }
    }
    
    public static void readNorLobs(DBCollection cl, List<ObjectId> oidList, String downloadDir) throws FileNotFoundException {
        for (int i = 0; i < oidList.size(); ++i) {
            ObjectId oid = oidList.get(i);
            DBLob lob = null;
            try {
                lob = cl.openLob(oid);
                String downloadPath = downloadDir + oid;
                FileOutputStream fos = new FileOutputStream(downloadPath);
                lob.read(fos);
            } finally {
                if (lob != null) {
                    lob.close();
                }
            }
            
        }
    }
    
    public static void readBigLobs(DBCollection cl, List<ObjectId> oidList, String downloadDir) throws FileNotFoundException {
        for (int i = 0; i < oidList.size(); ++i) {
            ObjectId oid = oidList.get(i);
            DBLob lob = null;
            try {
                lob = cl.openLob(oid, DBLob.SDB_LOB_READ);
                String downloadPath = downloadDir + oid;
                FileOutputStream fos = new FileOutputStream(downloadPath);
                lob.seek((int) (lob.getSize() * 0.3), DBLob.SDB_LOB_SEEK_CUR); // randomly seek read
                lob.read(fos);
                lob.seek((int) (lob.getSize() * 0.7), DBLob.SDB_LOB_SEEK_SET);
                lob.read(fos);
            } finally {
                if (lob != null) {
                    lob.close();
                }
            }
        }
    }
}