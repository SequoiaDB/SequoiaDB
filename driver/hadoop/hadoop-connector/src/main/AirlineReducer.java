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

	// 大标签分析函数, 目前只支持分析是否为学生
	private String TakeTag(BasicBSONList historyRecord) {

		// 获取第一条记录的出发地和到达地
		BSONObject firstRecord = (BSONObject) historyRecord.get(0);

		// 需要飞行记录大于等于4次才作分析
		if (firstRecord == null || historyRecord.size() < 4) {
			return "unknown";
		}

		// 获取身份证号
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
			// 学生年龄在16岁到28岁之间
			if (age >= 16 && age <= 28) {
				// 遍历所有飞行记录
				int count = 0; // 在假期飞行的次数
				for (int i = 0; i < historyRecord.size(); i++) {
					try {
						//
						BSONObject record = (BSONObject) historyRecord.get(i);
						// 获取飞行时间
						Date flightDate = sdf.parse(record.get("flight_Date")
								.toString());
						// 如果是在寒暑假期,则统计计数一次
						if (vocationJudge(flightDate)) {
							count++;
							continue;
						}
					} catch (ParseException e) {
						e.printStackTrace();
					}
				}
				// 如果寒暑假飞行次数占总飞行次数的90%,则返回学生
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

		// 将所有飞行记录添加到列表
		BasicBSONList historyRecord = new BasicBSONList();
		for (BSONWritable record : values) {
			historyRecord.add(record.getBson());
		}

		// 打标签
		String userType = null; 
		try {
			userType = TakeTag(historyRecord); 
		} catch (Exception e) {
			log.warn("Failed to take tag for record:" + historyRecord.toString(), e);
		}
		userHisRecord.put("role", userType);

		// 将历史记录放在结构中, 方便后续作增量分析
		userHisRecord.put("history", historyRecord);

		// 输出到SequoiaDB
		context.write(null, new BSONWritable(userHisRecord));

	}
}
