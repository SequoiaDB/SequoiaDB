package com.sequoiadb.samples;

/******************************************************************************
 * 
 * Name: Snapshot.java 
 * Description: This program demonstrates how to use the Java Driver to get
 * 				database snapshot ( for other types of * snapshots/lists,
 *				the steps are very similar )
 *				Get more details in API document
 * 
 ******************************************************************************/
import java.io.IOException;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Snapshot {
	public static void main(String[] args) throws IOException {
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
		DBCursor cursor = null;

		try {
			sdb = new Sequoiadb(connString, "", "");
		} catch (BaseException e) {
			System.out.println("Failed to connect to database: " + connString
					+ ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}
		
		cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS,
				new BasicBSONObject(), null, null);
		try {
			while (cursor.hasNext())
				System.out.println(cursor.getNext());
		} finally {
			cursor.close();
		}
		sdb.disconnect();
	}
}
