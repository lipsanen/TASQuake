#include "catch_amalgamated.hpp"
#include "libtasquake/script_parse.hpp"
#include "libtasquake/utils.hpp"
#include <cstdio>

bool compare_buffers(std::shared_ptr<TASQuakeIO::Buffer> ptr1, std::shared_ptr<TASQuakeIO::Buffer> ptr2) {
    auto iface1 = TASQuakeIO::BufferReadInterface::Init(ptr1->ptr, ptr1->size);
    auto iface2 = TASQuakeIO::BufferReadInterface::Init(ptr2->ptr, ptr2->size);

    REQUIRE(ptr1->size == ptr2->size);

    while(iface1.CanRead() && iface2.CanRead()) {
        std::string line1;
        std::string line2;
        iface1.GetLine(line1);
        iface2.GetLine(line2);
        if(line1 != line2) {
            return false;
        }
    }

    return iface1.CanRead() == iface2.CanRead();
}

TEST_CASE("Parsing from string") {
    const char* str = "+1:\n"
    "\ttas_strafe 1\n"
    "+42:\n"
    "\ttas_strafe 0\n";

    TASScript script;
    REQUIRE(script.Load_From_String(str) == true);
    REQUIRE(script.blocks.size() == 2);
    REQUIRE(script.blocks[0].convars["tas_strafe"] == 1);
    REQUIRE(script.blocks[0].frame == 1);
    REQUIRE(script.blocks[1].convars["tas_strafe"] == 0);
    REQUIRE(script.blocks[1].frame == 43);
}

TEST_CASE("Add functions work") {
    TASScript script;
    script.AddCommand("echo test", 0);
    script.AddCvar("tas_strafe", 1, 30);
    script.AddToggle("tas_lgagst", true, 15);

    REQUIRE(script.blocks.size() == 3);
    REQUIRE(script.blocks[0].frame == 0);
    REQUIRE(script.blocks[0].commands[0] == "echo test");
    REQUIRE(script.blocks[1].frame == 15);
    REQUIRE(TASQuake::DoubleEqual(script.blocks[1].toggles["tas_lgagst"], true));
    REQUIRE(script.blocks[2].frame == 30);
    REQUIRE(TASQuake::DoubleEqual(script.blocks[2].convars["tas_strafe"], 1));
}

TEST_CASE("Apply gets rid of redundancies") {
    const char* str1 = "+1:\n"
    "\ttas_strafe 1\n"
    "+1:\n"
    "tas_strafe_yaw 12.345\n";

    TASScript script;
    TASScript script2;
    REQUIRE(script.Load_From_String(str1) == true);
    REQUIRE(script2.Load_From_String(str1) == true);

    script.AddScript(&script2, 2);
    REQUIRE(script.blocks.size() == 2);
    REQUIRE(script.blocks[0].convars["tas_strafe"] == 1);
    REQUIRE(TASQuake::DoubleEqual(script.blocks[1].convars["tas_strafe_yaw"], 12.345));
    REQUIRE(script.blocks[1].convars.size() == 1);
}

TEST_CASE("Parsing id1_er") {
    FILE* fp = fopen("./Runs/id1_er.qtas", "r");
    REQUIRE(fp != nullptr);

    fseek(fp, 0, SEEK_END);
    uint32_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto ptr = TASQuakeIO::Buffer::CreateBuffer(size + 4);
    memcpy(ptr->ptr, &size, 4);
    size_t bytesRead = 4;

    while(bytesRead < size) {
        uint8_t* buf = (uint8_t*)ptr->ptr + bytesRead;
        bytesRead += fread(buf, 1, 4096, fp);
    }

    TASScript script;
    auto readInterface = TASQuakeIO::BufferReadInterface::Init(ptr->ptr, size);
    bool result = script.Load_From_Memory(readInterface);
    REQUIRE(result == true);

    auto writeInterface = TASQuakeIO::BufferWriteInterface::Init();
    script.Write_To_Memory(writeInterface);
    auto generated = writeInterface.m_pBuffer;
    REQUIRE(generated != nullptr);
    REQUIRE(compare_buffers(ptr, generated));
}

TEST_CASE("Float parsing") {
    float value = TASQuake::FloatFromString("1.001");
    REQUIRE(value == 1.001f);
    auto string = TASQuake::FloatString(value);
    REQUIRE(strcmp(string.Buffer, "1.001") == 0);
}
