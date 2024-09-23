#!/usr/bin/python3

import os.path
import subprocess
import sys


# configuration type
test = "test/evset-attack"

def run_tests(ccfg, tcfg, level, prange, other):
    for period in prange:
        report = "report/evset-attack-%s-%s.dat" %(ccfg, tcfg)
        s_arg    = "%s %s %d %d %d %s" % (ccfg, tcfg, level, period, 100, other)
        print(test + " " + s_arg)
        subprocess.call(test + " " + s_arg + " >> " + report, shell=True)

run_tests("L2_1024x16_RCL",     "list", 2, range( 1*16*1024, 32*16*1024,  8*1024), "16")


