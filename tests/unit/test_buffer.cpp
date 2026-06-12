#include "../../src/core/buffer.h"
#include <cassert>
#include <iostream>

using namespace tdesktop;

bool testBufferCreation() {
    Buffer buf(80, 24);
    assert(buf.getCols() == 80);
    assert(buf.getRows() == 24);
    return true;
}

bool testCharAccess() {
    Buffer buf(10, 10);
    buf.setChar(5, 5, U'A');
    assert(buf.at(5, 5).ch == U'A');
    return true;
}

bool testDirtyRects() {
    Buffer buf(10, 10);
    buf.markAllDirty();
    auto rects = buf.getDirtyRects();
    assert(!rects.empty());
    buf.clearDirty();
    rects = buf.getDirtyRects();
    assert(rects.empty());
    return true;
}

bool testClear() {
    Buffer buf(10, 10);
    buf.setChar(5, 5, U'A');
    buf.clear();
    assert(buf.at(5, 5).ch == L' ');
    return true;
}

bool testResize() {
    Buffer buf(10, 10);
    buf.resize(20, 30);
    assert(buf.getCols() == 20);
    assert(buf.getRows() == 30);
    return true;
}

int main() {
    int passed = 0, failed = 0;

    auto run = [&](const char* name, auto fn) {
        try {
            if (fn()) {
                std::cout << "  PASS: " << name << std::endl;
                passed++;
            } else {
                std::cout << "  FAIL: " << name << std::endl;
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "  FAIL: " << name << " (" << e.what() << ")" << std::endl;
            failed++;
        }
    };

    std::cout << "Buffer Tests:" << std::endl;
    run("creation", testBufferCreation);
    run("char_access", testCharAccess);
    run("dirty_rects", testDirtyRects);
    run("clear", testClear);
    run("resize", testResize);

    std::cout << std::endl << "Result: " << passed << "/" << (passed + failed) << " passed";
    if (failed > 0) std::cout << " (" << failed << " failed)";
    std::cout << std::endl;

    return failed > 0 ? 1 : 0;
}
