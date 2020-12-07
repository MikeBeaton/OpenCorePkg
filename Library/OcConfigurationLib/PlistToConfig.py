#!/usr/bin/env python3

#  Copyright (c) 2020, Mike Beaton. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause

"""
Generate OpenCore .c and .h plist config definition files from template plist file.
"""

import base64
import io
import os
import sys
import xml.etree.ElementTree as ET

from dataclasses import dataclass

# Available flags for -f:

# show markup with implied types added
PRINT_PLIST = 1 << 0
# show creation of plist schema objects
PRINT_PLIST_SCHEMA = 1 << 1
# show creation of OC schema objects
PRINT_OC_SCHEMA = 1 << 2
# show processing steps
PRINT_DEBUG = 1 << 3

flags = 0

# output string buffers
h_types = None
c_structors = None
c_schema = None

# support customisation for other apps
DEFAULT_PREFIX = 'Oc'

camel_prefix = None
upper_prefix = None

SHARED_HEADER = \
'''/** @file
  Copyright (C) 2019-2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/**
  This entire file now automatically generated by PlistToConfig.py from Template.plist
  in OpenCorePkg/Library/OcConfigurationLib, do not edit by hand
**/
'''

C_TEMPLATE = \
'''
#include <Library/OcConfigurationLib.h>

[[BODY]]
EFI_STATUS
[[Prefix]]ConfigurationInit (
  OUT [[PREFIX]]_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  )
{
  BOOLEAN  Success;

  [[PREFIX]]_GLOBAL_CONFIG_CONSTRUCT (Config, sizeof (*Config));
  Success = ParseSerialized (Config, &mRootConfigurationInfo, Buffer, Size);

  if (!Success) {
    [[PREFIX]]_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

VOID
[[Prefix]]ConfigurationFree (
  IN OUT [[PREFIX]]_GLOBAL_CONFIG   *Config
  )
{
  [[PREFIX]]_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
}
'''

H_TEMPLATE = \
'''
#ifndef [[PREFIX]]_CONFIGURATION_LIB_H
#define [[PREFIX]]_CONFIGURATION_LIB_H

#include <Library/DebugLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcBootManagementLib.h>

[[BODY]]/**
  Initialize configuration with plist data.

  @param[out]  Config   Configuration structure.
  @param[in]   Buffer   Configuration buffer in plist format.
  @param[in]   Size     Configuration buffer size.

  @retval  EFI_SUCCESS on success
**/
EFI_STATUS
[[Prefix]]ConfigurationInit (
  OUT [[PREFIX]]_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  );

/**
  Free configuration structure.

  @param[in,out]  Config   Configuration structure.
**/
VOID
[[Prefix]]ConfigurationFree (
  IN OUT [[PREFIX]]_GLOBAL_CONFIG   *Config
  );

#endif // [[PREFIX]]_CONFIGURATION_LIB_H
'''

##
# Errors and IO
#

def error(*args, **kwargs):
  print('ERROR: ', *args, sep='', file=sys.stderr, **kwargs)
  sys.exit(-1)

def internal_error(*args, **kwargs):
  print('INTERNAL_ERROR: ', *args, sep='', file=sys.stderr, **kwargs)
  sys.exit(-1)

def debug(*args, **kwargs):
  if flags & PRINT_DEBUG != 0:
    print('DEBUG: ', *args, sep='', **kwargs)

def info_print(*args, **kwargs):
  if kwargs.pop('info_flags', 0) & flags != 0:
    for _ in range(0, kwargs.pop('tab', 0)):
      print(end='\t')
    print(*args, sep='', **kwargs)

def plist_print(*args, **kwargs):
  info_print(*args, info_flags=PRINT_PLIST, **kwargs)

def plist_schema_print(*args, **kwargs):
  info_print(*args, info_flags=PRINT_PLIST_SCHEMA, **kwargs)

def oc_schema_print(*args, **kwargs):
  info_print(*args, info_flags=PRINT_OC_SCHEMA, **kwargs)

def attr_print(name, value, flags):
  if value is not None:
    info_print(' ', name, '=', info_flags=flags, end='')
    if type(value) is list:
      info_print(value, info_flags=flags, end='')
    else:
      info_print('"', value, '"', info_flags=flags, end='')

def plist_schema_attr_print(name, value):
  attr_print(name, value, PRINT_PLIST_SCHEMA)

def oc_schema_attr_print(name, value):
  attr_print(name, value, PRINT_OC_SCHEMA)

