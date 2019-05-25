package com.sequoiadb.bolt;

import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.core.StormSequoiaObjectGrabber;

import backtype.storm.task.OutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.tuple.Tuple;

public class SequoiaInsertBolt extends SequoiaBoltBase {

	private LinkedBlockingQueue<Tuple> queue = new LinkedBlockingQueue<Tuple>(
			1024);
	private SequoiaBoltTask task;
	private Thread writeThread;

	public SequoiaInsertBolt(String host, int port, String userName,
			String password, String dbName, String collectionName,
			StormSequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, collectionName, mapper);
	}

	@SuppressWarnings("serial")
	@Override
	public void prepare(Map map, TopologyContext topologyContext,
			OutputCollector outputCollector) {
		task = new SequoiaBoltTask (queue, this.sdb, this.space, this.collection, this.mapper) {
			@Override
			public void execute(Tuple tuple) {
				BSONObject object = new BasicBSONObject();
				collection.insert(mapper.map(object, tuple));
			}
		};
		
		writeThread = new Thread(task);
		writeThread.start();
	}

	@Override
	public void execute(Tuple tuple) {
		queue.add(tuple);
		
		afterExecuteTuple(tuple);
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer) {
	}

	@Override
	public void afterExecuteTuple(Tuple tuple) {
	}

	@Override
	public void cleanup() {
		this.task.stopThread();
		this.sdb.disconnect();

	}

}
