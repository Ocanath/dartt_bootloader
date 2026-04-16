#include "binary_file_handler.h"
#include <string.h>
#include <stdio.h>

using namespace ELFIO;

FileType BinaryFileHandler::detect_type(const std::string& path)
{
	const char* ext = strrchr(path.c_str(), '.');
	if(ext != NULL && strcmp(ext, ".elf") == 0)
	{
		return FileType::ELF;
	}
	return FileType::BIN;
}

BinaryFileHandler::BinaryFileHandler(const std::string& path, bool for_write)
	: path_(path),
	  type_(FileType::UNKNOWN),
	  for_write_(for_write),
	  valid_(false),
	  bin_size_(0),
	  elf_data_(nullptr),
	  elf_file_size_(0),
	  elf_block_addr_(0),
	  elf_offset_(0),
	  elf_write_addr_(0)
{
	type_ = detect_type(path);
	if(type_ == FileType::BIN)
	{
		if(for_write_)
		{
			valid_ = open_bin_write();
		}
		else
		{
			valid_ = open_bin_read();
		}
	}
	else if(type_ == FileType::ELF)
	{
		if(for_write_)
		{
			valid_ = true;	// write mode: no source file, finalize() creates it
		}
		else
		{
			valid_ = open_elf_read();
		}
	}
}

bool BinaryFileHandler::open_bin_read()
{
	bin_in_.open(path_, std::ios::binary | std::ios::ate);
	if(!bin_in_.is_open())
	{
		return false;
	}
	bin_size_ = (size_t)bin_in_.tellg();
	bin_in_.seekg(0);
	return true;
}

bool BinaryFileHandler::open_bin_write()
{
	bin_out_.open(path_, std::ios::binary);
	return bin_out_.is_open();
}

bool BinaryFileHandler::open_elf_read()
{
	if(!elf_.load(path_))
	{
		return false;
	}
	for(Elf_Half i = 0; i < elf_.segments.size(); i++)
	{
		segment* seg = elf_.segments[i];
		if(seg->get_type() == PT_LOAD && seg->get_file_size() > 0)
		{
			elf_data_		= seg->get_data();
			elf_file_size_	= (size_t)seg->get_file_size();
			elf_block_addr_	= (uintptr_t)seg->get_physical_address();
			elf_offset_		= 0;
			return true;
		}
	}
	fprintf(stderr, "Error: no loadable PT_LOAD segment found in %s\n", path_.c_str());
	return false;
}

FileType BinaryFileHandler::get_file_type() const
{
	return type_;
}

size_t BinaryFileHandler::get_file_size() const
{
	if(type_ == FileType::BIN)
	{
		return bin_size_;
	}
	return elf_file_size_;
}

bool BinaryFileHandler::is_valid() const
{
	return valid_;
}

void BinaryFileHandler::reset()
{
	if(type_ == FileType::BIN)
	{
		bin_in_.seekg(0);
	}
	else if(type_ == FileType::ELF)
	{
		elf_offset_ = 0;
	}
}

size_t BinaryFileHandler::read_chunk(unsigned char* buf, size_t size)
{
	if(!valid_)
	{
		return 0;
	}
	if(type_ == FileType::BIN)
	{
		bin_in_.read((char*)buf, size);
		return (size_t)bin_in_.gcount();
	}
	else if(type_ == FileType::ELF)
	{
		size_t remaining = elf_file_size_ - elf_offset_;
		size_t actual = (size < remaining) ? size : remaining;
		if(actual == 0)
		{
			return 0;
		}
		memcpy(buf, elf_data_ + elf_offset_, actual);
		elf_offset_ += actual;
		return actual;
	}
	return 0;
}

void BinaryFileHandler::write_chunk(const unsigned char* buf, size_t size)
{
	if(!valid_)
	{
		return;
	}
	if(type_ == FileType::BIN)
	{
		bin_out_.write((const char*)buf, size);
	}
	else if(type_ == FileType::ELF)
	{
		elf_write_buf_.insert(elf_write_buf_.end(), buf, buf + size);
	}
}

uintptr_t BinaryFileHandler::get_block_address() const
{
	if(type_ == FileType::BIN)
	{
		fprintf(stderr, "Error: get_block_address() not valid for BIN file handler\n");
		return 0;
	}
	if(for_write_)
	{
		return elf_write_addr_;
	}
	return elf_block_addr_;
}

void BinaryFileHandler::set_block_address(uintptr_t addr)
{
	if(type_ == FileType::ELF)
	{
		elf_write_addr_ = addr;
	}
}

void BinaryFileHandler::finalize()
{
	if(type_ == FileType::BIN)
	{
		if(bin_out_.is_open())
		{
			bin_out_.flush();
			bin_out_.close();
		}
		return;
	}
	if(type_ == FileType::ELF && for_write_)
	{
		elfio writer;
		writer.create(ELFCLASS32, ELFDATA2LSB);
		writer.set_type(ET_EXEC);

		section* flash_sec = writer.sections.add(".flash");
		flash_sec->set_type(SHT_PROGBITS);
		flash_sec->set_flags(SHF_ALLOC);
		flash_sec->set_addr_align(1);
		flash_sec->set_address(elf_write_addr_);
		flash_sec->set_data(
			(const char*)elf_write_buf_.data(),
			(Elf_Xword)elf_write_buf_.size()
		);

		segment* flash_seg = writer.segments.add();
		flash_seg->set_type(PT_LOAD);
		flash_seg->set_physical_address(elf_write_addr_);
		flash_seg->set_virtual_address(elf_write_addr_);
		flash_seg->set_flags(PF_R | PF_X);
		flash_seg->set_align(1);
		flash_seg->add_section(flash_sec, flash_sec->get_addr_align());

		writer.save(path_);
	}
}
