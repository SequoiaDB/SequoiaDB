package com.sequoiadb.bolt;

import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import backtype.storm.task.OutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.tuple.Tuple;
import com.sequoiadb.core.StormSequoiaObjectGrabber;
import com.sequoiadb.core.UpdateQueryCreator;


public class SequoiaUpdateBolt extends SequoiaBoltBase {
	private static final long serialVersionUID = -3179653776895938041L;
	private UpdateQueryCreator  updateQueryCreator;
	private LinkedBlockingQueue<Tuple> queue = new LinkedBlockingQueue<Tuple>(1024);
	private SequoiaBoltTask task;
	private Thread writeThread;
	
	public SequoiaUpdateBolt(String host, int port, String userName,
			String password, String dbName, String collectionName,
			UpdateQueryCreator updateQueryCreator, StormSequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, collectionName, mapper);
		this.updateQueryCreator = updateQueryCreator;
	}
	
	@SuppressWarnings("serial")
	@Override
	public void prepare(Map map, TopologyContext topologyContext, OutputCollector outputCollector) {
		super.prepare(map, topologyContext, outputCollector);
		
		task = new SequoiaBoltTask(queue, sdb, space, collection, updateQueryCreator, mapper) {
			@Override
			public void execute(Tuple tuple) {
				BSONObject updateQuery = this.updateQueryCreator.createQuery(tuple);
				BSONObject mappedUpdateObject = new BasicBSONObject();
				mappedUpdateObject = this.mapper.map(mappedUpdateObject, tuple);
				collection.upsert(updateQuery, mappedUpdateObject, null);
			}
		};
		
		
		writeThread = new Thread(task);
		writeThread.start();
		
	}

	@Override
	public void execute(Tuple tuple) {
		queue.add(tuple);
		this.afterExecuteTuple(tuple);
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer) {
		
	}

	@Override
	public void afterExecuteTuple(Tuple tuple) {
	}

	@Override
	public void cleanup() {
		task.stopThread();
		sdb.disconnect();
	}

}
