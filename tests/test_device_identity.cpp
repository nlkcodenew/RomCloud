#include "platform/DeviceIdentity.h"

#include <cassert>

int main() {
    assert(RomCloud::DeviceIdentity::sha256Hex("") ==
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(RomCloud::DeviceIdentity::sha256Hex("abc") ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    return 0;
}
