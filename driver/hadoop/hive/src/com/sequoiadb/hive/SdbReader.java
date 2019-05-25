package com.sequoiadb.hive;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.regex.Pattern;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hive.ql.plan.ExprNodeColumnDesc;
import org.apache.hadoop.hive.ql.plan.ExprNodeConstantDesc;
import org.apache.hadoop.hive.ql.plan.ExprNodeDesc;
import org.apache.hadoop.hive.ql.plan.ExprNodeGenericFuncDesc;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.mapred.InputSplit;
import org.apache.hadoop.mapred.RecordReader;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

class SequoiaDBRecord {
	private byte[] stream = null;
	private int  length = 0;
	
	public SequoiaDBRecord(byte[] stream, int length) {
		this.stream = stream;
		this.length = length;
	}
	
	public byte[] getStrem() {
		return stream;
	}
	
	public int getLength() {
		return length;
	}
}

class ByteArrayField {
	private byte[] array = null;
	private int startPos = 0;
	private int endPos = 0;

	public ByteArrayField(byte[] array, int startPos, int endPos) {
		this.array = array;
		this.startPos = startPos;
		this.endPos = endPos;
	}

	public int copyFiledtoArray(byte[] destArray, int pos) {
		int length = endPos - startPos;
		for (int i = 0; i < length; i++) {
			destArray[pos + i] = array[this.startPos + i];
		}
		return length;
	}

	public String toString() {
		String str = new String(array, startPos, endPos - startPos);
		return str;
	}
}

