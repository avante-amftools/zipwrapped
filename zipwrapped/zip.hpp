//
//  zip.hpp
//  zipwrapped
//
//  Created by Ron Aldrich on 2/17/23.
//

#ifndef zip_hpp
#define zip_hpp

#include <stdio.h>

namespace zipwrapped
{
    class zip
    {
    protected:
        void* _zip_writer;  // mz_zip_writer
        
    public:
        zip();
        
        virtual ~zip();
        
        bool open(const char* inputBuffer,
                  int64_t inputBufferLength);
        
        void close();
    };
}

#endif /* zip_hpp */
