#ifndef BINARY_FILE_HANDLER_H
#define BINARY_FILE_HANDLER_H

#include <string>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <stddef.h>
#include <elfio/elfio.hpp>

enum class FileType { BIN, ELF, UNKNOWN };

class BinaryFileHandler
{
public:
	BinaryFileHandler(const std::string& path, bool for_write = false);

	FileType	get_file_type() const;
	size_t		get_file_size() const;
	bool		is_valid() const;

	void		reset();
	size_t		read_chunk(unsigned char* buf, size_t size);
	void		write_chunk(const unsigned char* buf, size_t size);

	// ELF-specific: physical address of the first PT_LOAD segment
	uintptr_t	get_block_address() const;
	void		set_block_address(uintptr_t addr);	// for ELF write (read-to-file output)

	// Flush/finalize — for ELF write builds and saves the ELF; for BIN flushes stream
	void		finalize();

private:
	std::string		path_;
	FileType		type_;
	bool			for_write_;
	bool			valid_;

	// BIN
	std::ifstream	bin_in_;
	std::ofstream	bin_out_;
	size_t			bin_size_;

	// ELF read — ELFIO holds segment data in memory (ELF format requirement)
	ELFIO::elfio	elf_;
	const char*		elf_data_;
	size_t			elf_file_size_;
	uintptr_t		elf_block_addr_;
	size_t			elf_offset_;		// sequential read cursor

	// ELF write — accumulate chunks, finalize() builds and saves
	std::vector<unsigned char>	elf_write_buf_;
	uintptr_t					elf_write_addr_;

	static FileType	detect_type(const std::string& path);
	bool open_bin_read();
	bool open_bin_write();
	bool open_elf_read();
};

#endif
