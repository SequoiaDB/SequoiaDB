package main;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.bson.BSONObject;

import com.sequoiadb.hadoop.io.BSONWritable;

public class AirlineMapper extends
		Mapper<Object, BSONObject, Text, BSONWritable> {

	private static Log log = LogFactory.getLog(AirlineMapper.class);

	@Override
	public void setup(Context context) {

	}

	public void map(Object key, BSONObject value, Context context)
			throws IOException, InterruptedException {

		// 提取身份证为KEY
		String credentNo = (String) value.get("credent_no");
		if (credentNo != null) {
			context.write(new Text(credentNo), new BSONWritable(
					value));
		}
	}
}
