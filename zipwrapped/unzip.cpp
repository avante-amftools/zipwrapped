//
//  unzip.cpp
//  zipwrapped
//
//  Created by Ron Aldrich on 2/17/23.
//

#include "unzip.hpp"

#include "mz.h"
#include "mz_strm.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"
#include "mz_strm_os.h"

namespace zipwrapped
{
    static std::string pathByRemovingExtension(const std::string& filePath)
    {
        size_t dot = filePath.find_last_of(".");
        
        if (dot != std::string::npos)
            return filePath.substr(0, dot);
        else
            return filePath;
    }

    static std::string pathByRemovingLastComponent(const std::string& path)
    {
        size_t slash = path.find_last_of("/");
        
        if (slash != std::string::npos)
            return path.substr(0, slash);
        else
            return "";
    }
    
    static std::string lastComponent(const std::string& path)
    {
        size_t slash = path.find_last_of("/");
        
        if (slash != std::string::npos)
            return path.substr(slash+1, path.size() - slash - 1);
        else
            return path;
    }
    
    __attribute__((unused)) static std::string fileName(const std::string& filePath)
    {
        size_t slash = filePath.find_last_of("/");
        if (slash != std::string::npos)
            return pathByRemovingExtension(filePath.substr(slash+1, filePath.size() - slash - 1));
        else
            return pathByRemovingExtension(filePath);
    }

    static std::string extension(const std::string& filePath)
    {
        size_t dot = filePath.find_last_of(".");
        
        if (dot != std::string::npos)
            return filePath.substr(dot, filePath.size() - dot);
        else
            return std::string("");
    }

    static bool hasExtension(const std::string& filePath,
                             const std::string& inExtension)
    {
        std::string lhs = extension(filePath);
        std::string rhs = inExtension;
        
        size_t dot = rhs.find_last_of(".");
        
        if (dot == std::string::npos)
            rhs = "." + rhs;
        
        return ((lhs.size() == rhs.size()) && std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](char & c1, char & c2)
                                                         {
            return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
        }));
    }

    unzip::unzip() :
    _zip_reader(NULL)
    {
        mz_zip_reader_create(&_zip_reader);
    }
    
    unzip::~unzip()
    {
        mz_zip_reader_delete(&_zip_reader);
    }
    
    bool unzip::open(const std::string& path)
    {
        if (mz_zip_reader_open_file(_zip_reader, path.c_str()) != MZ_OK)
        {
            return false;
        }
        
        return true;
    }
    
    bool unzip::open(const char* inputBuffer,
                     int64_t inputBufferLength)
    {
        if (inputBufferLength > INT32_MAX)
            return false;
        
        if (mz_zip_reader_open_buffer(_zip_reader, (uint8_t*)inputBuffer, (int32_t)inputBufferLength, false) != MZ_OK)
        {
            return false;
        }
                
        return true;
    }
    
    void unzip::close()
    {
        mz_zip_reader_close(_zip_reader);
    }
    
    bool unzip::seekFirstEntry()
    {
        if (mz_zip_reader_goto_first_entry(_zip_reader) != MZ_OK)
            return false;
        
        return this->collectEntryInfo();
    }
    
    bool unzip::seekNextEntry()
    {
        if (mz_zip_reader_goto_next_entry(_zip_reader) != MZ_OK)
            return false;
        
        return this->collectEntryInfo();
    }
    
    bool unzip::seekFileEntryWithName(const std::string& name, bool ignore_case)
    {
        if (mz_zip_reader_locate_entry(_zip_reader, name.c_str(), ignore_case) != MZ_OK)
            return false;
        
        return this->collectEntryInfo();
    }
    
    bool unzip::seekFileEntryWithExtension(const std::string& extension)
    {
        if (this->seekFirstEntry() == false)
            return false;
            
        do
        {
            if (this->currentEntryIsFile() &&
                this->currentEntryIsVisible() &&
                hasExtension(this->currentEntryName(), extension))
                return true;
        } while (this->seekNextEntry() == true);
        
        return false;
    }
    
    const std::string& unzip::currentEntryPath() const
    {
        return _currentEntryPath;
    }
    
    const std::string unzip::currentEntryFolder() const
    {
        // return everything up to and including the first /
        
        return pathByRemovingLastComponent(_currentEntryPath);
    }
    
    const std::string unzip::currentEntryName() const
    {
        return lastComponent(_currentEntryPath);
    }
    
    const bool unzip::currentEntryIsFile() const
    {
        return this->currentEntryName() != "";
    }
    
    const bool unzip::currentEntryIsVisible() const
    {
        return true;
    }
    
    const int64_t unzip::uncompressedSize() const
    {
        mz_zip_file* file_info;
        
        if (mz_zip_reader_entry_get_info(_zip_reader, &file_info) != MZ_OK)
            return -1;
        
        return file_info->uncompressed_size;
    }
    
    int64_t unzip::unzipCurrentEntry(char* buffer,
                                     int64_t bytes)
    {
        /*
        err = mz_zip_goto_first_entry(zip_handle); if (err == MZ_OK) err = mz_zip_entry_read_open(zip_handle, 0, NULL); if (err == MZ_OK) printf("Zip entry open for reading\n");
         */
        
        // printf("_zip_reader: %08lX\n", (unsigned long)_zip_reader);
        
        // mz_zip_entry_read_open expects mz_zip, rather than mz_zip_reader

        if (mz_zip_reader_entry_open(_zip_reader) != MZ_OK)
            return 0;
                        
        int64_t result = 0;
        while (bytes > 0)
        {
            int32_t reqBytes = (bytes <= INT32_MAX ?
                                (int32_t) bytes :
                                INT32_MAX);
            
            if (mz_zip_reader_entry_read(_zip_reader, buffer, reqBytes) <= 0)
                return 0;
            
            bytes -= reqBytes;
            result += reqBytes;
        }
        
        mz_zip_reader_entry_close(_zip_reader);
        
        return result;
    }
    
    bool unzip::collectEntryInfo()
    {
        mz_zip_file* file_info;

        if (mz_zip_reader_entry_get_info(_zip_reader, &file_info) != MZ_OK)
            return false;
        
        _currentEntryPath = file_info->filename;
        
        return true;
    }
}
