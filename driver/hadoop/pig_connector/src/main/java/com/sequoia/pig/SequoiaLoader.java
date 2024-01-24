package com.sequoia.pig;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.mapreduce.InputFormat;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.RecordReader;
import org.apache.pig.Expression;
import org.apache.pig.LoadFunc;
import org.apache.pig.LoadMetadata;
import org.apache.pig.ResourceSchema;
import org.apache.pig.ResourceSchema.ResourceFieldSchema;
import org.apache.pig.ResourceStatistics;
import org.apache.pig.backend.hadoop.executionengine.mapReduceLayer.PigSplit;
import org.apache.pig.data.BagFactory;
import org.apache.pig.data.DataBag;
import org.apache.pig.data.DataType;
import org.apache.pig.data.Tuple;
import org.apache.pig.data.TupleFactory;
import org.apache.pig.impl.util.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.hadoop.SequoiaConfigUtil;
import com.sequoiadb.hadoop.SequoiaInputFormat;
import com.sequoiadb.hadoop.SequoiaRecordReader;

public class SequoiaLoader extends LoadFunc implements LoadMetadata {
	private static final Log log = LogFactory.getLog(SequoiaLoader.class);
	private TupleFactory tupleFactory = TupleFactory.getInstance();
	private BagFactory bagFactory = BagFactory.getInstance();

	static final String PIG_INPUT_SCHEMA = "sequoia.pig.input.schema";
	static final String PIG_INPUT_SCHEMA_UDF_CONTEXT = "sequoia.pig.input.schema.udf_context";

	private SequoiaRecordReader recordReader = null;
	private String udfContextSignature = null;
	private Configuration config = null;

	private String inputConn = null;
	private String inputCsName = null;
	private String inputCName = null;

	protected ResourceSchema schema = null;
	public ResourceFieldSchema[] fields;

	public SequoiaLoader() {
		throw new IllegalArgumentException("Undefined schema");
	}

	public SequoiaLoader(String userScheme) {
		// System.out.println("initial sequoiaLoader...");
		try {
			schema = new ResourceSchema(Utils.getSchemaFromString(userScheme));
			fields = schema.getFields();
		} catch (Exception e) {
			throw new IllegalArgumentException("Invalid Schema Format");
		}
	}

	@Override
	public void setLocation(String location, Job job) throws IOException {
		// System.out.println("set location...");
		if (!location.startsWith("sequoia://"))
			throw new IllegalArgumentException("invalid url");
		parseUrl(location.substring(10));
		config = job.getConfiguration();
		BSONObject loadFields = new BasicBSONObject();
		for (ResourceFieldSchema temp : fields) {
			loadFields.put(temp.getName(), "");
		}
		System.out.println(loadFields);
		log.info("Load Location Config: " + config + " For URI: " + location);
		SequoiaConfigUtil.setInputConn(config, inputConn);
		SequoiaConfigUtil.setInputCollectionspace(config, inputCsName);
		SequoiaConfigUtil.setInputCollection(config, inputCName);
		SequoiaConfigUtil.setInputFields(config, loadFields);
		config.set(PIG_INPUT_SCHEMA, schema.toString());
	}

	@Override
	public InputFormat getInputFormat() throws IOException {
		// System.out.println("get inputFormat...");
		final SequoiaInputFormat inputFormat = new SequoiaInputFormat();
		log.info("InputFormat..." + inputFormat);
		return inputFormat;
	}

	@Override
	public void prepareToRead(RecordReader reader, PigSplit split)
			throws IOException {
		// System.out.println("prepare to read...");
		this.recordReader = (SequoiaRecordReader) reader;
		log.info("Preparing to read with " + recordReader);
		if (recordReader == null)
			throw new IOException("Invalid Record Reader");
	}

