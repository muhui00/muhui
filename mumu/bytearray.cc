/*****************************************
 * Copyright (C) 2023 * Ltd. All rights reserved.
 *
 * File name   : bytearray.cpp
 * Author      : muhui
 * Created date: 2023-01-26 10:44:11
 * Description : 
 *
 *******************************************/

#define LOG_TAG "BYTEARRAY"
#include <fstream>
#include <math.h>
#include <sstream>
#include <iomanip>

#include "bytearray.h"
#include "log.h"
#include "endian.hh"

namespace muhui {

static Logger::ptr g_logger = MUHUI_LOG_NAME("system");

ByteArray::Node::Node()
    : ptr(nullptr)
    , next(nullptr)
    , size(0)
{}

ByteArray::Node::Node(size_t s)
    : ptr(new char[s])
    , next(nullptr)
    , size(s)
{}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}
/**
 * @brief 使用指定长度的内存块构造ByteArray
 * @param[in] base_size 内存块大小
 */
ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size)
    , m_position(0)
    , m_capacity(base_size)
    , m_size(0)
    , m_endian(MUHUI_BIG_ENDIAN)
    , m_root(new Node(base_size))
    , m_cur(m_root)
{
    
}

/**
 * @brief 析构函数
 */
ByteArray::~ByteArray() {
    Node* tmp = m_root;
    //清空内存块
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

/**
 * @brief 写入固定长度int8_t类型的数据
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
 void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));
 }
/**
 * @brief 写入固定长度uint8_t类型的数据
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度int16_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFint16(int16_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
/**
 * @brief 写入固定长度uint16_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度int32_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFint32 (int32_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度uint32_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度int64_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFint64 (int64_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度uint64_t类型的数据(大端/小端)
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != MUHUI_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//Zigzag编解码（解决varint对负数编码效率低的问题）
static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}


/**
 * @brief 写入有符号Varint32类型的数据
 * @post m_position += 实际占用内存(1 ~ 5)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));    
}
/**
 * @brief 写入无符号Varint32类型的数据
 * @post m_position += 实际占用内存(1 ~ 5)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeUint32(uint32_t value) {
    //数据压缩
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

/**
 * @brief 写入有符号Varint64类型的数据
 * @post m_position += 实际占用内存(1 ~ 10)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

/**
 * @brief 写入无符号Varint64类型的数据
 * @post m_position += 实际占用内存(1 ~ 10)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeUint64(uint64_t value) {
    //数据压缩
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);

}

/**
 * @brief 写入float类型的数据
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

/**
 * @brief 写入double类型的数据
 * @post m_position += sizeof(value)
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

/**
 * @brief 写入std::string类型的数据,用uint16_t作为长度类型
 * @post m_position += 2 + value.size()
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

/**
 * @brief 写入std::string类型的数据,用uint32_t作为长度类型
 * @post m_position += 4 + value.size()
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

/**
 * @brief 写入std::string类型的数据,用uint64_t作为长度类型
 * @post m_position += 8 + value.size()
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeStringF64(const std::string& value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

/**
 * @brief 写入std::string类型的数据,用无符号Varint64作为长度类型
 * @post m_position += Varint64长度 + value.size()
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

/**
 * @brief 写入std::string类型的数据,无长度
 * @post m_position += value.size()
 *       如果m_position > m_size 则 m_size = m_position
 */
void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}

/**
 * @brief 读取int8_t类型的数据
 * @pre getReadSize() >= sizeof(int8_t)
 * @post m_position += sizeof(int8_t);
 * @exception 如果getReadSize() < sizeof(int8_t) 抛出 std::out_of_range
 */
int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

/**
 * @brief 读取uint8_t类型的数据
 * @pre getReadSize() >= sizeof(uint8_t)
 * @post m_position += sizeof(uint8_t);
 * @exception 如果getReadSize() < sizeof(uint8_t) 抛出 std::out_of_range
 */
uint8_t ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v;\
    read(&v, sizeof(v)); \
    if(m_endian == MUHUI_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    } \

int16_t ByteArray::readFint16() {
    XX(int16_t);
}

uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

#undef XX

/**
 * @brief 读取有符号Varint32类型的数据
 * @pre getReadSize() >= 有符号Varint32实际占用内存
 * @post m_position += 有符号Varint32实际占用内存
 * @exception 如果getReadSize() < 有符号Varint32实际占用内存 抛出 std::out_of_range
 */
int32_t ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