##
# Emit file elements
#

def emit_section_name(key):
  print('/**', file = h_types)
  print('  %s section' % key.value, file = h_types)
  print('**/', file = h_types)
  print(file = h_types)

def upper_path(path):
  return '_'.join(p.upper() for p in path)

def emit_array(elem_array, context):
  context_type = 'ENTRY' if context == 'map' else 'ARRAY'

  upath = upper_path(elem_array.path)
  elem_array.def_name = '%s_%s_%s' % (upper_prefix, upath, context_type)

  print('#define %s_FIELDS(_, __) \\' % elem_array.def_name, file = h_types)
  print('  OC_ARRAY (%s, _, __)' % elem_array.of.def_name, file = h_types)
  print('  OC_DECLARE (%s)' % elem_array.def_name , file = h_types)
  print(file = h_types)

def emit_map(elem_map):
  upath = upper_path(elem_map.path)
  elem_map.def_name = '%s_%s_MAP' % (upper_prefix, upath)

  print('#define %s_FIELDS(_, __) \\' % elem_map.def_name, file = h_types)
  print('  OC_MAP (OC_STRING, %s, _, __)' % elem_map.of.def_name, file = h_types)
  print('  OC_DECLARE (%s)' % elem_map.def_name , file = h_types)
  print(file = h_types)

##
# Schema objects
#

@dataclass
class PlistKey:
  schema_type: str
  value: str
  path_spec: str

  def __init__(
    self,
    value: str = None,
    path_spec: str = None,
    tab: int = 0
    ):

    self.schema_type = 'key'
    self.value = value
    self.path_spec = path_spec

    plist_schema_print('[plist:key', tab=tab, end='')
    plist_schema_attr_print('value', value)
    plist_schema_attr_print('path', path_spec)
    plist_schema_print(']')

    if path_spec is None:
      self.path_spec = value

@dataclass
class OcSchemaElement:
  schema_type: str
  name: str
  path: str
  data_type: str
  size: str
  value: str
  of: object
  # .h file definition name, once emitted
  def_name: str

  def __init__(
    self,
    schema_type: str,
    name: str = None,
    path: str = None,
    size: str = None,
    value: str = None,
    of: object = None,
    def_name: str = None,
    tab: int = 0
    ):

    self.schema_type = schema_type
    self.name = name
    self.path = path
    self.size = size
    self.value = value
    self.of = of
    self.def_name = def_name

    oc_schema_print('[OC:', schema_type, tab=tab, end='')
    oc_schema_attr_print('name', name)
    oc_schema_attr_print('path', path)
    oc_schema_attr_print('size', size)
    oc_schema_attr_print('value', value)
    of_type = None
    if of is not None:
      if type(of) is list:
        of_type = 'list[%d]' % len(of)
      else:
        of_type = of.schema_type
    oc_schema_attr_print('of', of_type)
    oc_schema_attr_print('def_name', def_name)
    oc_schema_print(']')

  def set_name(
    self,
    name,
    tab = 0
    ):

    if self.name is not None:
      internal_error('name should not get set more than once on OcSchemaElement')
    self.name = name
    oc_schema_print('... [name="', name, '"]', tab=tab)

##
# Parsing
#

def plist_open(elem, tab):
  plist_print('<', elem.tag, '>', tab=tab)

def plist_close(elem, tab):
  plist_print('</', elem.tag, '>', tab=tab)

def plist_open_close(elem, tab):
  if elem.text is not None:
    plist_print('<', elem.tag, '>', elem.text, '</', elem.tag, '>', tab=tab)
  else:
    plist_print('<', elem.tag, '/>', tab=tab)

def parse_data(elem, tab, path):
  schema_type = elem.attrib['type'] if 'type' in elem.attrib else None
  size = elem.attrib['size'] if 'size' in elem.attrib else None
  data = elem.text

  if data is not None:
    data_bytes = base64.b64decode(data)
    data_print = '0x' + data_bytes.hex()
  else:
    data_bytes = None
    data_print = None

  if data is not None and (schema_type is None or size is None):
    length = len(data_bytes)

    type_from_data = None

    if length == 2:
      type_from_data = 'uint16'
    elif length == 4:
      type_from_data = 'uint32'
    elif length == 8:
      type_from_data = 'uint64'
    else:
      type_from_data = 'uint8'
      if length != 1 and size is None:
        size = length

    if schema_type is None:
      schema_type = type_from_data

  if schema_type is None:
    if size is None:
      schema_type = 'blob'
    else:
      schema_type = 'uint8'
  elif schema_type == 'blob' and size is not None:
    error('size attribute not valid with schema_type="blob"')

  plist_print('<', elem.tag, tab=tab, end='')
  plist_print(' type="', schema_type, end='')
  if size is not None:
    plist_print('" size="', size, end='')
  plist_print('">', data_print if data_print is not None else '[None]', '</', elem.tag, '>')

  if schema_type == 'blob':
    schema_type = 'oc_data'

  return OcSchemaElement(schema_type=schema_type.upper(), size=size, value=data_print, tab=tab, path=path)

