# grbl_port_opencm3

# Scope

The first idea of this project was to port the GRBL code to the Nucleo F401RE board, that is a quite cheap microcontroller (~10/15 $), but also quite performing (72MHz 32-bit Cortex-M4 CPU and many other features).
In order to achieve this scope, I chose to use libopencm3 as a base library, concentrating the effort on the development/porting of the application while having a light driver access substrate. 
I used the GRBL version 0.9 as a code base, and then solved some issues to reproduce the same functionality on this microcontroller.

Since the libopencm3 are structured to support many microcontrollers, this porting may be achieved with some effort on other popular microcontrollers. 
And this may be a future development, hopefully with the collaboration of the community.

# License

Being a work derived from the GNU GPLv3 licensed GRBL project, this project uses the same license. 
The authors and owners of every file are mentioned in the header of the file itself.
Please refer to GRBL repository for more information about the authors and owners of GRBL project.
Please refer to libopencm3 repository for more information about the authors and owners of libopencm3 project.

# How to compile?

At the moment this repository has a structure similar to the libopencm3 examples repository, so to compile you may refer to the libopencm3 repository's build instructions.

To summarize: get the repository in a folder (I call it "main folder" from now on), and get the library libopencm3 in "libopencm3" sub-folder using "submodule init".
You should install a gcc arm toolchain and make sure it's used to compile by setting the correct PATH to it. Since the makefiles use GNU compiler tools and linux commands (e.g. "ls"), in order to compile on Windows environment something as MinGW or Cygwin should be used. 
In particular for Windows, download and install:  


1. msys ([click here](http://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.11/MSYS-1.0.11.exe)) or equivalent.
2. Python 2.7 ([click here](http://www.python.org/ftp/python/2.7/python-2.7.msi))
3. arm-none-eabi/arm-elf toolchain (for example this one [here](https://launchpad.net/gcc-arm-embedded))

Set the path for the tools and the toolchain, for example:  
set PATH=C:\msys\1.0\bin\;C:\Python27\;C:\path-to-gcc-arm-none-eabi\bin;

If you are compiling under unix environment you need to point to the gcc arm toolchain and you should be ready to compile.

Then from a shell change directory to the main project folder and type:  

make bin all

, to compile the project and generate the binaries. To clean use the command:  

make clean

# How to install?

For the Nucleo F401RE, if you have the drivers installed, the board should be seen as a mass storage device.
In this case, copy the binary obtained from the compilation into the mass storage and the on board firmware will take care of the rest.

As an alternative, if you are using a Window OS, the application ST-LINK may be used to download the compiled software in flash. In this case, the ELF file may be used.
