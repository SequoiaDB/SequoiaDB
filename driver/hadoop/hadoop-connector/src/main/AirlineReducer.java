package main;

import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Reducer;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.hadoop.io.BSONWritable;

public class AirlineReducer extends
		Reducer<Text, BSONWritable, Text, BSONWritable> {
	private static Log log = LogFactory.getLog(AirlineReducer.class);
	private SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM");

	private String TakeTag(BasicBSONList historyRecord) {

		BSONObject firstRecord = (BSONObject) historyRecord.get(0);

		if (firstRecord == null || historyRecord.size() < 4) {
			return "unknown";
		}

		String idNo = firstRecord.get("credent_no").toString();
		if (idNo != "" && idNo.length() == 18) {
			int year = 0;
			try {
				year = Integer.valueOf(idNo.substring(6, 10));
			} catch (Exception e) {
				log.warn("The record's birth year is error:" + idNo.substring(6, 10), e);
				return "unknown";
			}
			int age = new Date().getYear() - year;
			if (age >= 16 && age <= 28) {
				int count = 0; // 在假期飞行的次数
				for (int i = 0; i < historyRecord.size(); i++) {
					try {
						BSONObject record = (BSONObject) historyRecord.get(i);
						Date flightDate = sdf.parse(record.get("flight_Date")
								.toString());
						if (vocationJudge(flightDate)) {
							count++;
							continue;
						}
					} catch (ParseException e) {
						e.printStackTrace();
					}
				}
				if ((float) count / historyRecord.size() > 0.9) {
					return "student";
				}
			}

		}

		return "unknown";
	}

	private boolean vocationJudge(Date date) {
		boolean flag = false;
		if ((date.getMonth() >= 7 && date.getMonth() <= 9)
				|| (date.getMonth() >= 1 && date.getMonth() <= 2))
			flag = true;
		return flag;
	}

	public void reduce(Text key, Iterable<BSONWritable> values, Context context)
			throws IOException, InterruptedException {

		BSONObject userHisRecord = new BasicBSONObject();
		userHisRecord.put("credent_no", key.toString());

		BasicBSONList historyRecord = new BasicBSONList();
		for (BSONWritable record : values) {
			historyRecord.add(record.getBson());
		}

		String userType = null; 
		try {
			userType = TakeTag(historyRecord); 
		} catch (Exception e) {
			log.warn("Failed to take tag for record:" + historyRecord.toString(), e);
		}
		userHisRecord.put("role", userType);

		userHisRecord.put("history", historyRecord);

		context.write(null, new BSONWritable(userHisRecord));

	}
}
