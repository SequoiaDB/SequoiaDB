
group by 用于结合合计函数，根据一个或多个字段名对结果集进行分组。

##语法##
***group by \<field_name1 [asc/desc], [field_name2 [asc/desc], ...]\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| field_name1 | string | 字段名 | 是 |
| field_name2 | string | 字段名 | 否 |
| asc/desc | string | 排列顺序，asc 表示升序，desc 表示降序，默认为升序 | 否 |
> **Note:**
>
> sum、count、min、max、avg等计数函数必须与别名配合使用。

##返回值##

无

##示例##

   * 集合 sample.employee 中记录如下

   ```lang-json
   { "dept_no": 1, "name": "tom", "emp_no": 10001 }
   { "dept_no": 1, "name": "james", "emp_no": 10002 }
   { "dept_no": 1, "name": "lily", "emp_no": 10003 }
   { "dept_no": 2, "name": "sam", "emp_no": 20001 }
   { "dept_no": 2, "name": "mark", "emp_no": 20002 }
   ```

   * 计算每个部门的员工数，并按字段名 dept_no 分组

   ```lang-javascript
   > db.exec("select dept_no, count(emp_no) as 员工总数 from sample.employee group by dept_no")
   { "dept_no": 1, "员工总数": 3 }
   { "dept_no": 2, "员工总数": 2 }
   Return 2 row(s).
   ```