def parse_array(elem, tab, path, key, context):
  plist_open(elem, tab)
  count = len(elem)
  if count == 0:
    error('No template for <array>')
  child = parse_elem(elem[0], tab, path, None)
  if (count > 1):
    plist_print('\t(skipping ', count - 1, ' item', '' if (count - 1) == 1 else 's' , ')', tab=tab)
  plist_close(elem, tab)

  array = OcSchemaElement(schema_type='OC_ARRAY', of=child, tab=tab, path=path)
  emit_array(array, context)

  return array

def init_dict(elem, tab, map):
  displayName = '<' + elem.tag + (' type="map"' if map else '') + '>'

  plist_print(displayName, tab=tab)

  count = len(elem)

  if count == 0:
    error('No elements in ', displayName)

  if count % 2 != 0:
    error('Number of nodes in ', displayName, ' must be even')

  return count >> 1

def check_key(parent, child, index):
  if child.schema_type != 'key':
    error('<key> required as ', 'first' if index == 0 else 'every even' , ' element of <', parent.tag, '>')

def parse_map(elem, tab, path, key):
  count = init_dict(elem, tab, True)

  key = parse_elem(elem[0], tab, path, None)

  check_key(elem, key, 0)

  oc_value = parse_elem(elem[1], tab, path, None, context='map')

  count -= 1

  if (count > 0):
    plist_print('\t(skipping ', count, ' item', '' if count == 1 else 's' , ')', tab=tab)

  plist_close(elem, tab)

  if oc_value.schema_type == 'OC_DATA':
    return OcSchemaElement(schema_type='OC_ASSOC', tab=tab, path=path, def_name = 'OC_ASSOC')
  else:
    elem_map = OcSchemaElement(schema_type='OC_MAP', of=oc_value, tab=tab, path=path)
    emit_map(elem_map)
    return elem_map

def parse_fields(elem, tab, path, key):
  count = init_dict(elem, tab, False)

  fields = []

  index = 0
  while count > 0:
    key = parse_elem(elem[index], tab, path, None)

    check_key(elem, key, index)

    if key.value is None:
      error('<key> tag within <dict> fields template cannot be empty, contents are used as variable name')

    if len(path) == 0:
      emit_section_name(key)

    oc_child = parse_elem(elem[index + 1], tab, path, key.path_spec)

    oc_child.set_name(key.value, tab=tab)

    fields.append(oc_child)

    count -= 1
    index += 2

  plist_close(elem, tab)

  return OcSchemaElement(schema_type='OC_STRUCT', of=fields, tab=tab, path=path)

def parse_plist(elem, tab, path, key):
  plist_open(elem, tab)

  count = len(elem)
  if count != 1:
    error('Invalid contents for <plist>')

  child = parse_elem(elem[0], tab, path, None, indent=False)

  plist_close(elem, tab)

  return child

def parse_key(elem, tab):
  display_name = elem.tag
  path_spec = None

  if 'path' in elem.attrib:
    path_spec = elem.attrib['path']
    display_name += ' path="' + path_spec + '"'

  plist_print('<', display_name, end='', tab=tab)

  if elem.text is not None:
    plist_print('>', elem.text, '</', elem.tag, '>')
  else:
    plist_print('/>')

  return PlistKey(value=elem.text, path_spec=path_spec, tab=tab)

