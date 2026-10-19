#pragma once
#include <cstddef>
namespace V62Text {
inline size_t next(const char *data,size_t length,size_t pos,size_t width=48){size_t cols=0;while(pos<length&&cols<width){char c=data[pos++];if(c=='\n')break;if(c!='\r')cols++;}return pos;}
inline size_t lines(const char *data,size_t length){size_t count=0,pos=0;while(pos<length){pos=next(data,length,pos);count++;}return count;}
inline size_t offset(const char *data,size_t length,size_t target){size_t line=0,pos=0;while(line<target&&pos<length){pos=next(data,length,pos);line++;}return pos;}
inline size_t line(const char *data,size_t length,size_t offset){size_t count=0,pos=0;while(pos<offset&&pos<length){pos=next(data,length,pos);count++;}return count;}
}
