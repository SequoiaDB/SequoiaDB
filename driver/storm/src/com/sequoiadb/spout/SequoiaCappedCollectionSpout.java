package com.sequoiadb.spout;

import java.io.Serializable;
import java.util.List;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.core.SequoiaObjectGrabber;

public class SequoiaCappedCollectionSpout extends SequoiaSpoutBase implements
		Serializable {
	private static final long serialVersionUID = 6488454364748419501L;
	private static Logger LOG = Logger.getLogger(SequoiaCappedCollectionSpout.class);

	public SequoiaCappedCollectionSpout(String host, int port, String userName,
			String password, String dbName, String collectionName,
			BSONObject query, SequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, new String[]{collectionName}, query, mapper);
	}
	
	public SequoiaCappedCollectionSpout(String host, int port, String userName,
			String password, String dbName, String collectionName) {
		super(host, port, userName, password, dbName, new String[]{collectionName}, null, null);
	}
	
	public SequoiaCappedCollectionSpout(String host, int port, String userName,
			String password, String dbName, String collectionName, SequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, new String[]{collectionName}, null, mapper);
	}

	@Override
	protected void processNextTuple() {
		BSONObject object = queue.poll();
		
		//if we have an object, Let's process it, map and emit it
		if (object != null ) {
			//Map the object to a tuple
			List<Object> tuples = this.mapper.map(object);
			
			//Fetch the object Id
			ObjectId objectId = (ObjectId) object.get("_id");
			
			//Emit the tuple collection
			collector.emit(tuples, objectId);
		}
	}

}
