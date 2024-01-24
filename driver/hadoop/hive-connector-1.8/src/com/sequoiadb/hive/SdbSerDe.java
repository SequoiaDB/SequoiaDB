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
		// TODO Auto-generated constructor stub
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
	//private String[] columnTypesArray;

	private LazyStruct row;

	@Override
	public void initialize(final Configuration conf, final Properties tbl)
			throws SerDeException {

		LOG.debug("Entry SdbSerDe::initialize");

		final String columnString = tbl
				.getProperty(ConfigurationUtil.COLUMN_MAPPING);
		if (StringUtils.isBlank(columnString)) {
			throw new SerDeException("No column mapping found, use "
					+ ConfigurationUtil.COLUMN_MAPPING);
		}
		final String[] columnNamesArray = ConfigurationUtil
				.getAllColumns(columnString);
		fieldCount = columnNamesArray.length;
		columnNames = new ArrayList<String>(columnNamesArray.length);
		columnNames.addAll(Arrays.asList(columnNamesArray));

		// log.debug("column names in mongo collection: " + columnNames);

//		String hiveColumnNameProperty = tbl.getProperty(Constants.LIST_COLUMNS);
//		List<String> hiveColumnNameArray = new ArrayList<String>();
//
//		if (hiveColumnNameProperty != null
//				&& hiveColumnNameProperty.length() > 0) {
//			hiveColumnNameArray = Arrays.asList(hiveColumnNameProperty
//					.split("[,:;]"));
//		}
//		LOG.debug("column names in hive table: " + hiveColumnNamay);
//
//		String columnTypeProperty = tbl
//				.getProperty(Constants.LIST_COLUMN_TYPES);
//		 System.err.println("column types:" + columnTypeProperty);
//		columnTypesArray = columnTypeProperty.split("[,:;]");
//		LOG.debug("column types in hive table: " + columnTypesArray);
		
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
		
//		final List<ObjectInspector> fieldOIs = new ArrayList<ObjectInspector>(
//				columnNamesArray.length);
//		for (int i = 0; i < columnNamesArray.length; i++) {
//			if (HIVE_TYPE_INT.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaIntObjectInspector);
//			} else if (HIVE_TYPE_SMALLINT.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaShortObjectInspector);
//			} else if (HIVE_TYPE_TINYINT.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaByteObjectInspector);
//			} else if (HIVE_TYPE_BIGINT.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaLongObjectInspector);
//			} else if (HIVE_TYPE_BOOLEAN.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaBooleanObjectInspector);
//			} else if (HIVE_TYPE_FLOAT.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaFloatObjectInspector);
//			} else if (HIVE_TYPE_DOUBLE.equalsIgnoreCase(columnTypesArray[i])) {
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaDoubleObjectInspector);
//			} else {
//				// treat as string
//				fieldOIs.add(PrimitiveObjectInspectorFactory.javaStringObjectInspector);
//			}
//		}
//		objectInspector = ObjectInspectorFactory
//				.getStandardStructObjectInspector(hiveColumnNameArray, fieldOIs);

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
		

		// ********************************************************************************
		// if (!(wr instanceof BSONWritable)) {
		// throw new SerDeException("Expected BSONWritable, received "
		// + wr.getClass().getName());
		// }
		//
		// ((BSONWritable) wr).setFieldType(this.columnNames,
		// this.columnTypesArray);
		//
		// // row.init((BSONWritable) wr, columnNames);
		//
		// LOG.debug("Exit SdbSerDe::deserialize");
		// return wr;

		// ********************************************************************************
		// if (!(wr instanceof MapWritable)) {
		// throw new SerDeException("Expected MapWritable, received "
		// + wr.getClass().getName());
		// }
		//
		// MapWritable input = (MapWritable) wr;
		//
		//
		// final Text t = new Text();
		// row.clear();
		//
		// for (int i = 0; i < fieldCount; i++) {
		//
		// t.set(columnNames.get(i));
		// final Writable value = input.get(t);
		//
		// if (value != null && !NullWritable.get().equals(value)) {
		// // parse as double to avoid NumberFormatException...
		// // TODO:need more test,especially for type 'bigint'
		// if (HIVE_TYPE_INT.equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof IntWritable) {
		// row.add(((IntWritable) value).get());
		// }
		// } else if (HIVE_TYPE_SMALLINT
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof IntWritable) {
		// row.add(((IntWritable) value).get());
		// }
		// } else if (HIVE_TYPE_TINYINT
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof IntWritable) {
		// row.add(((IntWritable) value).get());
		// }
		// } else if (HIVE_TYPE_BIGINT
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// row.add(Long.valueOf(value.toString()));
		// } else if (HIVE_TYPE_BOOLEAN
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof BooleanWritable) {
		// row.add(((BooleanWritable) value).get());
		// }
		// } else if (HIVE_TYPE_FLOAT
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof FloatWritable) {
		// row.add(((FloatWritable) value).get());
		// }
		// } else if (HIVE_TYPE_DOUBLE
		// .equalsIgnoreCase(columnTypesArray[i])) {
		// if (value instanceof DoubleWritable) {
		// row.add(((DoubleWritable) value).get());
		// }
		// } else if (HIVE_TYPE_STRING
		// .equalsIgnoreCase(columnTypesArray[i])){
		// if (value instanceof Text) {
		// row.add(((Text) value).toString());
		// }
		// }
		//
		// } else {
		// row.add(null);
		// }
		// }
		// ********************************************************************************
		// System.out.println("Exit SdbSerDe::deserialize");

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

		// System.out.println("Entry SdbSerDe::serialize");
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

				// TODO:currently only support hive primitive type
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
		// System.out.println("Exit SdbSerDe::serialize");

		return cachedWritable;
	}

	@Override
	public SerDeStats getSerDeStats() {
		// TODO Auto-generated method stub
		return null;
	}

}
