package com.sequoiadb.samples;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class Constants {
	public final static String CS_NAME = "SAMPLE";
	public final static String CL_NAME = "employee";
	public final static String INDEX_NAME = "employee";
	public final static String SQL_CS_NAME = "sqlCS";
	public final static String SQL_CL_NAME = "sqlCL";

	// create english record
	public static BSONObject createEnglishRecord() {
		BSONObject obj = null;
		try {
			obj = new BasicBSONObject();
			BSONObject obj1 = new BasicBSONObject();
			obj.put("name", "Tom");
			obj.put("age", 21);
			obj.put("Id", 0);

			obj1.put("1", "1234");
			obj1.put("2", "5678");

			obj.put("phone", obj1);
		} catch (Exception e) {
			System.out.println("Failed to create english record.");
			e.printStackTrace();
			System.exit(1);
		}
		return obj;
	}

	// create chinese record
	public static BSONObject createChineseRecord() {
		BSONObject obj = null;
		try {
			obj = new BasicBSONObject();
			BSONObject obj1 = new BasicBSONObject();
			obj.put("姓名", "汤姆");
			obj.put("年龄", 31);
			obj.put("Id", 1);
			obj1.put("1", "123456");
			obj1.put("2", "654321");

			obj.put("电话", obj1);
		} catch (Exception e) {
			System.out.println("Failed to create chinese record.");
			e.printStackTrace();
			System.exit(1);
		}
		return obj;
	}

	public static List<BSONObject> createNameList(int listSize) {
		List<BSONObject> list = null;
		if (listSize <= 1) {
			return list;
		}
		try {
			list = new ArrayList<BSONObject>(listSize);

			for (int i = 0; i < listSize; i++) {
				BSONObject obj = new BasicBSONObject();
				BSONObject addressObj = new BasicBSONObject();
				BSONObject phoneObj = new BasicBSONObject();

				addressObj.put("StreetAddress", "21 2nd Street");
				addressObj.put("City", "New York");
				addressObj.put("State", "NY");
				addressObj.put("PostalCode", "11121");

				phoneObj.put("Type", "Home");
				phoneObj.put("Number", "212 555-1234");

				obj.put("FirstName", "John");
				obj.put("LastName", "Smith");
				obj.put("Age", "51");
				obj.put("Id", i+2);
				obj.put("Address", addressObj);
				obj.put("PhoneNumber", phoneObj);

				list.add(obj);
			}
		} catch (Exception e) {
			System.out.println("Failed to create name list record.");
			e.printStackTrace();
			System.exit(1);
		}
		return list;
	}

}
