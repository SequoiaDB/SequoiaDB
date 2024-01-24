package com.sequoia.pig;

import java.io.IOException;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.OutputFormat;
import org.apache.hadoop.mapreduce.RecordWriter;
import org.apache.pig.ResourceSchema;
import org.apache.pig.ResourceSchema.ResourceFieldSchema;
import org.apache.pig.ResourceStatistics;
import org.apache.pig.StoreFunc;
import org.apache.pig.StoreMetadata;
import org.apache.pig.data.DataBag;
import org.apache.pig.data.DataType;
import org.apache.pig.data.Tuple;
import org.apache.pig.impl.util.UDFContext;
import org.apache.pig.impl.util.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.hadoop.SequoiaConfigUtil;
import com.sequoiadb.hadoop.SequoiaOutputFormat;
import com.sequoiadb.hadoop.SequoiaRecordWriter;

public class SequoiaWriter extends StoreFunc implements StoreMetadata {

	private static final Log log = LogFactory.getLog(SequoiaWriter.class);

	static final String PIG_OUTPUT_SCHEMA = "sequoia.pig.output.schema";
	static final String PIG_OUTPUT_SCHEMA_UDF_CONTEXT = "sequoia.pig.output.schema.udf_context";

	private String udfContextSignature = null;
	private Configuration config = null;
	private SequoiaRecordWriter recordWriter;
	private String outputConn = null;
	private String outputCsName = null;
	private String outputCName = null;

	private ResourceSchema schema = null;
	private ResourceFieldSchema[] fields;

	public SequoiaWriter() {
		throw new IllegalArgumentException("Undefined schema");
	}

	public SequoiaWriter(String... userScheme) throws ParseException {
		// System.out.println("initial sequoia writer");
		if (!userScheme[0].equals("*") && !userScheme[0].equals("")) {
			throw new IllegalArgumentException("Undefined schema");
		}
	}

	@Override
	public void setStoreFuncUDFContextSignature(String signature) {
		// System.out.println("set store funcUDFContext signature...");
		udfContextSignature = signature;
	}

	@Override
	public String relToAbsPathForStoreLocation(String location, Path curDir)
			throws IOException {
		// System.out.println("relToAbsPathForStoreLocation....");
		log.info("Converting path: " + location + "(curDir: " + curDir + ")");
		return location;
	}

	@Override
	public void checkSchema(ResourceSchema schema) throws IOException {
		// System.out.println("check schema...");
		final Properties properties = UDFContext.getUDFContext()
				.getUDFProperties(this.getClass(),
						new String[] { udfContextSignature });
		properties
				.setProperty(PIG_OUTPUT_SCHEMA_UDF_CONTEXT, schema.toString());
	}

	@Override
	public void setStoreLocation(String location, Job job) throws IOException {
		// System.out.println("set store location...");
		// System.out.println(location);
		config = job.getConfiguration();
		log.info("Store Location Config: " + config + " For URI: " + location);
		if (!location.startsWith("sequoia://"))
			throw new IllegalArgumentException("invalid url");
		parseUrl(location.substring(10));
		SequoiaConfigUtil.setOutputConn(config, outputConn);
		SequoiaConfigUtil.setOutputCollectionspace(config, outputCsName);
		SequoiaConfigUtil.setOutputCollection(config, outputCName);
		final Properties properties = UDFContext.getUDFContext()
				.getUDFProperties(this.getClass(),
						new String[] { udfContextSignature });
		String strSchema = properties
				.getProperty(PIG_OUTPUT_SCHEMA_UDF_CONTEXT);
		if (strSchema == null) {
			throw new IOException("Could not find schema");
		}
		schema = new ResourceSchema(Utils.getSchemaFromString(strSchema));
		fields = schema.getFields();
		config.set(PIG_OUTPUT_SCHEMA, schema.toString());
	}

	@Override
	public OutputFormat getOutputFormat() throws IOException {
		// System.out.println("get output format...");
		final SequoiaOutputFormat format = new SequoiaOutputFormat();
		log.info("OutputFormat... " + format);
		return format;
	}

