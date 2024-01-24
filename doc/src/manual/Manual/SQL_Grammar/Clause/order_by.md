
order by 用于根据指定的字段对结果集进行排序，默认为升序。

##语法##
***order by \<field1_name [asc|desc ], ...\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field1_name | string | 字段名  | 是 |
| asc/desc | string | 排序，asc 表示升序，desc 表示降序，默认为 asc  | 是 |

##返回值##
无 

##示例##

   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { emp_no: 1, dept_no: 1 }
   { emp_no: 2, dept_no: 1 }
   { emp_no: 3, dept_no: 2 }
   { emp_no: 4, dept_no: 2 }
   ```

   * 希望按部门计算员工数，并按部门号降序输出 

   ```lang-javascript
   > db.exec( "select dept_no,count(emp_no) as 员工总数 from sample.employee group by dept_no order by dept_no desc" )
   { "dept_no": 2, "员工总数": 2 }
   { "dept_no": 1, "员工总数": 2 }
   Return 2 row(s).
   ```
   >**Note:**
   >
   >像 sum、count、min、max 和 avg 这样的聚合函数必须使用别名。
