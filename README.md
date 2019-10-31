# GRBL port using opencm3 libraries <span style="float:right;"><a href="https://travis-ci.org/adichell/grbl_port_opencm3" style="border-bottom:none">![Build Status](https://travis-ci.org/adichell/grbl_port_opencm3.svg?branch=master)</a></span>

# Scope  
  
The first idea of this project was to port the GRBL code to a Cortex-M processor, using the Nucleo F401RE board, that is a cheap microcontroller (~10/15 $), but is also quite performing (84MHz 32-bit Cortex-M4 CPU and many other features).  
In order to achieve this scope, I chose to use libopencm3 as a base library, concentrating the effort on the development/porting of the application while having a light driver access substrate.  
I used the GRBL version 0.9 as a code base since it is a stable and vastily used version of GRBL, and then solved some issues to reproduce the same functionality on this microcontroller.  

Since the libopencm3 are structured to support many microcontrollers, this porting may be achieved with some effort on other popular microcontrollers.  
  
<b>NEWS:</b> Also added support for Nucleo F103RB and F303K8. I have tested many functionalities on a COREXY machine using F401RE and some functionalities with F303K8.
  
# License  
  
Being a work derived from the GNU GPLv3 licensed GRBL project, this project uses the same license.  
The authors and owners of every file are mentioned in the header of the file itself.  
Please refer to [GRBL repository](https://github.com/grbl/grbl) for more information about the authors and owners of GRBL project.  
Please refer to [libopencm3 repository](https://github.com/libopencm3/libopencm3) for more information about the authors and owners of libopencm3 project.  
  
# How to compile?  
  
This repository has a structure similar to the libopencm3 examples repository, it uses a makefile in the main directory.  
  
You should install a gcc arm toolchain and make sure it's used to compile by setting the correct PATH to it.  
Since the makefiles use GNU compiler tools and linux commands (e.g. "ls"), in order to compile on Windows environment something as MinGW or Cygwin should be used.  
As an example, if you are a Windows user, you may download and install:  
  
1. msys ([click here](http://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.11/MSYS-1.0.11.exe)) or equivalent  
2. Python 2.7 ([click here](http://www.python.org/ftp/python/2.7/python-2.7.msi))  
3. arm-none-eabi/arm-elf toolchain (for example this one [here](https://launchpad.net/gcc-arm-embedded))  
  
Set the path for the tools and the toolchain, for example:  
`set PATH=C:\msys\1.0\bin\;C:\Python27\;C:\path-to-gcc-arm-none-eabi\bin;`  
  
If you are compiling under unix environment you need your PATH to point to the gcc arm toolchain Python 2.7, and you should be ready to compile.  
  
Then, to be able to build the application:  
- get the repository in a folder, by cloning it or downloading it from GitHub;  
- change directory to the main project folder  
- get the library opencm3 in "libopencm3" sub-folder using "git submodule init".  
- run "make bin" to compile the project and generate the binaries.  
  
Just the "make" command is sufficient to trigger the build. The build process will start by building the lib opencm3, and then it will build the grbl applications for every microcontroller target present in the repository.  
The grbl application artifacts created by the build process will be stored in a sub-directory "build_dir" in the path of chosen microcontroller board. As an example, for Nucleo F401 the build_dir will be "/grbl_port/stm32/f4/nucleo-f401re/build_dir".  
  
The most important make targets that may be used are:  
  
- "make", to build lib opencm3 and grbl applications in sequence.  
- "make all" or "make build", that are equivalent to "make" by itself.  
- "make bin", to build everything as "make" and generate the binaries file from ELF files of grbl.  
- "make lib", to build only the lib opencm3.  
- "make grbl", to build only the grbl applications and generate binaries. If the lib opencm3 was not built before, this will generate build errors.  
- "make clean", to clean every build artifact both from library and grbl applications.  
- "make clean_grbl", to clean every build artifact of grbl applications only.  

Moreover, to make the build verbose it is possible to use "V=1" after every command specified above.  

# How to install?  
  
At the moment only Nucleo boards are present in the repository, so the installation methods for this boards is detailed here, when there will be other kinds of boards, this section shall be modified.  
  
For Nucleo boards and similar, if you have the ST drivers installed, the board should be seen as a mass storage device.  
In this case, copy the binary obtained from the building process into the mass storage and the on-board firmware should take care of the rest.  
Take care, likely with this procedure only the sector used by the application will be erased. So if you have anything already written in the sectors used by the grbl parameters, the application behaviour is not guaranteed. Likely, it will find that parameters are corrupted and restore them to default values.  
  
As an alternative method to install your application, if you are using a Windows OS, the application ST-LINK may be used to download the compiled software in flash. In this case, the ELF file may be used.  


