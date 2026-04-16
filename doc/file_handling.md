# File Handling Refactor for Flashing Tool

In the current implementation, file handling is done in-place in read/write/verify functions, and only .bin files are supported. In order to add support for .elf files (and possibly in the future .hex or other types of files), we will implement a new class in it's own translation unit which creates an abstraction layer for writing to and reading from program binaries.

## BinaryFileHandler Class

The class must provide the following functionality:

1. Retrieve the file size
1. Reset to the 'top' of the file (file.seekg(0) for ifstream) with a helper function call
1. Read out a chunk of some size argument sequentially from a file to an argument buffer. The read function will be called in a sequential loop.
1. Write an argument buffer of some size argument sequentially to a file. 
1. Have a getter to expose file type so that minimal changes to the existing flasher loops can be made to integrate this.
1. Have any additional file type-specific functions that the user might need to call for a specific file type, such as block address retrieval for .elf files so the writer can verify the target fits, and block address setting for writing .elf. The class should have internal tracking of the type and throw errors if the core functions are called without sufficient information, like lack of a start address for .elf. 

It is **imperative** that we do not write the whole file into a RAM buffer. The file handling tool must stream the binary contents to and from the file without a large RAM buffer containing the whole file. It should be able to therefore handle arbitrarily large .bin files.

## Integration

- This should integrate as seamlessly as possible with the existing dartt_flasher loops. 

- Do add the .elf specific checks described in the flashing_tool.md spec. 
	- We can do a one-time check for application address alignment in main.cpp on init, with the exception for the `--no-application` argument. I would like the program to print a helpful hint if that check fails, like:
	```C
	printf("Error: file start address does not match the target start address 0x%lX. Did you mean to use --no-application?\n" (unsigned long)app_start);
	```
	- You may add a public getter to expose app_start and flash_start to accomplish this 

- Don't get too crazy with formatting. I vastly prefer:

		```C
		if(thing)	//good
		{
			return result;
		}
		```
	to:

		```C
		if(thing) //will make me angry if i see it too often
			return result;
		```
	I do this in some locations of the flasher, but consider it completely off limits:

		```C
		if(thing) {return result;}	//bad
		if(thing) return result;	//very bad
		```

	Same rules apply to loops.


- **IMPERATIVE:** Do not make unnecessary insertions/deletions in dartt_flasher.cpp and dartt_flasher.h. Only add minimum changes necessary to produce the requested functionality. This is currently only set up for hardware testing and I will be scrutinizing changes made to these files carefully. If the modifications are too verbose for me to review, I will not merge them.

