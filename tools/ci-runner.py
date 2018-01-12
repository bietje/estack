#
# E/STACK - Continious integration runner
#
# Author: Michel Megens
# Date:   12/01/2018
# Email:  dev@bietje.net
#

import sys
import yaml
import os

def parse(config):
    with open(config) as f:
        data = f.read()

    data = yaml.load(data)
    data = data['estack']['script']
    for cmd in data:
        os.system(cmd)

def check_args():
    if len(sys.argv) != 2:
        print("Usage: %s <ci-config-file>" % sys.argv[0])
        sys.exit(-1)


def main():
    check_args()
    configfile = sys.argv[1]
    parse(configfile)


if __name__ == "__main__":
    main()