	@Override
	public void putNext(Tuple tuple) throws IOException {
		// System.out.println("put next...");
		final Configuration config = recordWriter.getContext()
				.getConfiguration();
		final BSONObject obj = new BasicBSONObject();
		System.out.println(tuple);
		byte temp = 0;
		for (int i = 0; i < fields.length; i++) {
			writeField(obj, fields[i], tuple.get(i), temp);
		}
		try {
			if(temp !=0 )
				recordWriter.write(null, obj);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

	}

	@Override
	public void storeSchema(ResourceSchema schema, String location, Job job)
			throws IOException {
	}

	@Override
	public void storeStatistics(ResourceStatistics arg0, String arg1, Job arg2)
			throws IOException {
	}

	@Override
	public void prepareToWrite(RecordWriter writer) throws IOException {
		// System.out.println("prepare to write...");
		recordWriter = (SequoiaRecordWriter) writer;
		log.info("Preparing to write to " + recordWriter);
		if (recordWriter == null)
			throw new IOException("Invalid Record Writer");
	}

	private void parseUrl(String substring) {
		String[] temp = substring.split("/");
		if (temp == null || temp.length != 3)
			throw new IllegalArgumentException("invalid arguments");
		outputConn = temp[0];
		outputCsName = temp[1];
		outputCName = temp[2];
	}

	public void writeField(BSONObject builder,
			ResourceSchema.ResourceFieldSchema field, Object d, byte flag)
			throws IOException {
		// If the field is missing or the value is null, write a null
		if (d == null) {
//			builder.put(field.getName(), d);
			return;
		}
		++flag;
		ResourceSchema s = field.getSchema();
		// Based on the field's type, write it out
		switch (field.getType()) {
		case DataType.INTEGER:
			builder.put(field.getName(), (Integer) d);
			return;

		case DataType.LONG:
			builder.put(field.getName(), (Long) d);
			return;

		case DataType.FLOAT:
			builder.put(field.getName(), (Float) d);
			return;

		case DataType.DOUBLE:
			builder.put(field.getName(), (Double) d);
			return;

		case DataType.BYTEARRAY:
			builder.put(field.getName(), d.toString());
			return;

		case DataType.CHARARRAY:
			builder.put(field.getName(), (String) d);
			return;

			// Given a TUPLE, create a Map so BSONEncoder will eat it
		case DataType.TUPLE:
			if (s == null) {
				throw new IOException("Schemas must be fully specified to use "
						+ "this storage function.  No schema found for field "
						+ field.getName());
			}
			ResourceSchema.ResourceFieldSchema[] fs = s.getFields();
			LinkedHashMap m = new java.util.LinkedHashMap();
			for (int j = 0; j < fs.length; j++) {
				m.put(fs[j].getName(), ((Tuple) d).get(j));
			}
			builder.put(field.getName(), (Map) m);
			return;

			// Given a BAG, create an Array so BSONEnconder will eat it.
		case DataType.BAG:
			if (s == null) {
				throw new IOException("Schemas must be fully specified to use "
						+ "this storage function.  No schema found for field "
						+ field.getName());
			}
			fs = s.getFields();
			if (fs.length != 1 || fs[0].getType() != DataType.TUPLE) {
				throw new IOException("Found a bag without a tuple "
						+ "inside!");
			}
			// Drill down the next level to the tuple's schema.
			s = fs[0].getSchema();
			if (s == null) {
				throw new IOException("Schemas must be fully specified to use "
						+ "this storage function.  No schema found for field "
						+ field.getName());
			}
			fs = s.getFields();

			ArrayList a = new ArrayList<Map>();
			for (Tuple t : (DataBag) d) {
				LinkedHashMap ma = new java.util.LinkedHashMap();
				for (int j = 0; j < fs.length; j++) {
					ma.put(fs[j].getName(), ((Tuple) t).get(j));
				}
				a.add(ma);
			}

			builder.put(field.getName(), a);
			return;
		case DataType.MAP:
			Map map = (Map) d;
			for (Object key : map.keySet()) {
				builder.put(key.toString(), map.get(key));
			}
			return;
		}
	}

}