/**
 * @brief 读取无符号Varint32类型的数据
 * @pre getReadSize() >= 无符号Varint32实际占用内存
 * @post m_position += 无符号Varint32实际占用内存
 * @exception 如果getReadSize() < 无符号Varint32实际占用内存 抛出 std::out_of_range
 */
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;   
}

/**
 * @brief 读取有符号Varint64类型的数据
 * @pre getReadSize() >= 有符号Varint64实际占用内存
 * @post m_position += 有符号Varint64实际占用内存
 * @exception 如果getReadSize() < 有符号Varint64实际占用内存 抛出 std::out_of_range
 */
int64_t ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

/**
 * @brief 读取无符号Varint64类型的数据
 * @pre getReadSize() >= 无符号Varint64实际占用内存
 * @post m_position += 无符号Varint64实际占用内存
 * @exception 如果getReadSize() < 无符号Varint64实际占用内存 抛出 std::out_of_range
 */
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

/**
 * @brief 读取float类型的数据
 * @pre getReadSize() >= sizeof(float)
 * @post m_position += sizeof(float);
 * @exception 如果getReadSize() < sizeof(float) 抛出 std::out_of_range
 */
float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/**
 * @brief 读取double类型的数据
 * @pre getReadSize() >= sizeof(double)
 * @post m_position += sizeof(double);
 * @exception 如果getReadSize() < sizeof(double) 抛出 std::out_of_range
 */
double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/**
 * @brief 读取std::string类型的数据,用uint16_t作为长度
 * @pre getReadSize() >= sizeof(uint16_t) + size
 * @post m_position += sizeof(uint16_t) + size;
 * @exception 如果getReadSize() < sizeof(uint16_t) + size 抛出 std::out_of_range
 */
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取std::string类型的数据,用uint32_t作为长度
 * @pre getReadSize() >= sizeof(uint32_t) + size
 * @post m_position += sizeof(uint32_t) + size;
 * @exception 如果getReadSize() < sizeof(uint32_t) + size 抛出 std::out_of_range
 */
std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取std::string类型的数据,用uint64_t作为长度
 * @pre getReadSize() >= sizeof(uint64_t) + size
 * @post m_position += sizeof(uint64_t) + size;
 * @exception 如果getReadSize() < sizeof(uint64_t) + size 抛出 std::out_of_range
 */
std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取std::string类型的数据,用无符号Varint64作为长度
 * @pre getReadSize() >= 无符号Varint64实际大小 + size
 * @post m_position += 无符号Varint64实际大小 + size;
 * @exception 如果getReadSize() < 无符号Varint64实际大小 + size 抛出 std::out_of_range
 */
std::string ByteArray::readStringVint() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 清空ByteArray
 * @post m_position = 0, m_size = 0
 */
void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

/**
 * @brief 写入size长度的数据
 * @param[in] buf 内存缓存指针
 * @param[in] size 数据大小
 * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
 */
//首先，如果要写入的数据大小为0，则直接返回。然后调用addCapacity函数确保有足够的空间来存储新数据。
//接下来，定义了三个变量 npos、ncap 和 bpos，分别表示当前指针在当前块中的位置，当前块剩余的容量和读入数据的当前位置。
//进入一个 while 循环，如果当前块剩余的容量大于等于要写入的数据大小，则使用 memcpy 函数将数据拷贝到当前块的合适位置。
//如果当前块已经满了，则将 m_cur 指针指向下一个块。最后更新写入位置和数据剩余大小。
//如果当前块剩余的容量小于要写入的数据大小，则使用 memcpy 函数将数据拷贝到当前块的合适位置。
//将 m_cur 指针指向下一个块，更新当前块的剩余容量和指针位置。最后更新写入位置和数据剩余大小。
//最后检查写入的数据是否超过了整个字节数组的大小，如果超过了，则更新字节数组的大小。
void ByteArray::write(const void* buf, size_t size) {
    if(size == 0) {
        return;
    }        
    addCapacity(size);
     size_t npos = m_position % m_baseSize;
     size_t ncap = m_cur->size - npos;
     size_t bpos = 0;
     while(size > 0) {
         if(ncap >= size) {
            //当前内存块剩余容量大于写入数据大小
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            //当前内存块已满
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            //size数据完全写入
            size = 0;
         } else {
            //当前内存块剩余容量小于写入数据大小
            //能写多少写多少，剩余写入下一块内存块
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
         }
     }
     //更新当前数据大小
     if(m_position > m_size) {
         m_size = m_position;
     }
}

/**
 * @brief 读取size长度的数据
 * @param[out] buf 内存缓存指针
 * @param[in] size 数据大小
 * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
 * @exception 如果getReadSize() < size 则抛出 std::out_of_range
 */
