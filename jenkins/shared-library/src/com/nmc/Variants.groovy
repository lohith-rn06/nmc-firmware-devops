package com.nmc

// Single source of truth for card variants inside pipelines (mirrors firmware/variants/*.cmake)
class Variants implements Serializable {
  static final Map<String, Map> ALL = [
    'GVS'     : [arch: 'armhf',   elf: 'ARM, EABI5', kva: 100,  region: 'IEC'],
    'GVL'     : [arch: 'aarch64', elf: 'ARM aarch64', kva: 300,  region: 'IEC'],
    'GVXL'    : [arch: 'aarch64', elf: 'ARM aarch64', kva: 1000, region: 'IEC'],
    'GVXL-NAM': [arch: 'aarch64', elf: 'ARM aarch64', kva: 1000, region: 'UL'],
  ]
  static String arch(String v)       { return lookup(v).arch }
  static String elfMachine(String v) { return lookup(v).elf }
  static List<String> names()        { return ALL.keySet() as List }
  private static Map lookup(String v) {
    if (!ALL.containsKey(v)) { throw new IllegalArgumentException("Unknown NMC variant: ${v}") }
    return ALL[v]
  }
}
