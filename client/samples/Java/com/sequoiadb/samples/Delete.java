package com.sequoiadb.samples;

/******************************************************************************
 * 
 * Name: Delete.java 
 * Description: This program demonstrates how to use the Java Driver to
 * 				delete data in DB
 *				Get more details in API document
 * 
 ******************************************************************************/

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Delete {
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
			System.out.println("Failed to connect to database: " + connString + ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}
		
		if(sdb.isCollectionSpaceExist(Constants.CS_NAME))
			cs = sdb.getCollectionSpace(Constants.CS_NAME);
		else
			cs = sdb.createCollectionSpace(Constants.CS_NAME);
		
		if(cs.isCollectionExist(Constants.CL_NAME))
			cl = cs.getCollection(Constants.CL_NAME);
		else
			cl = cs.createCollection(Constants.CL_NAME);	
		
		// create the delete condition
		BSONObject condition = new BasicBSONObject();
		condition.put("Id", 0 );
		try {
			cl.delete(condition);
			// if you want to delete all the data in current collection, you can use like: cl.delete(null)
			// or
			// cl.delete("{'Id':0}");
		} catch(BaseException e) {
			System.out.println("Failed to delete data, condition: " + condition);
			e.printStackTrace();
		}
		System.out.println("Delete data successfully");
		
		sdb.disconnect();
	}

}
