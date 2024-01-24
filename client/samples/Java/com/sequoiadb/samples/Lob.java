package com.sequoiadb.samples;

/******************************************************************************
 * 
 * Name: Lob.java 
 * Description: This program demonstrates how to use the Java Driver to
 *              operate on lob in DB
 *              Get more details in API document
 * 
 ******************************************************************************/

import org.bson.types.ObjectId;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Lob {
    public static void main(String[] args) {
        if (args.length != 1) {
            if (args.length != 1) {
                System.out
                        .println("Please give the database server address <IP:Port>");
                System.exit(0);
            }
        }

        // the database server address
        String connString = args[0];
        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection cl = null;

        try {
            sdb = new Sequoiadb(connString, "", "");
        } catch (BaseException e) {
            System.out.println("Failed to connect to database: " + connString
                    + ", error description" + e.getErrorType());
            e.printStackTrace();
            System.exit(1);
        }

        if (sdb.isCollectionSpaceExist(Constants.CS_NAME))
            cs = sdb.getCollectionSpace(Constants.CS_NAME);
        else
            cs = sdb.createCollectionSpace(Constants.CS_NAME);

        if (cs.isCollectionExist(Constants.CL_NAME))
            cl = cs.getCollection(Constants.CL_NAME);
        else
            cl = cs.createCollection(Constants.CL_NAME);

        try {
            // create a lob and write some data("HelloWorld")
            DBLob lob = cl.createLob();
            String testData = "HelloWorld";
            lob.write(testData.getBytes());
            System.out.println("write:" + testData);
            lob.close();
            
            ObjectId id = lob.getID();
            
            // read the data from an exist lob with id
            lob = cl.openLob(id);
            byte[] readData = new byte[20];
            int readLen = lob.read(readData);
            lob.close();
            
            String result = new String( readData, 0, readLen );
            System.out.println("read:" + result);
            
        } catch (BaseException e) {
            e.printStackTrace();
        }

        sdb.disconnect();
    }

}
