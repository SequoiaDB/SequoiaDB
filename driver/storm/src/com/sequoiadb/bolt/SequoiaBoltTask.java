package com.sequoiadb.bolt;

import java.io.Serializable;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import org.apache.log4j.Logger;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.core.StormSequoiaObjectGrabber;
import com.sequoiadb.core.UpdateQueryCreator;

import backtype.storm.tuple.Tuple;

public abstract class SequoiaBoltTask implements Runnable, Serializable {

	private static final long serialVersionUID = 2443155668413540015L;
	private static Logger LOG = Logger.getLogger(SequoiaBoltTask.class);
	private AtomicBoolean running = new AtomicBoolean(true);

	protected LinkedBlockingQueue<Tuple> queue;
	protected Sequoiadb sdb;
	protected CollectionSpace space;
	protected DBCollection collection;
	protected UpdateQueryCreator updateQueryCreator;
	protected StormSequoiaObjectGrabber mapper;

	/**
	 * @fn SequoiaBoltTask(LinkedBlockingQueue<Tuple> queue, Sequoiadb sdb,
	 *     CollectionSpace space, DBCollection collection,
	 *     StormSequoiaObjectGrabber mapper)
	 * @brief Constructor
	 * @param queue
	 *            The queue for save object inserted
	 * @param sdb
	 *            sequoiadb connection
	 * @param space
	 *            sequoiadb collection space
	 * @param collection
	 *            sequoiadb collection
	 * @param mapper
	 *            The mapper for map tuple to bson object
	 **/
	public SequoiaBoltTask(LinkedBlockingQueue<Tuple> queue, Sequoiadb sdb,
			CollectionSpace space, DBCollection collection,
			StormSequoiaObjectGrabber mapper) {
		this.queue = queue;
		this.sdb = sdb;
		this.space = space;
		this.collection = collection;
		this.mapper = mapper;
	}

	/**
	 * @fn SequoiaBoltTask(LinkedBlockingQueue<Tuple> queue, Sequoiadb sdb,
	 *     CollectionSpace space, DBCollection collection,
	 *     StormSequoiaObjectGrabber mapper)
	 * @brief Constructor
	 * @param queue
	 *            The queue for save object inserted
	 * @param sdb
	 *            sequoiadb connection
	 * @param space
	 *            sequoiadb collection space
	 * @param collection
	 *            sequoiadb collection
	 * @param updateQueryCreator
	 *            The creator for create query condition by tuple
	 * @param mapper
	 *            The mapper for map tuple to bson object
	 **/
	public SequoiaBoltTask(LinkedBlockingQueue<Tuple> queue, Sequoiadb sdb,
			CollectionSpace space, DBCollection collection,
			UpdateQueryCreator updateQueryCreator,
			StormSequoiaObjectGrabber mapper) {
		this.queue = queue;
		this.sdb = sdb;
		this.space = space;
		this.collection = collection;
		this.mapper = mapper;
		this.updateQueryCreator = updateQueryCreator;
	}

	/**
	 * @fn stopThread()
	 * @brief stop task's thread
	 **/
	public void stopThread() {
		running.set(false);
	}

	@Override
	public void run() {
		while (running.get()) {
			try {
				Tuple tuple = queue.poll();

				if (tuple != null) {
					execute(tuple);
				} else {
					Thread.sleep(50);
				}
			} catch (Exception e) {
				LOG.error("Failed with exception:" + e);
				if (running.get()) {
					throw new RuntimeException(e);
				}
			}
		}
	}

	/**
	 * @fn execute(Tuple tuple)
	 * @brief abstract function, for execute insert/update operator
	 **/
	protected abstract void execute(Tuple tuple);

}
