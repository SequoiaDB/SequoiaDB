import oyaml as yaml
from Cheetah.Template import Template
import SCons
import struct
from collections import OrderedDict
import math

action_type_enum_name = "ACTION_TYPE_ENUM"
resource_type_enum_name = "RESOURCE_TYPE_ENUM"
builtin_roles_name = "BUILTIN_ROLES"
cmd_privilege_map_name = "CMD_PRIVILEGE_MAP"
action_type_bits = "ACTION_TYPE_BITS"
resource_bitsets = "RESOURCE_BITSETS"
uint64_mask = (1 << 64) - 1

def actionset_to_bitset_numbers(location, actionset, data):
   typebits = data[action_type_bits]
   num = 0
   for action_type in actionset:
      if action_type not in typebits:
         raise SCons.Errors.StopError("Undefined action type: '" + action_type + 
                                         "' but used in: " + location)
      num |= typebits[action_type]
   out = [0] * data["actionset_numbers"]
   for index in range(len(out)):
      out[index] = num & uint64_mask
      num >>= 64
   return out

def flatten_list( arr ):
   result = []
   stack = [arr]

   while stack:
      item = stack.pop()
      if (isinstance(item, list)):
         stack.extend(item)
      else:
         result.append(item)

   result.reverse()
   return result

def flatten_actions_of_roles(data):
   roles = data[builtin_roles_name]
   typebits = data[action_type_bits]
   for role_name, role in roles.items():
      for privilege in role["privileges"]:
         if privilege["actions"] == "__all__":
            privilege["actions"] = [uint64_mask] * data["actionset_numbers"]
         else:
            privilege["actions"] = actionset_to_bitset_numbers("builtin role " + role_name , flatten_list(privilege["actions"]), data)


def numbers_is_subset( a, b ):
   if ( len(a) != len(b) ):
      return False
   for i in range(len(a)):
      if ( a[i] & b[i] != a[i] ):
         return False
   return True

   

def append_tags(data):
   for cmd, required in data[cmd_privilege_map_name]["values"].items():
      for privilege in required:
         source_from = { "obj": "NONE", "key": "NULL" }
         if isinstance( privilege["resource"], dict ):
            resource_type = privilege["resource"]["type"]
            source_from = privilege["resource"]["from"]
            if privilege["resource"].has_key("tag"):
               tag_name = "AUTH_{cmd}_{tag}".format(cmd=cmd, tag=privilege["resource"]["tag"])
            else:
               tag_name = "AUTH_{cmd}_default".format(cmd=cmd)
         else:
            resource_type = privilege["resource"]
            tag_name = "AUTH_{cmd}_default".format(cmd=cmd)
         tag = {
            "tag_name": tag_name,
            "resource_type": resource_type,
            "actionsets":[],
            "from": source_from
         }
         for actionset in privilege["actionSets"]:
            numbers = actionset_to_bitset_numbers("cmd " + cmd, actionset, data)
            if not numbers_is_subset(numbers, data[resource_bitsets][resource_type]):
               raise SCons.Errors.StopError("Actions: '" + str(actionset) + 
                                         "' used in cmd: " + cmd + ", resource type: " + resource_type + " is invalid")
            tag["actionsets"].append(numbers)
         data["tags"].append(tag)


def parse_yaml(yaml_data):
   data = {
      action_type_enum_name: yaml_data['enums'][action_type_enum_name],
      resource_type_enum_name: yaml_data['enums'][resource_type_enum_name],
      builtin_roles_name: yaml_data[builtin_roles_name],
      cmd_privilege_map_name: yaml_data[cmd_privilege_map_name],
      action_type_bits: OrderedDict(),
      resource_bitsets: OrderedDict(),
      "uint64_mask": uint64_mask,
      "action_num": 0,
      "actionset_numbers": 0,

      # "tags": [
      #    { 
      #       "tag_name": "<name>", 
      #       "resource_type": "<RESOURCE_TYPE_ENUM>",
      #       "actionsets": [
      #          [<bitset numbers>], # actionset 0
      #          [<bitset numbers>], # actionset 1
      #       ]
      #    },
      # ]
      "tags": [],
   }

   index = 0
   for action_type in yaml_data['enums'][action_type_enum_name]["values"]:
      if action_type == "_invalid":
         continue
      data[action_type_bits][action_type] = 1 << index
      index += 1
   data["action_num"] = index + 1
   data["actionset_numbers"] = int(math.ceil(data["action_num"] / 64.0 ))

   for resource_type_name, resource_type_value in yaml_data['enums'][resource_type_enum_name]["values"].items():
      if resource_type_value['extra_data'] == "__all__":
         data[resource_bitsets][resource_type_name] = [uint64_mask] * data["actionset_numbers"]
      else:
         data[resource_bitsets][resource_type_name] = actionset_to_bitset_numbers(resource_type_name, flatten_list(resource_type_value['extra_data']), data)
   
   flatten_actions_of_roles(data)

   append_tags(data)

   return data

def build_action_type_header(target, source, env):
   yaml_file = source[0]
   header_template = source[1]
   header_output = target[0]

   yaml_data = yaml.safe_load(yaml_file.get_contents())
   data = parse_yaml(yaml_data)

   template_content = header_template.get_contents()
   template = Template(template_content, searchList =[data])
   header_code = template.respond()
   with open(str(header_output), 'w') as file:
      file.write(header_code)

   return None

def build_action_type_source(target, source, env):
   yaml_file = source[0]
   source_template = source[1]
   source_output = target[0]
   
   yaml_data = yaml.safe_load(yaml_file.get_contents())
   data = parse_yaml(yaml_data)

   template_content = source_template.get_contents()
   template = Template(template_content, searchList =[data])
   source_code = template.respond()
   with open(str(source_output), 'w') as file:
      file.write(source_code)

   return None
      

def exists(env):
   return True

def generate(env):
   headerBuilder = SCons.Builder.Builder(action = build_action_type_header)
   env.Append(BUILDERS = {'ActionHeaderBuilder': headerBuilder})
   sourceBuilder = SCons.Builder.Builder(action = build_action_type_source)
   env.Append(BUILDERS = {'ActionSourceBuilder': sourceBuilder})
