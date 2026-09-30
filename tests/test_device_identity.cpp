#include "platform/DeviceIdentity.h"

#include <cassert>
#include <cstdio>
#include <fstream>

int main() {
    assert(RomCloud::DeviceIdentity::sha256Hex("") ==
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(RomCloud::DeviceIdentity::sha256Hex("abc") ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const char* testPath = "/tmp/romcloud-device-identity-test.txt";
    std::ofstream(testPath, std::ios::binary) << "abc";
    assert(RomCloud::DeviceIdentity::sha256File(testPath) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    {
        std::ofstream largeFile(testPath, std::ios::binary | std::ios::trunc);
        for (int index = 0; index < 1000000; ++index) largeFile.put('a');
    }
    assert(RomCloud::DeviceIdentity::sha256File(testPath) ==
           "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    std::remove(testPath);
    return 0;
}
