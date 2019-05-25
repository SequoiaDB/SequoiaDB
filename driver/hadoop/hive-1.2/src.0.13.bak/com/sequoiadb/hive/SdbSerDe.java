package com.sequoiadb.hive;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hive.serde2.SerDe;
import org.apache.hadoop.hive.serde2.SerDeException;
import org.apache.hadoop.hive.serde2.SerDeStats;
import org.apache.hadoop.hive.serde2.lazy.ByteArrayRef;
import org.apache.hadoop.hive.serde2.lazy.LazyFactory;
import org.apache.hadoop.hive.serde2.lazy.LazySimpleSerDe;
import org.apache.hadoop.hive.serde2.lazy.LazySimpleSerDe.SerDeParameters;
import org.apache.hadoop.hive.serde2.lazy.LazyStruct;
import org.apache.hadoop.hive.serde2.lazy.objectinspector.LazySimpleStructObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.StructField;
import org.apache.hadoop.hive.serde2.objectinspector.StructObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.AbstractPrimitiveObjectInspector;
import org.apache.hadoop.io.BooleanWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.hive.serde2.io.DoubleWritable;
import org.apache.hadoop.hive.serde2.io.ByteWritable;
import org.apache.hadoop.io.FloatWritable;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.io.MapWritable;


public class SdbSerDe implements SerDe {

	public static final Log LOG = LogFactory.getLog(SdbSerDe.class.getName());

	public SdbSerDe() throws SerDeException {
		super();
	}

	static final String HIVE_TYPE_DOUBLE = "double";
	static final String HIVE_TYPE_FLOAT = "float";
	static final String HIVE_TYPE_BOOLEAN = "boolean";
	static final String HIVE_TYPE_BIGINT = "bigint";
	static final String HIVE_TYPE_TINYINT = "tinyint";
	static final String HIVE_TYPE_SMALLINT = "smallint";
	static final String HIVE_TYPE_INT = "int";
	static final String HIVE_TYPE_STRING = "string";

	private SerDeParameters serdeParams;
	private final MapWritable cachedWritable = new MapWritable();

	private int fieldCount;
	private ObjectInspector objectInspector;
	private List<String> columnNames;

	private LazyStruct row;

	@Override
	public void initialize(final Configuration conf, final Properties tbl)
			throws SerDeException {

		LOG.debug("Entry SdbSerDe::initialize");

		final String columnString = tbl
				.getProperty(ConfigurationUtil.COLUMN_MAPPING);
		LOG.info("columnString:"+columnString);
		if (StringUtils.isBlank(columnString)) {
			throw new SerDeException("No column mapping found, use "
					+ ConfigurationUtil.COLUMN_MAPPING);
		}
		final String[] columnNamesArray = ConfigurationUtil
				.getAllColumns(columnString);
		
		fieldCount = columnNamesArray.length;
		columnNames = new ArrayList<String>(columnNamesArray.length);
		columnNames.addAll(Arrays.asList(columnNamesArray));


		
		serdeParams = LazySimpleSerDe.initSerdeParams(conf, tbl, getClass().getName());
		
		LOG.debug("EscapeChar:" + serdeParams.getEscapeChar() +  "getSeparators:" + new String(serdeParams.getSeparators()));
		
		byte[] sparator = new byte[1];
		sparator[0] = '|';
		objectInspector = LazyFactory.createLazyStructInspector(
				serdeParams.getColumnNames(), 
				serdeParams.getColumnTypes(), 
				sparator,
				serdeParams.getNullSequence(), 
				serdeParams.isLastColumnTakesRest(), 
				serdeParams.isEscaped(), 
				serdeParams.getEscapeChar());

		row = new LazyStruct((LazySimpleStructObjectInspector) objectInspector);
		

		LOG.debug("Exit SdbSerDe::initialize");
	}

	@Override
	public Object deserialize(Writable wr) throws SerDeException {
		LOG.debug("Entry SdbSerDe::deserialize");

		if (!(wr instanceof BytesWritable)) {
			throw new SerDeException("Expected BSONWritable, received "
					+ wr.getClass().getName());
		}

		BytesWritable record = (BytesWritable) wr;
		
		ByteArrayRef bytes = new ByteArrayRef();
		bytes.setData(record.getBytes());
		
		row.init(bytes, 0, record.getLength());
		



		return row;
	}

	@Override
	public ObjectInspector getObjectInspector() throws SerDeException {
		return this.objectInspector;
	}

	@Override
	public Class<? extends Writable> getSerializedClass() {
		return ByteWritable.class;
	}

	@Override
	public Writable serialize(final Object obj, final ObjectInspector inspector)
			throws SerDeException {

		final StructObjectInspector structInspector = (StructObjectInspector) inspector;
		final List<? extends StructField> fields = structInspector
				.getAllStructFieldRefs();
		if (fields.size() != columnNames.size()) {
			throw new SerDeException(String.format(
					"Required %d columns, received %d.", columnNames.size(),
					fields.size()));
		}

		cachedWritable.clear();
		for (int c = 0; c < fieldCount; c++) {
			StructField structField = fields.get(c);

			LOG.debug("fieldId=" + c + ",structField=" + structField.toString());

			if (structField != null) {
				final Object field = structInspector.getStructFieldData(obj,
						fields.get(c));

				final AbstractPrimitiveObjectInspector fieldOI = (AbstractPrimitiveObjectInspector) fields
						.get(c).getFieldObjectInspector();

				Writable value = (Writable) fieldOI
						.getPrimitiveWritableObject(field);

				if (value == null) {
					continue;
				}

				LOG.debug("fieldCount=" + fieldCount + ",value="
						+ value.toString());
				if (value instanceof IntWritable) {
					cachedWritable.put(new Text(columnNames.get(c)), value);
				} else if (value instanceof Text) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((Text) value));
				} else if (value instanceof LongWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((LongWritable) value));
				} else if (value instanceof DoubleWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((DoubleWritable) value));
				} else if (value instanceof FloatWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((FloatWritable) value));
				} else if (value instanceof BooleanWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((BooleanWritable) value));
				} else if (value instanceof ByteWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((ByteWritable) value));
				} else if (value instanceof BytesWritable) {
					cachedWritable.put(new Text(columnNames.get(c)),
							((BytesWritable) value));
				} else {
					LOG.warn("fieldCount=" + fieldCount + ",type="
							+ value.getClass().getName());
				}

			}
		}

		return cachedWritable;
	}

	@Override
	public SerDeStats getSerDeStats() {
		return null;
	}

}