public class SdbReader extends Thread implements
		RecordReader<LongWritable, BytesWritable> {
	public static final Log LOG = LogFactory.getLog(SdbReader.class.getName());
	private Sequoiadb sdb = null;
	private DBCursor cursor = null;

	private long returnRecordCount = 0;
	private long recordCount = 0;

	BlockingQueue<SequoiaDBRecord> queue = new ArrayBlockingQueue<SequoiaDBRecord>(1024);

	List<Integer> readColIDs;
	private String[] columnsMap;
	private int[] selectorColIDs;
	private SdbSplit sdbSplit = null;

	private static final Map<String, String> COMP_BSON_TABLE = new HashMap<String, String>();
	private static final Map<String, String> LOGIC_BSON_TABLE = new HashMap<String, String>();
	static {
		COMP_BSON_TABLE.put(
				"org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPEqual",
				"$et");
		COMP_BSON_TABLE.put(
				"org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPLessThan",
				"$lt");
		COMP_BSON_TABLE
				.put("org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPEqualOrLessThan",
						"$lte");
		COMP_BSON_TABLE
				.put("org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPGreaterThan",
						"$gt");
		COMP_BSON_TABLE
				.put("org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPEqualOrGreaterThan",
						"$gte");

		LOGIC_BSON_TABLE
				.put("org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPAnd",
						"$and");
		LOGIC_BSON_TABLE
				.put("org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPNot",
						"$not");
		LOGIC_BSON_TABLE.put(
				"org.apache.hadoop.hive.ql.udf.generic.GenericUDFOPOr", "$or");
	}

	public SdbReader( InputSplit split,
			String[] columns, List<Integer> readColIDs, ExprNodeDesc filterExpr) {
		if (split == null || !(split instanceof SdbSplit)) {
			throw new IllegalArgumentException(
					"The split is not SdbSplit type.");
		}
		this.readColIDs = readColIDs;
		this.columnsMap = columns;
		this.sdbSplit = (SdbSplit) split;
		if (sdbSplit.getSdbAddr() == null) {
			throw new IllegalArgumentException(
					"The split.sdbAddr is null. split=" + sdbSplit.toString());
		}

		sdb = new Sequoiadb(sdbSplit.getSdbAddr().getHost(), sdbSplit
				.getSdbAddr().getPort(), null, null);
		String spaceName=sdbSplit.getCollectionSpaceName();
		String colName=sdbSplit.getCollectionName();
		CollectionSpace space = sdb.getCollectionSpace(spaceName);
		DBCollection collection = space.getCollection(colName);

		recordCount = collection.getCount();

		BSONObject query = null;
		if (filterExpr != null) {
			try {

				query = parserFilterExprToBSON(filterExpr, 0);

			} catch (Exception e) {
				query = null;
			}
		}
		LOG.info("query:" + query);

		BasicBSONObject selector = new BasicBSONObject();
		for (String column : parserReadColumns(columnsMap, readColIDs)) {
			selector.put(column.toLowerCase(), null);
		}
		LOG.info("selector:" + selector);

		selectorColIDs = new int[selector.size()];

		int index = 0;
		for (Entry<String, Object> entry : selector.entrySet()) {
			for (int i = 0; i < this.columnsMap.length; i++) {
				if (columnsMap[i].equalsIgnoreCase(entry.getKey())) {
					LOG.debug("selectorColIDs[" + index + "] = " + i);
					this.selectorColIDs[index++] = i;
					break;
				}
			}
		}

		BSONObject orderBy = null;

		cursor = collection.query(query, selector, orderBy, null, 1);

		this.start();
	}

	private String[] parserReadColumns(String[] columnsMap,
			List<Integer> readColIDs) {

		String[] readColumns = null;
		boolean addAll = (readColIDs.size() == 0);
		if (addAll) {
			readColumns = columnsMap;
		} else {
			readColumns = new String[readColIDs.size()];
			for (int i = 0; i < readColumns.length; i++) {
				readColumns[i] = columnsMap[readColIDs.get(i)];
			}
		}
		for (String f : readColumns) {
			LOG.info("readColumns is " + f);
		}
		return readColumns;
	}

	protected BSONObject parserFilterExprToBSON(ExprNodeDesc filterExpr,
			int level) throws IOException {
		StringBuffer space = new StringBuffer();
		for (int i = 0; i < level * 3; i++) {
			space.append(" ");
		}
		String prexString = space.toString();

		BSONObject bson = new BasicBSONObject();

		if (filterExpr instanceof ExprNodeGenericFuncDesc) {
			ExprNodeGenericFuncDesc funcDesc = (ExprNodeGenericFuncDesc) filterExpr;

			LOG.debug(prexString + "ExprNodeGenericFuncDesc:"
					+ funcDesc.toString());

			String funcName = funcDesc.getGenericUDF().getClass().getName();

			LOG.debug(prexString + "funcName:" + funcName);
			LOG.info(prexString + "funcName:" + funcName);
			for (Entry<String, String> entry : COMP_BSON_TABLE.entrySet()) {
				LOG.debug(entry.getKey());
				LOG.info(entry.getKey());
			}
			if (COMP_BSON_TABLE.containsKey(funcName)) {

				List<String> columnList = new ArrayList<String>();
				List<Object> constantList = new ArrayList<Object>();

				for (ExprNodeDesc nodeDesc : funcDesc.getChildren()) {
					if (nodeDesc instanceof ExprNodeColumnDesc) {
						ExprNodeColumnDesc columnDesc = (ExprNodeColumnDesc) nodeDesc;
						columnList.add(columnDesc.getColumn());
					} else if (nodeDesc instanceof ExprNodeConstantDesc) {
						ExprNodeConstantDesc constantDesc = (ExprNodeConstantDesc) nodeDesc;
						constantList.add(constantDesc.getValue());
					} else if (nodeDesc instanceof ExprNodeGenericFuncDesc) {
						return null;
					}
				}

				BSONObject compObj = new BasicBSONObject();
				if (constantList.size() == 0 && columnList.size() > 1) {
					BSONObject fieldObj = new BasicBSONObject();
					fieldObj.put("$field", columnList.get(1).toUpperCase());

					compObj.put(COMP_BSON_TABLE.get(funcName), fieldObj);
				} else {
					compObj.put(COMP_BSON_TABLE.get(funcName),
							constantList.get(0));
				}

				bson.put(columnList.get(0).toLowerCase(), compObj);

			} else if (LOGIC_BSON_TABLE.containsKey(funcName)) {

				BasicBSONList bsonList = new BasicBSONList();

				for (ExprNodeDesc chileDesc : funcDesc.getChildren()) {
					BSONObject Child = parserFilterExprToBSON(chileDesc,
							level + 1);
					bsonList.add(Child);
				}
				bson.put(LOGIC_BSON_TABLE.get(funcName), bsonList);
			} else if (funcName
					.equals("org.apache.hadoop.hive.ql.udf.generic.GenericUDFIn")) {

				String column = findColumnNameInChildrenNode(funcDesc
						.getChildren());

				BSONObject compObj = new BasicBSONObject();
				BasicBSONList bsonList = new BasicBSONList();
				for (Object value : findValueInChildrenNode(funcDesc
						.getChildren())) {
					bsonList.add(value);
				}
				compObj.put("$in", bsonList);
				bson.put(column, compObj);
			} else if (funcName.equals("org.apache.hadoop.hive.ql.udf.UDFLike")) {

				String column = findColumnNameInChildrenNode(funcDesc
						.getChildren());

				Object value = findValueInChildrenNode(funcDesc.getChildren())
						.get(0);
				if (value instanceof String) {
					String likeRegx = likePatternToRegExp((String) value);
					Pattern pattern = Pattern.compile(likeRegx,
							Pattern.CASE_INSENSITIVE);
					bson.put(column, pattern);
				} else {
					throw new IOException(
							"The like UDF have not string parame:"
									+ funcDesc.toString());
				}

			} else {
				throw new IOException("The current is not support this UDF:"
						+ funcDesc.toString());
			}
		}
		return bson;
	}

	public static String likePatternToRegExp(String likePattern) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < likePattern.length(); i++) {
			char n = likePattern.charAt(i);
			if (n == '\\'
					&& i + 1 < likePattern.length()
					&& (likePattern.charAt(i + 1) == '_' || likePattern
							.charAt(i + 1) == '%')) {
				sb.append(likePattern.charAt(i + 1));
				i++;
				continue;
			}

			if (n == '_') {
				sb.append(".");
			} else if (n == '%') {
				sb.append(".*");
			} else {
				sb.append(Pattern.quote(Character.toString(n)));
			}
		}
		return sb.toString();
	}

	protected String findColumnNameInChildrenNode(
			List<ExprNodeDesc> childrenNodeDesc) {
		for (ExprNodeDesc nodeDesc : childrenNodeDesc) {
			if (nodeDesc instanceof ExprNodeColumnDesc) {
				ExprNodeColumnDesc columnDesc = (ExprNodeColumnDesc) nodeDesc;
				return columnDesc.getColumn();
			}
		}
		return null;
	}

	protected List<Object> findValueInChildrenNode(
			List<ExprNodeDesc> childrenNodeDesc) {
		List<Object> constantList = new ArrayList<Object>();
		for (ExprNodeDesc nodeDesc : childrenNodeDesc) {
			if (nodeDesc instanceof ExprNodeConstantDesc) {
				ExprNodeConstantDesc constantDesc = (ExprNodeConstantDesc) nodeDesc;
				constantList.add(constantDesc.getValue());
			}
		}
		return constantList;
	}

	@Override
	public void close() throws IOException {
		if (cursor != null) {
			cursor.close();
		}

		if (sdb != null) {
			sdb.disconnect();
		}
	}

	@Override
	public LongWritable createKey() {
		return new LongWritable();
	}

	@Override
	public BytesWritable createValue() {
		return new BytesWritable();
	}

	@Override
	public float getProgress() throws IOException {
		return recordCount > 0 ? returnRecordCount / recordCount : 1.0f;
	}

	@Override
	public void run() {
		LOG.info("Start get record thread.");
		while (cursor.hasNextRaw()) {
			try {
				final int TEXT_START_POS = 10;
				byte[] record = cursor.getNextRaw();


				ByteArrayField[] byteArrayRef = new ByteArrayField[this.selectorColIDs.length];


				int startPos = TEXT_START_POS;
				int i = TEXT_START_POS;

				int nFileNum = 0;

				for (; i < record.length - 2; i++) {
					if (record[i] == '|') {
						ByteArrayField ref = new ByteArrayField(record,
								startPos, i);

						byteArrayRef[nFileNum++] = ref;
						startPos = i + 1;
					}
				}


				if (startPos <= i) {
					ByteArrayField ref = new ByteArrayField(record, startPos, i);
					byteArrayRef[nFileNum++] = ref;
				}

				byte[] recordWithAllColumns = new byte[record.length
						- TEXT_START_POS + this.columnsMap.length];
				int pos = 0;
				
				for (i = 0; i < this.columnsMap.length; i++) {
					for (int j = 0; j < this.selectorColIDs.length; j++) {
						if (this.selectorColIDs[j] == i) {

							pos += byteArrayRef[j].copyFiledtoArray(
									recordWithAllColumns, pos);
							break;
						}
						
					}
					if (pos != recordWithAllColumns.length) {
						recordWithAllColumns[pos++] = '|';
					}

					LOG.info("recordWithAllColumns:"+Arrays.toString(recordWithAllColumns));
				}

				this.queue.put(new SequoiaDBRecord(recordWithAllColumns, pos));
				
			} catch (Exception e) {
				LOG.error("Failed to get a record from sequoiadb.", e);
				continue;
			}
		}
		
		try {
			this.queue.put(new SequoiaDBRecord(null, -1));
		} catch (InterruptedException e) {
			LOG.error("Failed to put null flag.", e);
		}
		
		LOG.info("Compeleted get record thread.");
	}

	@Override
	public boolean next(LongWritable keyHolder, BytesWritable valueHolder)
			throws IOException {

		SequoiaDBRecord record;
		try {
			record = this.queue.take();
		} catch (InterruptedException e) {
			LOG.error("Failed to get record from queue.", e);
			throw new IOException(e);
		}
		
		if (record == null || record.getLength() == -1) {
			return false;
		}
		

		valueHolder.set(record.getStrem(), 0,record.getLength());

		returnRecordCount++;

		return true;
	}

	@Override
	public long getPos() throws IOException {
		return 0;
	}
}
