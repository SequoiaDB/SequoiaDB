package com.sequoiadb.test.common;

public class SdbNest {

    public static String createNestElement(int num) {
        String str = "";
        int i = 0;
        if (i != num) {
            i++;
            str += ("{element:" + createNestElement(num - 1) + "}");
        } else
            return str += "{element:100}";
        return str;
    }

    public static String createNestArray(int num) {
        String arr = "";
        int j = 0;
        if (j != num) {
            j++;
            arr += ("{arr:[" + createNestArray(num - 1) + "]}");
        } else
            return arr += "{arr:[10,20,20]}";
        return arr;
    }

	/*public static void main(String[] args){
        SdbNest nest = new SdbNest();
	    String string2= nest.createNestElement(4);
	    System.out.println(string2);
		String string = nest.createNestArray(4);
		System.out.println(string);
	}*/
}
