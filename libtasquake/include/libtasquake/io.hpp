#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

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
    private:
        void* m_pBuffer = nullptr;
        std::uint32_t m_uSize = 0;
        std::uint32_t m_uFileOffset = 0;
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

        std::shared_ptr<Buffer> m_pBuffer;
    private:
        std::uint32_t m_uFileOffset = 0;
    };
}
