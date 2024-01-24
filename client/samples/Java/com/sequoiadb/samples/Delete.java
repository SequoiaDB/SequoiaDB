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

import com.sequoiadb.base.result.DeleteResult;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Delete {
	public static void main(String[] args) {
		if (args.length != 1) {
			System.out
					.println("Please give the database server address <IP:Port>");
			System.exit(1);
		}

		// the database server address
		String connString = args[0];
		Sequoiadb sdb = new Sequoiadb(connString, "", "");
		try {
			DBCollection cl = Constants.getCL(sdb);

			BSONObject matcher = new BasicBSONObject();
			matcher.put("Id", 0 );

			DeleteResult result = cl.deleteRecords(matcher);
			System.out.println(result);
		} catch (BaseException e) {
			e.printStackTrace();
		} finally {
			sdb.close();
		}
	}

}
