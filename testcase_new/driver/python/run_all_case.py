import unittest
import os
import xmlrunner

case_path = os.path.join(os.getcwd(),"")
report_path = "report"
tmp_case_path0 = os.path.split(case_path)[0]
tmp_case_path1 = os.path.split(case_path)[1]
pattern_param='*.py'
if tmp_case_path1.endswith('.py'):
   pattern_param = tmp_case_path1
   case_path = tmp_case_path0
   
print (pattern_param)
print (case_path)

def all_case():
   discover = unittest.defaultTestLoader.discover(case_path, pattern=pattern_param, top_level_dir=None)
   print(discover)
   return discover

if __name__ == "__main__":
   runner = xmlrunner.XMLTestRunner(output=report_path)
   test_result = runner.run(all_case())