	@Override
	public Tuple getNext() throws IOException {
		// System.out.println("get next...");
		BSONObject val = null;
		try {
			if (!recordReader.nextKeyValue())
				return null;
			val = recordReader.getCurrentValue();
		} catch (Exception e) {
			throw new IOException(e);
		}
		// System.out.println(val.get("name"));
		Tuple t = tupleFactory.newTuple(fields.length);

		for (int i = 0; i < fields.length; i++) {
			t.set(i, readField(val.get(fields[i].getName()), fields[i]));
		}
		return t;
	}

	@Override
	public void setUDFContextSignature(String signature) {
		// System.out.println("set udf context signature...");
		udfContextSignature = signature;
	}

	@Override
	public String[] getPartitionKeys(String arg0, Job arg1) throws IOException {
		return null;
	}

	@Override
	public ResourceSchema getSchema(String arg0, Job arg1) throws IOException {
		// System.out.println("get schema...");
		if (schema != null) {
			return schema;
		}
		return null;
	}

	@Override
	public ResourceStatistics getStatistics(String arg0, Job arg1)
			throws IOException {
		return null;
	}

	private void parseUrl(String substring) {
		String[] temp = substring.split("/");
		if (temp.length != 3)
			throw new IllegalArgumentException("invalid arguments");
		inputConn = temp[0];
		inputCsName = temp[1];
		inputCName = temp[2];
		// System.out.println(inputConn + " " + inputCsName + " " + inputCName);
	}

	@Override
	public void setPartitionFilter(Expression expression) throws IOException {
	}

	public Object readField(Object obj, ResourceFieldSchema field)
			throws IOException {
		if (obj == null)
			return null;

		try {
			if (field == null) {
				return obj;
			}

			switch (field.getType()) {
			case DataType.INTEGER:
				return Integer.parseInt(obj.toString());
			case DataType.LONG:
				return Long.parseLong(obj.toString());
			case DataType.FLOAT:
				return Float.parseFloat(obj.toString());
			case DataType.DOUBLE:
				return Double.parseDouble(obj.toString());
			case DataType.BYTEARRAY:
				return obj;
			case DataType.CHARARRAY:
				return obj.toString();
			case DataType.TUPLE:
				ResourceSchema s = field.getSchema();
				ResourceFieldSchema[] fs = s.getFields();
				Tuple t = tupleFactory.newTuple(fs.length);

				BSONObject val = (BasicBSONObject) obj;

				for (int j = 0; j < fs.length; j++) {
					t.set(j, readField(val.get(fs[j].getName()), fs[j]));
				}

				return t;

			case DataType.BAG:
				s = field.getSchema();
				fs = s.getFields();

				s = fs[0].getSchema();
				fs = s.getFields();

				DataBag bag = bagFactory.newDefaultBag();

				BasicBSONList vals = (BasicBSONList) obj;

				for (int j = 0; j < vals.size(); j++) {
					t = tupleFactory.newTuple(fs.length);
					for (int k = 0; k < fs.length; k++) {
						t.set(k,
								readField(((BasicBSONObject) vals.get(j))
										.get(fs[k].getName()), fs[k]));
					}
					bag.add(t);
				}

				return bag;

			case DataType.MAP:
				s = field.getSchema();
				fs = s != null ? s.getFields() : null;
				BSONObject inputMap = (BasicBSONObject) obj;

				Map outputMap = new HashMap();
				for (String key : inputMap.keySet()) {
					if (fs != null) {
						outputMap.put(key, readField(inputMap.get(key), fs[0]));
					} else {
						outputMap.put(key, readField(inputMap.get(key), null));
					}
				}
				return outputMap;

			default:
				return obj;
			}
		} catch (Exception e) {
			String fieldName = field.getName() == null ? "" : field.getName();
			String type = DataType.genTypeToNameMap().get(field.getType());
			log.warn("Type " + type + " for field " + fieldName
					+ " can not be applied to " + obj.toString()
					+ ".  Returning null.");
			return null;
		}

	}

}
