#if !defined(_WIN32)

// Standard Headers
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
// Platform headers
#include <iconv.h>
// Module headers
#include "Platform.h"

char * itoa(int value, char *string, int radix)
{
	std::ostringstream oss;
	
	oss << std::setbase(radix) << value;
	
	// We don't get passed the size of the destionation buffer which is very
	//  unwise, so plump for a reasonable value.  I don't reckon Graphite
	//  needs more than 64 didits of output.
	// std::string::copy doesn't null terminate the string to don't forget to 
	//  do it.
	string[oss.str().copy(string, 64)] = '\0';
	
	return string;
}


unsigned short MultiByteToWideChar(unsigned long code_page, unsigned long, 
        const char * source, size_t src_count, 
        unsigned short * dest, size_t dst_count)
{
    if (!dest)
        return src_count;
    std::ostringstream oss("CP"); oss << code_page;
    iconv_t cdesc = iconv_open("UCS-2", oss.str().c_str()); // tocode, fromcode
   
    char *dst_ptr = reinterpret_cast<char *>(dest);
    ICONV_CONST char *src_ptr = const_cast<char *>(source);
    dst_count *= sizeof(unsigned short);    
   
    iconv(cdesc, &src_ptr, &src_count, &dst_ptr, &dst_count);
    iconv_close(cdesc);
    
	return src_count;
}

#endif // _WIN32
