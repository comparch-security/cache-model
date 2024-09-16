#!/usr/bin/python3

import os.path
import subprocess
import sys


# configuration type
test = "test/evset-effective"

def run_tests(ccfg, tcfg, level, evrange, csize):
    for evsize in evrange:
        report = "report/evset-effective-%s-%s.dat" %(ccfg, tcfg)
        s_arg    = "%s %s %d %d %d %d" % (ccfg, tcfg, level, evsize, csize, 1000)
        print(test + " " + s_arg)
        subprocess.call(test + " " + s_arg + " >> " + report, shell=True)

run_tests("L2_1024x16",        "list", 2, range(12, 33, 1), 36000)
run_tests("skewed_L2_512x16",  "list", 2, range(12, 99, 3), 20000)
run_tests("skewed_L2_1024x8",  "list", 2, range( 6, 50, 2), 20000)
run_tests("skewed_L2_1024x12", "list", 2, range( 9, 75, 3), 28000)
run_tests("skewed_L2_1024x16", "list", 2, range(12, 99, 3), 36000)
run_tests("skewed_L2_1024x20", "list", 2, range(15,124, 4), 45000)
run_tests("skewed_L2_2048x4",  "list", 2, range( 3, 25, 1), 20000)


