package com.sequoiadb.spout;

import java.io.Serializable;
import java.util.concurrent.Callable;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import org.apache.log4j.Logger;
import org.bson.BSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SequoiaSpoutTask implements Callable<Boolean>, Serializable,
		Runnable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1481907266524773498L;
	private static Logger LOG = Logger.getLogger(SequoiaSpoutTask.class);

	private LinkedBlockingQueue<BSONObject> queue;
	private Sequoiadb sdb;
	private CollectionSpace space;
	private DBCollection collection;
	private DBCursor cursor;
	private String[] collectionNames;
	private BSONObject query;
	private BSONObject selector;

	private AtomicBoolean running = new AtomicBoolean(true);

	/**
	 * @fn SequoiaSpoutTask(LinkedBlockingQueue<BSONObject> queue, String host,
	 *     int port, String userName, String password, String dbName, String[]
	 *     collectionNames, BSONObject query)
	 * @brief Constructor
	 * @param queue
	 *            The queue for push object
	 * @param host
	 *            The host name.
	 * @param port
	 *            The port for sequoiadb's coord node.
	 * @param userName
	 *            The user name for sequoiadb
	 * @param password
	 *            The password for sequoiadb
	 * @param dbName
	 *            The Collection space name for sequoiadb
	 * @param collectionNames
	 *            The collection name list
	 * @param query
	 *            The query condition
	 */
	public SequoiaSpoutTask(LinkedBlockingQueue<BSONObject> queue, String host,
			int port, String userName, String password, String dbName,
			String[] collectionNames, BSONObject query, BSONObject selector) {
		this.queue = queue;
		this.collectionNames = collectionNames;
		this.query = query;
		this.selector = selector;

		initSequoia(host, port, userName, password, dbName);
	}

	/**
	 * @fn initSequoia(String host, int port, String userName, String password,
	 *     String dbName)
	 * @brief connect to sequoiadb
	 * @param host
	 *            The host name.
	 * @param port
	 *            The port for sequoiadb's coord node.
	 * @param userName
	 *            The user name for sequoiadb
	 * @param password
	 *            The password for sequoiadb
	 * @param dbName
	 *            The Collection space name for sequoiadb
	 */
	private void initSequoia(String host, int port, String userName,
			String password, String dbName) {
		try {
			Sequoiadb sdb = new Sequoiadb(host, port, userName, password);
			space = sdb.getCollectionSpace(dbName);
		} catch (BaseException e) {
			LOG.error("SequoiaDB Exception:", e);
			throw new RuntimeException(e);
		}
	}

	/**
	 * @fn stopThread()
	 * @brief Stop query thread
	 */
	public void stopThread() {
		running.set(false);
	}

	@Override
	public void run() {
		try {
			call();
		} catch (Exception e) {
			LOG.error(e);
		}
	}

	@Override
	public Boolean call() throws Exception {
		try {
			String collectionName = this.collectionNames[0];
			this.collection = space.getCollection(collectionName);
			cursor = this.collection.query(query, selector, null, null);

			while (running.get()) {
				if (cursor.hasNext()) {
					BSONObject obj = cursor.getNext();
					if (LOG.isInfoEnabled()) {
						LOG.info("Fetching a new item from Sequoiadb cursor:"
								+ obj.toString());
					}

					queue.put(obj);
				} else {
					Thread.sleep(50);
				}
			}

		} catch (Exception e) {
			LOG.error("Failed to fetch record from sequoiadb, exception:" + e);
			if (running.get()) {
				throw new RuntimeException(e);
			}
		} finally {
			cursor.close();
			sdb.disconnect();
		}

		return true;
	}
}
