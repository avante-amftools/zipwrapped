//
//  unzip.hpp
//  zipwrapped
//
//  Created by Ron Aldrich on 2/17/23.
//

#ifndef unzip_hpp
#define unzip_hpp

#include <string>

namespace zipwrapped
{
    class unzip
    {
    protected:
        void* _zip_reader;  // mz_zip_reader
                
        std::string _currentEntryPath;
        
    public:
        unzip();
        
        virtual ~unzip();
        
        bool open(const std::string& path);
        
        bool open(const char* inputBuffer, int64_t inputBufferLength);
        
        void close();
        
        bool seekFirstEntry();
        
        bool seekNextEntry();
        
        bool seekFileEntryWithName(const std::string& name, bool ignore_case);
        
        bool seekFileEntryWithExtension(const std::string& extension);
        
        const std::string& currentEntryPath() const;
        
        const std::string currentEntryFolder() const;
        
        const std::string currentEntryName() const;
        
        const bool currentEntryIsFile() const;
        
        const bool currentEntryIsVisible() const;
        
        const int64_t uncompressedSize() const;
        
        int64_t unzipCurrentEntry(char* buffer, int64_t bytes);
        
    protected:
        bool collectEntryInfo();
    };
}

#endif /* unzip_hpp */
