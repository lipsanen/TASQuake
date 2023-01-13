#include "catch_amalgamated.hpp"
#include "libtasquake/script_parse.hpp"
#include "libtasquake/utils.hpp"
#include <cstdio>

bool compare_buffers(std::shared_ptr<TASQuakeIO::Buffer> ptr1, std::shared_ptr<TASQuakeIO::Buffer> ptr2) {
    auto iface1 = TASQuakeIO::BufferReadInterface::Init(ptr1->ptr, ptr1->size);
    auto iface2 = TASQuakeIO::BufferReadInterface::Init(ptr2->ptr, ptr2->size);

    while(iface1.CanRead() && iface2.CanRead()) {
        std::string line1;
        std::string line2;
        iface1.GetLine(line1);
        iface2.GetLine(line2);
        if(line1 != line2) {
            return false;
        }
    }

    return true;
}

TEST_CASE("Parsing id1_er") {
    FILE* fp = fopen("./Runs/id1_er.qtas", "r");
    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto ptr = TASQuakeIO::Buffer::CreateBuffer(size);
    size_t bytesRead = 0;

    while(bytesRead < size) {
        uint8_t* buf = (uint8_t*)ptr->ptr + bytesRead;
        bytesRead += fread(buf, 1, 4096, fp);
    }

    TASScript script;
    bool result = script.Load_From_Memory(ptr);
    REQUIRE(result == true);

    auto generated = script.Write_To_Memory();
    REQUIRE(generated != nullptr);
    REQUIRE(compare_buffers(ptr, generated));
}

TEST_CASE("Float parsing") {
    float value = TASQuake::FloatFromString("1.001");
    REQUIRE(value == 1.001f);
    auto string = TASQuake::FloatString(value);
    REQUIRE(strcmp(string.Buffer, "1.001") == 0);
}
