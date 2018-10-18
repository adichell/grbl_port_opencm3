#!/usr/bin/env python2

import os
import subprocess

def runProcess(exe):    
    p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while(True):
      retcode = p.poll() #returns None while subprocess is running
      line = p.stdout.readline()
      yield line
      if(retcode is not None):
        break

def run_command(command):
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')

##Read the tool path list that has to be modified according to
##the locally installed versions of the used tools.
with open('tools_paths.txt', 'r') as myfile:
    pathlist = [line.rstrip('\n') for line in myfile]

print "Tools paths list: "
print pathlist

with open('flags_combos.txt', 'r') as myfile:
    flags_lines = [line.rstrip('\n') for line in myfile]

print "Flags combos list: "
print flags_lines

os.chdir('..')

##print "Original PATH environment variable: "
##print os.environ["PATH"]
os.environ["PATH"] = os.pathsep.join(pathlist) + os.pathsep + os.environ["PATH"]
print "Modified PATH environment variable: "
print os.environ["PATH"]

dirname = os.path.dirname(__file__)
lib_dirname = os.path.join(dirname, '..\libopencm3\lib')
##print dirname
##print lib_dirname

#Check if the libraries are already compiled
#and choose if recompile just the application
#or everything.
check_lib_flag = 0
for fname in os.listdir(lib_dirname):
    if fname.endswith('.a'):
        check_lib_flag = 1

if check_lib_flag:

    ## Execute various compilations passing defines
    for elem in flags_lines:
        # Clean ...
        externalCommand = 'make clean_grbl'
        result = subprocess.Popen(externalCommand, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
        print "Executing command : " + externalCommand
        print result
        
        # ... and build
        externalCommand = 'make grbl '+elem+' '
        print "Executing command : " + externalCommand
        for line in run_command(externalCommand):
            print line

else:
    externalCommand = 'make clean'
    os.system(externalCommand)
    
    externalCommand = 'make'
    os.system(externalCommand)

print "Compile script execution ended."


    