void ByteArray::read(void* buf, size_t size) {
    //MUHUI_LOG_DEBUG(g_logger) << "size=" << size;
    //MUHUI_LOG_DEBUG(g_logger) << "getReadSize()=" << getReadSize(); 
    if(size > getReadSize()) {
        throw std::out_of_range("read not enough len");
    }
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos , m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_position += npos;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}


/**
 * @brief 读取size长度的数据
 * @param[out] buf 内存缓存指针
 * @param[in] size 数据大小
 * @param[in] position 读取开始位置
 * @exception 如果 (m_size - position) < size 则抛出 std::out_of_range
 */
void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }
    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(size <= ncap) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (size + npos)) {
                cur = cur->next;
            }
            position += ncap;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

/**
 * @brief 设置ByteArray当前位置
 * @post 如果m_position > m_size 则 m_size = m_position
 * @exception 如果m_position > m_capacity 则抛出 std::out_of_range
 */
void ByteArray::setPosition(size_t v) {
    if(v > m_capacity) {
        throw std::out_of_range("set_postion out of range");
    }
    m_position = v;
    //设置当前数据大小
    if(m_position > m_size) {
        m_size = m_position;
    }
    //设置当前操作的内存块
    m_cur= m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

/**
 * @brief 把ByteArray的数据写入到文件中
 * @param[in] name 文件名
 */
bool ByteArray::writeToFile(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary); //二进制方式存取
    if(!ofs) {
        MUHUI_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error, errno="  << errno << " strerr=" << strerror(errno);
        return false;
    }
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;
    while(read_size > 0) {
        //当前操作位置
        int diff = pos % m_baseSize;
        //当前内存块可存入数据长度
        int len = ((read_size > (int64_t)m_baseSize) ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }
    return true;
}

/**
 * @brief 从文件中读取数据
 * @param[in] name 文件名
 */
bool ByteArray::readFromFile(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary); //二进制方式打开
    if(!ifs) {
        MUHUI_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno="  << errno << " strerr=" << strerror(errno);
        return false;
    }
    //shared_ptr自定义析构
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* buff){ delete[] buff; });
    while(!ifs.eof()) {
        //以m_baseSize为单次读取长度
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

/**
 * @brief 是否是小端
 */
bool ByteArray::isLittleEndian() const {
    return m_endian == MUHUI_LITTLE_ENDIAN;
}

/**
 * @brief 设置是否为小端
 */
void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = MUHUI_LITTLE_ENDIAN;
    } else {
        m_endian = MUHUI_BIG_ENDIAN;
    }
}

/**
 * @brief 将ByteArray里面的数据[m_position, m_size)转成std::string
 */
std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

/**
 * @brief 将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)
 */
std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;
    for(size_t i = 0; i < str.size(); ++i) {
        //每32个字符添加换行符
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        //流控制函数填充字符
        ss << std::setw(2) << std::setfill('0') << std::hex 
            << (int)(uint8_t)str[i] << " ";
    }
    ss << std::endl;
    return ss.str();
}

/**
 * @brief 获取可读取的缓存,保存成iovec数组 (I/O vector) 从可读取位置往后读
 * @param[out] buffers 保存可读取数据的iovec数组
 * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
 * @return 返回实际数据的长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur; 

    if(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;
}

/**
 * @brief 获取可读取的缓存,保存成iovec数组,从position位置开始
 * @param[out] buffers 保存可读取数据的iovec数组
 * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
 * @param[in] position 读取数据的位置
 * @return 返回实际数据的长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root; 
    //获取position所在的内存块
    while(count > 0) {
        cur = cur->next;
        count --;
    }
    size_t ncap = m_cur->size - npos;
    struct iovec iov;

    if(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;

}

/**
 * @brief 获取可写入的缓存,保存成iovec数组
 * @param[out] buffers 保存可写入的内存的iovec数组
 * @param[in] len 写入的长度
 * @return 返回实际的长度
 * @post 如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
 */
uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;
}

/**
 * @brief 扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
 */
void ByteArray::addCapacity(size_t size) {
    if(size == 0) {
        return;
    }
    size_t old_cap = getCapacity();
    if(old_cap >= size) {
        return;
    }

    size -= old_cap;
    //最小扩充内存块个数
    size_t count = ceil(size * 1.0 / m_baseSize);
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }
    //链表插入元素
    //first指向扩充后的第一个内存块
    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }
    if(old_cap == 0) {
        m_cur = first;
    }

}

}//muhui
