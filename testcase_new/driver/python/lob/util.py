import string
from random import Random


def random_str(length=1024):
   s = ""
   r = Random()
   for i in range(length):
      s += r.choice(string.ascii_letters + string.digits)
   return s


def get_md5(text):
   import hashlib
   #  m2=hashlib.md5()
   #  m2.update(text)
   if isinstance(text, str):
      m2 = text.encode("utf-8")
   else:
      m2 = text
   return hashlib.sha256(m2).hexdigest()
