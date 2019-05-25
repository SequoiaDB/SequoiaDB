package com.sequoiadb.hive;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hive.serde.serdeConstants;
import org.apache.hadoop.hive.serde2.SerDe;
import org.apache.hadoop.hive.serde2.SerDeException;
import org.apache.hadoop.hive.serde2.SerDeStats;
import org.apache.hadoop.hive.serde2.lazy.ByteArrayRef;
import org.apache.hadoop.hive.serde2.lazy.LazyFactory;
import org.apache.hadoop.hive.serde2.lazy.LazySimpleSerDe;
import org.apache.hadoop.hive.serde2.lazy.LazyStruct;
import org.apache.hadoop.hive.serde2.lazy.objectinspector.LazySimpleStructObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.ListObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.MapObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.PrimitiveObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.StandardMapObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.StructField;
import org.apache.hadoop.hive.serde2.objectinspector.StructObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.AbstractPrimitiveObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.JavaTimestampObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.LongObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.TimestampObjectInspector;
import org.apache.hadoop.hive.serde2.typeinfo.ListTypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.MapTypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.PrimitiveTypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.StructTypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.TypeInfo;
import org.apache.hadoop.hive.serde2.typeinfo.TypeInfoFactory;
import org.apache.hadoop.hive.serde2.typeinfo.TypeInfoUtils;
import org.apache.hadoop.io.BooleanWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.hive.serde2.io.DoubleWritable;
import org.apache.hadoop.hive.serde2.io.ByteWritable;
import org.apache.hadoop.hive.serde2.io.TimestampWritable;
import org.apache.hadoop.io.FloatWritable;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.io.MapWritable;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.Symbol;


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

	private final BSONWritable bsonwritable = new BSONWritable ();

	private int fieldCount;
	private ObjectInspector objectInspector;
	private List<String> columnNames;
	public List<TypeInfo> columnTypes;
	private StructTypeInfo docTypeInfo;
	

	private List<Object> row = new ArrayList<Object>();
	
	
	private BufferedWriter out = null;

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

		String colTypesStr = tbl.getProperty(serdeConstants.LIST_COLUMN_TYPES);
		columnTypes = TypeInfoUtils.getTypeInfosFromTypeString(colTypesStr);
		
		docTypeInfo = (StructTypeInfo) TypeInfoFactory.getStructTypeInfo(columnNames, columnTypes);
		objectInspector = TypeInfoUtils.getStandardJavaObjectInspectorFromTypeInfo(docTypeInfo);

		LOG.debug("Exit SdbSerDe::initialize");
	}
	 
	 
	 /*
	  * For SequoiaDB specific types, return the most appropriate java types
	  */
	 private Object deserializeElseType(Object value){
     	if (value instanceof Symbol) {
            return value.toString();
        } else {
            LOG.error("Unable to parse " + value + " for type " + value.getClass());
            return null;
        }
	
	 }
	 
	 /*
	  * Most primitives are include, but some type are specific to SequoiaDB instances
	  */
	 private Object deserializePrimitive(Object value, final PrimitiveTypeInfo typeInfo, String fieldName){
		 switch (typeInfo.getPrimitiveCategory()) {
		 	case BINARY:
                return value;
            case BOOLEAN:
                return value;
            case DOUBLE:
                return ((Number) value).doubleValue();
            case FLOAT:
                return ((Number) value).floatValue();
            case INT:
                return ((Number) value).intValue();
            case LONG:
                return ((Number) value).longValue();
            case SHORT:
                return ((Number) value).shortValue();
            case STRING:
                return value.toString();
            case TIMESTAMP:
			LOG.debug("deserializePrimitive timestamp \n");

			if (value instanceof Date) {
				LOG.debug("value = Date \n");
			    return new Timestamp(((Date) value).getTime());
			} else if (value instanceof BSONTimestamp) {
				LOG.debug("value = BSONTimestamp \n");
			    return new Timestamp((((BSONTimestamp) value).getTime() * 1000L) + (((BSONTimestamp) value).getInc() / 1000));
			} else {
				LOG.debug("value = Others \n");
				LOG.debug("value.class.Name = " + value.getClass().getName() + "\n");
			    return value;
			}
            default:
            	value = deserializeElseType(value);
            	return value;
        }
		
	 }
	 
	 private Object deserializeStruct(final Object value, final StructTypeInfo typeInfo, final String fieldName) {
		 
		 Object t_value = null;
		 ArrayList<String> structNames = typeInfo.getAllStructFieldNames();
         ArrayList<TypeInfo> structTypes = typeInfo.getAllStructFieldTypeInfos();
         
		 List<Object> struct = new ArrayList<Object>(structNames.size());
		 
		 if(value instanceof BSONObject){
			 BSONObject t_obj = (BSONObject) value;
			 
			 for(int i=0; i<structNames.size(); ++i){
				 String bsonFieldName = structNames.get(i);
				 TypeInfo bsonTypeInfo = structTypes.get(i);
				 Object bsonValue = t_obj.get(bsonFieldName);
				 t_value = deserializeField(bsonValue, bsonTypeInfo, bsonFieldName);
				 
				 struct.add(t_value);
			 }
			  
		 }else{
			 LOG.error("Unable to parse " + value + " for type Struct");
			 return null;
		 }
		 return struct;
	 }
	 
	 
	 private Object deserializeMap(final Object value, final MapTypeInfo typeInfo, final String fieldName) {

		 BSONObject t_obj = null;
		 Object t_value = null;
		 TypeInfo mapValueTypeInfo = typeInfo.getMapValueTypeInfo();

		 
		 if(value instanceof BSONObject){
			 t_obj = (BSONObject) value;
			 
			 Set<String> keys = t_obj.keySet();
			 Iterator<String> iter = keys.iterator();
			 
			 while(iter.hasNext()){
				 
				 String bsonkey = iter.next();
				 t_value = t_obj.get(bsonkey);
				 t_value = deserializeField(t_value, mapValueTypeInfo, bsonkey);
				 
				 t_obj.put(bsonkey, t_value);
			 }
			 
			 
		 }else{
			 LOG.error("Unable to parse " + value + " for type Map");
			 return null;
		 }
		 return t_obj.toMap();
		
	 }
	 
	 private Object deserializeList(final Object value, final ListTypeInfo typeInfo, final String fieldName) {

		 BasicBSONList t_list = null;
		 
		 TypeInfo listElemTypeInfo = typeInfo.getListElementTypeInfo();
		 
		 if(value instanceof BasicBSONList){
			 t_list = (BasicBSONList) value;
			 
			 for (int i=0; i<t_list.size(); ++i){
				 t_list.set(i, deserializeField(t_list.get(i), listElemTypeInfo, fieldName)); 
			 }
		 }else{
			 LOG.error("Unable to parse " + value + " for type List");
			 return null;
		 }
		 return t_list.toArray();
	 }
	 
	 private Object deserializeField(Object value, TypeInfo typeInfo, String fieldName){

		 if (value != null){
			 switch (typeInfo.getCategory()){
				 case LIST:
					 value = deserializeList (value, (ListTypeInfo) typeInfo, fieldName);
					 break;
				 case MAP:
					 value =  deserializeMap (value, (MapTypeInfo) typeInfo, fieldName);
					 break;
				 case PRIMITIVE:
					 value = deserializePrimitive (value, (PrimitiveTypeInfo) typeInfo, fieldName);
					 break;
				 case STRUCT:
					 value = deserializeStruct (value, (StructTypeInfo) typeInfo, fieldName); 
					 break;
				 case UNION:
					 LOG.warn("SequoiaDB-hive connector does not support unions.");
					 value = null;
					 break;
				 default:
					 value = deserializeElseType(value);
					 break;
			 }
		 }
		 return value;
	 }
	
	 @Override
	 public Object deserialize(Writable wr) throws SerDeException {
		LOG.debug("Entry SdbSerDe::deserialize");

		if (!(wr instanceof BSONWritable)) {
			throw new SerDeException("Expected BSONWritable, received "
					+ wr.getClass().getName());
		}

		BSONWritable record = (BSONWritable) wr;
		BSONObject t_obj = record.getBson();
		
		List<String> structFieldNames = docTypeInfo.getAllStructFieldNames(); 
		
		Object value = null ;
		row.clear();
        for (String fieldName : structFieldNames) {                                
            LOG.debug("SdbSerDe::deserialize fieldName : " + fieldName);
        	try {                                                                  
                TypeInfo fieldTypeInfo = docTypeInfo.getStructFieldTypeInfo(fieldName);    
                
                
                value = t_obj.get(fieldName);
                value = deserializeField (value, fieldTypeInfo, fieldName);
                 
            } catch (Exception e) {
                LOG.warn("Could not find the appropriate field for name " + fieldName);
                value = null;
            }
            row.add(value);
        }
		
		LOG.debug("SdbSerDe deserialize over");
		return row;
	}

	@Override
	public ObjectInspector getObjectInspector() throws SerDeException {
		return this.objectInspector;
	}

	@Override
	public Class<? extends Writable> getSerializedClass() {
		LOG.debug("Enter SdbSerDe getSerializedClass");
		return BSONWritable.class;
	}

	@SuppressWarnings("deprecation")
	private Object serializePrimitive (final Object obj, 
			final PrimitiveObjectInspector fieldInspector){
		Object value = null;
		switch (fieldInspector.getPrimitiveCategory()) { 
			case TIMESTAMP:
				Timestamp ts = ((JavaTimestampObjectInspector) fieldInspector).getPrimitiveJavaObject(obj);
				if (ts == null){
					value = null;
				}else{
					value = new BSONTimestamp( (int) (ts.getTime() / 1000), (int) ((ts.getTime() % 1000) * 1000) );
				}
				break;
			default:
				value = fieldInspector.getPrimitiveJavaObject(obj);
				break;
		}
		return value;
	}
	
	private Object serializeList (final Object obj, 
			final ListObjectInspector fieldInspector){
		
		BasicBSONList list = new BasicBSONList();
		List<?> field = fieldInspector.getList(obj); 
		
		if (field == null || field.size() == 0){
			return null;
		}
		
		ObjectInspector elemInspector = fieldInspector.getListElementObjectInspector();
		for (Object elem : field) {
			list.add (serializeObject (elem, elemInspector));
		}
		return list;
	}
	
	private Object serializeMap(final Object obj, 
			final MapObjectInspector fieldInspector){
		
		BSONObject bsonobj =  null;
	
		bsonobj = new BasicBSONObject ();

		try{
			ObjectInspector mapValOI = fieldInspector.getMapValueObjectInspector();
	
			
			for (Entry<?, ?> entry : fieldInspector.getMap(obj).entrySet()) {
				String field = entry.getKey().toString();
				Object value = serializeObject(entry.getValue(), mapValOI);
				bsonobj.put(field, value);
			}
		}
		catch(Exception e){
			return null;
		}

		return bsonobj;
	}
	
	private Object serializeStruct (final Object obj, 
			final StructObjectInspector fieldInspector){
		
		BSONObject bsonobj = new BasicBSONObject ();
		
		
		List<? extends StructField> fields = fieldInspector.getAllStructFieldRefs();
		
		for (int i=0; i<fields.size(); ++i){
			StructField structField = fields.get(i); 
			
			ObjectInspector fieldOI = structField.getFieldObjectInspector();
			
			Object fieldObj = fieldInspector.getStructFieldData(obj, structField);
			
			Object value =  serializeObject(fieldObj, fieldOI);
			
			
			if (value != null){
				bsonobj.put(structField.getFieldName(), value);
			}
			
		}
		
		if (bsonobj.toMap().size() == 0)
			return null;
		else
			return bsonobj;
	}
	private Object serializeObject(final Object obj, final ObjectInspector fieldInspector){
		Object value = null;
		switch (fieldInspector.getCategory()){
			case LIST:
				value = serializeList (obj, (ListObjectInspector) fieldInspector);
				break;
			case MAP:
				value = serializeMap (obj, (MapObjectInspector) fieldInspector);
				break;
			case PRIMITIVE:
				value = serializePrimitive (obj, (PrimitiveObjectInspector) fieldInspector);
				break;
			case STRUCT:
				value = serializeStruct(obj, (StructObjectInspector) fieldInspector);
				break;
			case UNION:
			default:
				LOG.error("Cannot serialize " + obj + " of type " + obj.getClass());
				break;
		
		}
		return value;
	}
	
	/*
	 * if value == null
	 *    return -1;
	 * if value != null
	 *    return 1;
	 */
		
		

		

	@Override 
	public Writable serialize(final Object obj, final ObjectInspector inspector)
			throws SerDeException {

		
        
		
		LOG.debug("Enter SdbSerDe serialize ");
		
		final StructObjectInspector structInspector = (StructObjectInspector) inspector;
		final List<? extends StructField> fields = structInspector
				.getAllStructFieldRefs();
		
		LOG.debug("SdbSerDe serialize, fields = " + fields.toString() + "***** fields.size() = " + fields.size());
		if (fields.size() != columnNames.size()) {
			throw new SerDeException(String.format(
					"Required %d columns, received %d.", columnNames.size(),
					fields.size()));
		}

		BSONObject t_obj = new BasicBSONObject ();

		for (int c = 0; c < fields.size(); c++) {
			
			StructField structField = fields.get(c);
			String fieldName = columnNames.get(c);
			

			if (structField != null) {
				ObjectInspector fieldInspector = structField.getFieldObjectInspector();
				Object fieldObj = structInspector.getStructFieldData(obj, structField);
				
				

				
				Object value =  serializeObject(fieldObj, fieldInspector);
				if( value == null){
					continue;
				}
				t_obj.put(fieldName, value );
				
			}
		}
		
		bsonwritable.setBson(t_obj);
		
        
		return bsonwritable;
	}

	@Override
	public SerDeStats getSerDeStats() {

		return null;
	}

}
