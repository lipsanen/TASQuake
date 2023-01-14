#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace TASQuakeIO
{
    struct Buffer {
        void* ptr = nullptr;
        std::uint32_t size = 0;
        bool readonly = false;

        ~Buffer();
        Buffer() = default;
        bool Realloc(std::uint32_t newsize);
        static std::shared_ptr<Buffer> CreateBuffer(std::uint32_t initialSize);
        static std::shared_ptr<Buffer> CreateFromCString(const char* str);
    };

        
    template<typename T>
    void GetElemsFromBuffer(void* ptr, size_t size, std::vector<T>& vec) {
        size_t elems = size / sizeof(T);
        vec.resize(elems);
        memcpy(&vec[0], ptr, elems * sizeof(T));
    }

    template<typename T>
    void GetElemsFromBuffer(std::shared_ptr<TASQuakeIO::Buffer> buffer, std::vector<T>& vec) {
        GetElemsFromBuffer(buffer->ptr, buffer->size, vec);
    }

    template<typename T>
    std::shared_ptr<TASQuakeIO::Buffer> GetBufferFromElems(const std::vector<T>& vec) {
        auto buffer = TASQuakeIO::Buffer::CreateBuffer(vec.size() * sizeof(T));
        memcpy((std::uint8_t*)buffer->ptr, &vec[0], vec.size() * sizeof(T));
        return buffer;
    }

    class ReadInterface {
    public:
        virtual ~ReadInterface() {}
        virtual bool CanRead() = 0;
        virtual bool GetLine(std::string& str) = 0;
        virtual std::uint32_t Read(void* dest, std::uint32_t buf_size) = 0;
        virtual void Finalize() {}
    };

    class FileReadInterface : public ReadInterface {
    public:
        static FileReadInterface Init(const char* filepath);
        virtual bool CanRead();
        virtual bool GetLine(std::string& str);
        virtual std::uint32_t Read(void* dest, std::uint32_t buf_size);
    private:
        std::ifstream m_pStream;
    };

    class BufferReadInterface : public ReadInterface {
    public:
        static BufferReadInterface Init(void* buf, size_t size);
        virtual bool CanRead();
        virtual bool GetLine(std::string& str);
        virtual std::uint32_t Read(void* dest, std::uint32_t buf_size);
        void* m_pBuffer = nullptr;
        std::uint32_t m_uSize = 0;
        std::uint32_t m_uFileOffset = 0;

        template<typename T>
        void ReadPODVec(std::vector<T>& out) {
            uint32_t size;
            memcpy(&size, (uint8_t*)m_pBuffer + m_uFileOffset, sizeof(uint32_t));
            m_uFileOffset += sizeof(uint32_t);
            out.resize(size / sizeof(T));
            memcpy(&out[0], (uint8_t*)m_pBuffer + m_uFileOffset, size);
            m_uFileOffset += size;
        }
    };

    class WriteInterface {
    public:
        virtual ~WriteInterface() {}
        virtual bool CanWrite() = 0;
        virtual bool WriteLine(const std::string& str) = 0;
        virtual std::uint32_t Write(const char* format, ...) = 0;
        virtual void Finalize() {}
    };

    class FileWriteInterface : public WriteInterface {
    public:
        static FileWriteInterface Init(const char* filepath);
        virtual bool CanWrite() override;
        virtual bool WriteLine(const std::string& str) override;
        virtual std::uint32_t Write(const char* format, ...) override;
        virtual void Finalize() override;
    private:
        std::ofstream m_pStream;
    };

    class BufferWriteInterface : public WriteInterface {
    public:
        static BufferWriteInterface Init();
        virtual bool CanWrite() override;
        virtual bool WriteLine(const std::string& str) override;
        virtual std::uint32_t Write(const char* format, ...) override;
        virtual void Finalize() override;
        void AllocateSpaceForWrite(uint32_t bytes);

        std::shared_ptr<Buffer> m_pBuffer;
        std::uint32_t m_uFileOffset = 0;

        template<typename T>
        void WritePODVec(std::vector<T>& out) {
            uint32_t size = out.size() * sizeof(T);
            AllocateSpaceForWrite(size + sizeof(uint32_t));
            memcpy((uint8_t*)m_pBuffer->ptr + m_uFileOffset, &size, sizeof(uint32_t));
            m_uFileOffset += sizeof(uint32_t);
            memcpy((uint8_t*)m_pBuffer->ptr + m_uFileOffset, &out[0], size);
            m_uFileOffset += size;
        }
    private:
    };
}
