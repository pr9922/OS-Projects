#ifndef _BOBFS_H_
#define _BOBFS_H_

#include "refs.h"
#include "ide.h"

class BobFS;

struct Bitmap {
    BobFS* fs;
    uint32_t offset;
};


int32_t findBlock(Bitmap* b);

class Node {
public:
    StrongPtr<BobFS> fs;
    uint32_t inumber;
    uint32_t offset;
    uint32_t getBlockNumber(uint32_t blockIndex);

    static constexpr uint32_t SIZE = 16;
    static constexpr uint16_t DIR_TYPE = 1;
    static constexpr uint16_t FILE_TYPE = 2;

    Node(StrongPtr<BobFS> fs, uint32_t inumber);

    uint16_t getType(void);
    uint16_t getLinks(void);
    uint32_t getSize(void);
    uint32_t getDirect(void);
    uint32_t getIndirect(void);

    void setType(uint16_t type);
    void setLinks(uint16_t type);
    void setSize(uint32_t type);
    void setDirect(uint32_t type);
    void setIndirect(uint32_t type);
    
    long unlink(const char* name);

    int32_t read(uint32_t offset, void* buffer, uint32_t n);
    int32_t readAll(uint32_t offset, void* buffer, uint32_t n);

    int32_t write(uint32_t offset, const void* buffer, uint32_t n);
    int32_t writeAll(uint32_t offset, const void* buffer, uint32_t n);

    uint32_t checkIndirect(uint32_t startIndex, uint32_t indirect);
    void writeNodeData(const char* name, uint32_t currINumber, uint32_t size, uint32_t strSize);

    StrongPtr<Node> newNode(const char* name, uint32_t type);
    StrongPtr<Node> newFile(const char* name);
    StrongPtr<Node> newDirectory(const char* name);
    StrongPtr<Node> findNode(const char* name);

    bool isFile(void);
    bool isDirectory(void);

    void linkNode(const char* name, StrongPtr<Node> file);

    void dump(const char* name);

    static StrongPtr<Node> get(StrongPtr<BobFS> fs, uint32_t index) {
        StrongPtr<Node> n { new Node(fs,index) };
        return n;
    }
};


class BobFS {
public:
    StrongPtr<Ide> device;
    Bitmap *inodeBitmap;
    Bitmap *dataBitmap;
   
    uint32_t allocateBlock(void);
    static constexpr uint32_t BLOCK_SIZE = 1024;

    BobFS(StrongPtr<Ide> device);
    virtual ~BobFS();
    static StrongPtr<BobFS> mkfs(StrongPtr<Ide> device);
    static StrongPtr<BobFS> mount(StrongPtr<Ide> device);

    static StrongPtr<Node> root(StrongPtr<BobFS> fs);

    friend class Node;
};


#endif
