#include "libtasquake/io.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdarg>

using namespace TASQuakeIO;

Buffer::~Buffer() {
    if(!readonly)
        free(ptr);
}

bool Buffer::Realloc(std::uint32_t newsize) {
    void* p = std::realloc(ptr, newsize);

    if(p) {
        this->size = newsize;
        ptr = p;
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<Buffer> Buffer::CreateBuffer(std::uint32_t initialSize) {
    auto ptr = std::make_shared<Buffer>();
    if(ptr) {
        ptr->Realloc(initialSize);
    }
    return ptr;
}


std::shared_ptr<Buffer> Buffer::CreateFromCString(const char* str) {
    auto ptr = std::make_shared<Buffer>();
    if(ptr) {
        ptr->ptr = (void*)str;
        ptr->size = strlen(str);
        ptr->readonly = true;
    }
    return ptr;
}

FileReadInterface FileReadInterface::Init(const char* filepath) {
    FileReadInterface iface;
    iface.m_pStream.open(filepath);
    return iface;
}

bool FileReadInterface::CanRead() {
    return m_pStream.good() && !m_pStream.eof();
}

bool FileReadInterface::GetLine(std::string& str) {
    std::getline(m_pStream, str);
    return !str.empty() || CanRead();
}

std::uint32_t FileReadInterface::Read(void* dest, std::uint32_t buf_size) {
    m_pStream.read((char*)dest, buf_size);
    if(m_pStream.eof()) {
        return m_pStream.gcount();
    } else {
        return buf_size;
    }
}

BufferReadInterface BufferReadInterface::Init(void* buffer, std::uint32_t size) {
    BufferReadInterface output;
    output.m_pBuffer = buffer;
    output.m_uSize = size;
    return output;
}

bool BufferReadInterface::CanRead() {
    return m_uFileOffset < m_uSize;
}

bool BufferReadInterface::GetLine(std::string& str) {
    str.clear();
    if(!CanRead()) {
        return false;
    }

    while(str.empty() && m_uFileOffset < m_uSize) {
        std::uint32_t endOffset;
        std::uint32_t bytesLeft = m_uSize - m_uFileOffset;
        const char* strStart = ((char*)m_pBuffer) + m_uFileOffset;

        for(endOffset=0; endOffset < bytesLeft; ++endOffset) {
            char c = strStart[endOffset];
            if(c == '\0' || c == '\n') {
                break;
            }
        }

        m_uFileOffset += endOffset + 1; // Skip past the newline or null character

        if(endOffset != 0) {
            str.assign(strStart, endOffset);
        }
    }

    return !str.empty();
}

std::uint32_t BufferReadInterface::Read(void* dest, std::uint32_t buf_size) {
    std::uint32_t bytesLeft = m_uSize - m_uFileOffset;
    std::uint32_t bytesToRead = std::min(bytesLeft, buf_size);
    std::memcpy(dest, (std::uint8_t*)m_pBuffer + m_uFileOffset, bytesToRead);
    m_uFileOffset += bytesToRead;
    return bytesToRead;
}

FileWriteInterface FileWriteInterface::Init(const char* filepath) {
    FileWriteInterface iface;
    iface.m_pStream.open(filepath);
    return iface;
}

bool FileWriteInterface::CanWrite() {
    return m_pStream.good();
}

bool FileWriteInterface::WriteLine(const std::string& str) {
    m_pStream << str << '\n';
    return true;
}

std::uint32_t FileWriteInterface::Write(const char* format, ...) {
    char BUFFER[1024];
    va_list args;
    va_start(args, format);
    auto bytes = vsnprintf(BUFFER, sizeof(BUFFER), format, args);
    va_end(args);

    m_pStream.write(BUFFER, std::min<std::uint32_t>(sizeof(BUFFER), bytes));
    return bytes;
}

void FileWriteInterface::Finalize() {
    m_pStream.close();
}

BufferWriteInterface BufferWriteInterface::Init() {
    BufferWriteInterface iface;
    iface.m_pBuffer = Buffer::CreateBuffer(128);
    return iface;
}

bool BufferWriteInterface::CanWrite() {
    return true;
}

bool BufferWriteInterface::WriteLine(const std::string& str) {
    Write("%s\n", str.c_str());
    return true;
}

std::uint32_t BufferWriteInterface::Write(const char* format, ...) {
    std::uint32_t bytesLeft = m_pBuffer->size - m_uFileOffset;
    va_list args;
    va_start(args, format);
    auto bytes = vsnprintf((char*)m_pBuffer->ptr + m_uFileOffset, bytesLeft, format, args);
    if(bytes != bytesLeft) {
        AllocateSpaceForWrite(bytes);
        bytesLeft = m_pBuffer->size - m_uFileOffset;
        // Now we have enough space
        bytes = vsnprintf((char*)m_pBuffer->ptr + m_uFileOffset, bytesLeft, format, args);
    }

    va_end(args);

    m_uFileOffset += bytes;
    return bytes;
}

void BufferWriteInterface::Finalize() {
    m_pBuffer->size = m_uFileOffset;
}


void BufferWriteInterface::AllocateSpaceForWrite(uint32_t bytes) {
    std::uint32_t newSize = std::max<std::uint32_t>(1, m_pBuffer->size);
    std::uint32_t targetSize = m_uFileOffset + bytes;

    while(newSize < targetSize) {
        newSize <<= 1;
    }

    if(newSize != m_pBuffer->size) {
        m_pBuffer->Realloc(newSize);
    }
}

void BufferWriteInterface::WriteBytes(const void* src, uint32_t bytes) {
    AllocateSpaceForWrite(bytes);
    memcpy((uint8_t*)m_pBuffer->ptr + m_uFileOffset, src, bytes);
    m_uFileOffset += bytes;
}

