#!/usr/bin/env python2

import os

##Read the tool path list that has to be modified according to
##the locally installed versions of the used tools.
with open('tools_paths.txt', 'r') as myfile:
    pathlist = [line.rstrip('\n') for line in myfile]

print "Tools paths list: "
print pathlist

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
    externalCommand = 'make clean_grbl'
    os.system(externalCommand)

    externalCommand = 'make grbl'
    os.system(externalCommand)
else:
    externalCommand = 'make clean'
    os.system(externalCommand)
    
    externalCommand = 'make'
    os.system(externalCommand)

print "Compile script execution ended."