def parse_elem(elem, tab, path, key, indent = True, context = None):
  if tab == None:
    tab = 0

  if indent:
    tab += 1

  if path is None:
    new_path = None
  else:
    new_path = path.copy()
    if key is not None:
      new_path.append(key) # NB modifies list, returns None

  if elem.tag == 'key':
    return parse_key(elem, tab)

  if elem.tag == 'true' or elem.tag == 'false':
    plist_open_close(elem, tab)
    return OcSchemaElement(schema_type='BOOLEAN', value=elem.tag, tab=tab, path=new_path)

  if elem.tag == 'string':
    plist_open_close(elem, tab)
    return OcSchemaElement(schema_type='OC_STRING', value=elem.text, tab=tab, path=new_path, def_name='OC_STRING')

  if elem.tag == 'integer':
    plist_open_close(elem, tab)
    return OcSchemaElement(schema_type='UINT32', value=elem.text, tab=tab, path=new_path)

  if elem.tag == 'data':
    return parse_data(elem, tab, new_path)

  if elem.tag == 'array':
    return parse_array(elem, tab, new_path, None, context)

  if elem.tag == 'dict':
    if 'type' in elem.attrib and elem.attrib['type'] == 'map':
      return parse_map(elem, tab, new_path, key)
    else:
      return parse_fields(elem, tab, new_path, key)

  if elem.tag == 'plist':
    return parse_plist(elem, tab, new_path, key)

  error('Unhandled tag:', elem.tag)

devnull = open(os.devnull, 'w')

def file_close(handle):
  if handle != devnull and handle != sys.stdout:
    debug('Closing: ', handle)
    handle.close()

def twice(flag, handle):
  if handle is not None:
    error(flag, ' specified twice')

def customise_template(template, body):
  #debug('customise_template body= \\\n\'\'\'', body, '\'\'\'')
  return template \
    .replace('[[Prefix]]', camel_prefix) \
    .replace('[[PREFIX]]', upper_prefix) \
    .replace('[[BODY]]', body)

##
# Usage, input args, general init
#

# usage
argc = len(sys.argv)
if argc < 2:
  print('PlistToConfig [-c c-file] [-h h-file] [-f print-flags] [-p prefix] plist-file')
  sys.exit(-1)

# file handles
c_file = None
h_file = None

# string buffers
h_types = io.StringIO()
c_structors = io.StringIO()
c_schema = io.StringIO()

## DEBUG
##print('// h_types', file=h_types)
print('// c_structors', file=c_structors)
print('// c_schema', file=c_schema)

# main template filename
plist_filename = None

# parse args
skip = False
for i in range(1, argc):

  if skip:
    
    skip = False
    continue

  arg = sys.argv[i]
  skip = True

  if arg == '-f' or arg == '-p':

    if i + 1 >= argc:
      error('Missing value for ', arg, ' flag')

    if arg == '-f':

      flags = int(sys.argv[i + 1])
      debug('flags = ', flags)

    elif arg == '-p':

      camel_prefix = sys.argv[i + 1]
      debug('prefix = \'', camel_prefix, '\'')

    else:

      internal_error('flag error')

  elif arg == '-c' or arg == '-h':

    if i + 1 >= argc:
      error('Missing file for ', arg)

    flag_filename = sys.argv[i + 1]

    if flag_filename == '-':
      handle = sys.stdout
    elif flag_filename.startswith('-'):
      error('Missing file for ', arg)
    else:
      handle = open(flag_filename, 'w')

    debug(arg, ' ', handle)

    if arg == '-c':
      twice(arg, c_file)
      c_file = handle
    elif arg == '-h':
      twice(arg, h_file)
      h_file = handle
    else:
      internal_error('file flag error')

  elif not arg.startswith('-'):

    if plist_filename is not None:
      error('\'', arg, '\': too many input files, already using \'', plist_filename, '\'')

    plist_filename = arg
    debug('input: \'', plist_filename, '\'')
    skip = False

  else:

    error('Unknown flag ', arg)

if plist_filename is None:
  error('No input file')

if camel_prefix is None:
  camel_prefix = DEFAULT_PREFIX
upper_prefix = camel_prefix.upper()

if c_file is None:
  c_file = devnull

if h_file is None:
  h_file = devnull

# ElementTree read plist file into memory
debug('Reading plist XML from \'', plist_filename, '\'')
root = ET.parse(plist_filename).getroot()

# process into string buffers
debug('Parsing plist in memory')
parse_elem(root, None, [], None, indent=False)

# write output
c_structors.seek(0)
c_schema.seek(0)
h_types.seek(0)

debug('Writing c file')
print(SHARED_HEADER, file=c_file, end='')
print(customise_template(C_TEMPLATE, c_structors.read() + c_schema.read()), file=c_file, end='')

file_close(c_file)

debug('Writing h file')
print(SHARED_HEADER, file=h_file, end='')
print(customise_template(H_TEMPLATE, h_types.read()), file=h_file, end='')

file_close(h_file)

debug('Done')
sys.exit(0)
