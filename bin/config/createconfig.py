#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import getopt
import shutil
import os

def create_config_by_hint(cfg_name, env_string):
    content = ""
    src_name = "%s.yaml"%cfg_name
    dst_name = "%s%s.yaml"%(cfg_name, env_string)
    if os.path.exists(src_name) == True and os.path.exists(dst_name) == False:
        shutil.copyfile(src_name, dst_name)

def create_config(env_string):
    cfg_names = ["cluster", "config", "mysql", "redis", "sdk", "servers", "apps"]
    for cfg_name in cfg_names:
        create_config_by_hint(cfg_name, env_string)

def usage():
    print('usage: createconfig.py -e <environment name>')
    print('   or: createconfig.py --env=<environment name>')

def main():
    env_string = ""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "he:", ["help", "env=" ])
    except getopt.GetoptError:
        print("error")
        usage()
        sys.exit(1)

    # 处理 返回值options是以元组为元素的列表。
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif opt in ("-e", "--env"):
            env_string  = arg

    if env_string == "":
        usage()
        sys.exit(1)
    create_config(env_string)


if __name__ == "__main__":
    main()

