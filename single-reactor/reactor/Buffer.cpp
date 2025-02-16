#include "Buffer.h"
#include <string.h>

Buffer::Buffer()
{

}
Buffer::~Buffer()
{

}
void Buffer::append(const char*data,size_t size)
{
    buf_.append(data,size);
}
size_t Buffer::size()
{
    return buf_.size();
}
const char* Buffer::data()
{
    return buf_.data();
}
void Buffer::clear()
{
    buf_.clear();
}
void Buffer::erase(size_t pos,size_t num)
{
    buf_.erase(pos,num);
}
Buffer Buffer::operator+(const std::string& inbuffer){
    this->buf_.append(inbuffer.data(),inbuffer.size());
    return *this;
};
std::string Buffer::getmessage()           //还可以支持固定长度,指定报文长度和分隔符等多种格式
{
    int len;
    memcpy(&len,buf_.data(),4);            //取出报文头部
    if(buf_.size()<len+4) return " ";      //例如len=6 但是inputbuffer里面是6abc,他的完整报文大小应该是4+6（int+len）注意6并不是string
    std::string message(buf_.data()+4,len);//如果是完整的消息
    buf_.erase(0,len+4);   
    return message;
}