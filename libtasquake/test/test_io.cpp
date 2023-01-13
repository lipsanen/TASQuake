#include "catch_amalgamated.hpp"
#include "libtasquake/io.hpp"
#include <filesystem>

TEST_CASE("BufferRead", "Works") {
    const char* test = "first line\n\n\n\nsecond line\nthird line";
    auto buffer = TASQuakeIO::Buffer::CreateFromCString(test);
    std::string line;
    
    TASQuakeIO::BufferReadInterface iface = TASQuakeIO::BufferReadInterface::Init(buffer->ptr, buffer->size);
    iface.GetLine(line);
    REQUIRE(line == "first line");
    iface.GetLine(line);
    REQUIRE(line == "second line");
    iface.GetLine(line);
    REQUIRE(line == "third line");
}

TEST_CASE("BufferWrite", "Works") {
    TASQuakeIO::BufferWriteInterface iface = TASQuakeIO::BufferWriteInterface::Init();
    iface.WriteLine("first line");
    iface.WriteLine("second line");
    iface.WriteLine("third line");
    iface.Finalize();
    auto buffer = iface.m_pBuffer;
    const char* ground_truth = "first line\nsecond line\nthird line\n";
    auto size = strlen(ground_truth);
    REQUIRE(buffer->size == size);
    REQUIRE(memcmp(ground_truth, buffer->ptr, size) == 0);
}

TEST_CASE("FileWriteRead", "Works") {
    auto path = std::filesystem::temp_directory_path();
    auto test_path = path / "tasquaketestfile";
    auto iface = TASQuakeIO::FileWriteInterface::Init(test_path.c_str());
    iface.WriteLine("first line");
    iface.WriteLine("second line");
    iface.WriteLine("third line");
    iface.Finalize();

    auto readFace = TASQuakeIO::FileReadInterface::Init(test_path.c_str());
    std::string line;
    REQUIRE(readFace.CanRead() == true);
    readFace.GetLine(line);
    REQUIRE(line == "first line");
    readFace.GetLine(line);
    REQUIRE(line == "second line");
    readFace.GetLine(line);
    REQUIRE(line == "third line");
    readFace.GetLine(line);
    REQUIRE(line == "");
    REQUIRE(readFace.CanRead() == false);

    std::error_code ec;

    if(std::filesystem::exists(test_path, ec)) {
        std::filesystem::remove(test_path, ec);
    }
}
