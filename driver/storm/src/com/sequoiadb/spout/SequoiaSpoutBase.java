package com.sequoiadb.spout;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.core.SequoiaObjectGrabber;

import backtype.storm.spout.SpoutOutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.topology.base.BaseRichSpout;
import backtype.storm.tuple.Fields;

@SuppressWarnings("serial")
public abstract class SequoiaSpoutBase extends BaseRichSpout {
	private static final long serialVersionUID = 7592194756963976354L;
	private static Logger LOG = Logger.getLogger(SequoiaSpoutBase.class);
	protected static SequoiaObjectGrabber wholeDocumentMapper = null;

	static {
		wholeDocumentMapper = new SequoiaObjectGrabber() {
			@Override
			public List<Object> map(BSONObject object) {
				List<Object> tuple = new ArrayList<Object>();
				tuple.add(object);
				return tuple;
			}

			@Override
			public String[] fields() {
				return new String[] { "document" };
			}
		};

	}
	private String dbName;
	private BSONObject query;
	protected SequoiaObjectGrabber mapper;
	protected Map<String, SequoiaObjectGrabber> fields;

	protected Map conf;
	protected TopologyContext context;
	protected SpoutOutputCollector collector;

	protected LinkedBlockingQueue<BSONObject> queue = new LinkedBlockingQueue<BSONObject>(
			1024);

	private String host;
	private int port;
	private String userName;
	private String password;
	private SequoiaSpoutTask spoutTask;
	private String[] collectionNames;

	/**
	 * @fn SequoiaSpoutBase(String host, int port, String userName, String
	 *     password, String dbName, String[] collectionNames, BSONObject query,
	 *     SequoiaObjectGrabber mapper)
	 * @brief Constructor
	 * @param host
	 *            The sequoiadb
	 * @param port
	 *            CollectionSpace handle
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
	 * @param mapper
	 *            For mappers bson object to list
	 */
	public SequoiaSpoutBase(String host, int port, String userName,
			String password, String dbName, String[] collectionNames,
			BSONObject query, SequoiaObjectGrabber mapper) {
		this.host = host;
		this.port = port;
		this.userName = userName;
		this.password = password;
		this.dbName = dbName;
		this.collectionNames = collectionNames;
		this.query = query;
		this.mapper = (mapper == null) ? wholeDocumentMapper : mapper;
	}

	@Override
	public void nextTuple() {
		processNextTuple();
	}

	@Override
	public void open(Map conf, TopologyContext context,
			SpoutOutputCollector collector) {
		this.conf = conf;
		this.context = context;
		this.collector = collector;
		
		BSONObject selector = null;
		if (mapper != wholeDocumentMapper) {
			selector = new BasicBSONObject();
			String[] fields = mapper.fields();
			for (String field: fields) {
				selector.put(field, null);
			}
			
			selector.put("_id", null);
		}

		this.spoutTask = new SequoiaSpoutTask(queue, host, port, userName,
				password, dbName, collectionNames, query, selector);

		Thread thread = new Thread(this.spoutTask);
		thread.start();
	}

	@Override
	public void close() {
		this.spoutTask.stopThread();
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer) {
		declarer.declare(new Fields(this.mapper.fields()));
	}

	/**
	 * @fn processNextTuple()
	 * @brief The function be called by nextTuple(), the nextTuple be called for
	 *        get a new tuple by storm.
	 */
	protected abstract void processNextTuple();

	@Override
	public void ack(Object msgId) {

	}

	@Override
	public void fail(Object msgID) {

	}

}
