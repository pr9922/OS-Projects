#include "file_sys.h"
#include "libk.h"
#include "heap.h"


//////////
// Node //
//////////


Node::Node(StrongPtr<BobFS> fs, uint32_t inumber){
    this->fs = fs;
    this->inumber = inumber;
    this->offset = (3 * BobFS::BLOCK_SIZE) + inumber * 16;  //offset + relative node location
}

void initializeNode(StrongPtr<Node> node, uint32_t type, uint32_t size, uint16_t links){
    node->setType(type);
    node->setSize(size);
    node->setDirect(0);
    node->setIndirect(0);
    node->setLinks(links);
} 

//Dir == 1
bool Node::isDirectory(void) {
    if(getType() == 1){
	    return true;
    }
    else{
	    return false;
    }
}

//File == 2
bool Node::isFile(void) {
    if(getType() == 2){
	    return true;
    }
    else{
	    return false;
    }
}

//2 byte type
//2 byte links
//4 byte size
//4 byte direct ptr
//4 byte indirect ptr
void Node::setType(uint16_t type) {
    fs->device->writeAll(offset, &type, 2);
}

void Node::setLinks(uint16_t links) {
    fs->device->writeAll(offset + 2, &links, 2);
}

void Node::setSize(uint32_t size) {
    fs->device->writeAll(offset + 4, &size, 4);
}

void Node::setDirect(uint32_t direct) {
    fs->device->writeAll(offset + 8, &direct, 4);
}

void Node::setIndirect(uint32_t indirect) {
    fs->device->writeAll(offset + 12, &indirect, 4);
}

uint16_t Node::getType(void) {
    uint16_t temp;
    fs->device->readAll(offset, &temp, 2);
    return temp;
}

uint32_t Node::getSize(void) {
    uint32_t temp;
    fs->device->readAll(offset + 4, &temp, 4);
    return temp;
}

uint16_t Node::getLinks(void) {
    uint16_t temp;
    fs->device->readAll(offset + 2, &temp, 2);
    return temp;
}

uint32_t Node::getDirect(void) {
    uint32_t temp;
    fs->device->readAll(offset + 8, &temp, 4);
    return temp;
}

uint32_t Node::getIndirect(void) {
    uint32_t temp;
    fs->device->readAll(offset + 12, &temp, 4);
    return temp;
}

bool streq(const char* a, const char* b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

//inumber is first 4 bits
//name length is next 4 bits
//name is last n bits
StrongPtr<Node> Node::findNode(const char* name) {
   StrongPtr<Node> result;
   for(uint32_t i = 0; i < getSize(); i += 8){
       uint32_t currNode;
       uint32_t currSize;
       char currName[100];
       readAll(i, &currNode, 4);     //get i number
       readAll(i + 4, &currSize, 4);   //name length is offset 4 bytes
       readAll(i + 8, &currName, currSize); //get name
       currName[currSize] = 0;   //terminate with zero!
       if(streq(currName, name)){  
           return Node::get(fs, currNode);
       }
       else{
	   i += currSize; //get to next node by adding size left
       }
   } 
   return result;
}

int32_t Node::write(uint32_t offset, const void* buffer, uint32_t n) {
   uint32_t index = offset / BobFS::BLOCK_SIZE;  
   uint32_t blockNumber = getBlockNumber(index);
   uint32_t start = offset % BobFS::BLOCK_SIZE;
   int32_t result = fs->device->write(start + blockNumber * BobFS::BLOCK_SIZE, buffer, n);
   if(offset + result > getSize()){  //set size if wrote more than current size
	  this->setSize(offset + result);
   }
   return result;
}

int32_t Node::writeAll(uint32_t offset, const void* buffer_, uint32_t n) {

    int32_t total = 0;
    char* buffer = (char*) buffer_;

    while (n > 0) {
        int32_t cnt = write(offset,buffer,n);
        if (cnt <= 0) return total;

        total += cnt;
        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
    return total;
}

int32_t Node::read(uint32_t offset, void* buffer, uint32_t n) {
    if (n > BobFS::BLOCK_SIZE || n + offset > getSize()){  //if more than the block size
	    n = getSize() - offset;  //read to end of block
    }
    uint32_t index = offset / BobFS::BLOCK_SIZE;
    uint32_t blockNumber = getBlockNumber(index);
    uint32_t start = offset % BobFS::BLOCK_SIZE;
    return this->fs->device->read((start) + (blockNumber * BobFS::BLOCK_SIZE), buffer, n);
}

int32_t Node::readAll(uint32_t offset, void* buffer_, uint32_t n) {

    int32_t total = 0;
    char* buffer = (char*) buffer_;

    while (n > 0) {
        int32_t cnt = read(offset,buffer,n);
        if (cnt <= 0) return total;

        total += cnt;
        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
    return total;
}

uint32_t Node::checkIndirect(uint32_t startIndex, uint32_t indirect){
    uint32_t result;
    uint32_t resultOffset = (startIndex * 4) + (indirect * BobFS::BLOCK_SIZE);
    this->fs->device->readAll(resultOffset, &result, 4);
    if(result == 0){
        result = this->fs->allocateBlock();
	    this->fs->device->writeAll(resultOffset, &result, 4);	   
    }
    return result;
}

StrongPtr<Node> Node::newFile(const char* name) {
    return newNode(name, FILE_TYPE);
}

StrongPtr<Node> Node::newDirectory(const char* name) {
    return newNode(name, DIR_TYPE);
}

void Node::linkNode(const char* name, StrongPtr<Node> node) {
    uint32_t strSize = 0;
    uint32_t currIndex = 0;
    while(name[currIndex] != '\0'){ //get length of string name
	currIndex++;
        strSize++;
    }
    uint32_t currINumber = node->inumber; //get inumber
    this->writeNodeData(name, currINumber, getSize(), strSize); 
    uint16_t currLinks = node->getLinks(); 
    node->setLinks(currLinks + 1);   //update links in the node  
}

long Node::unlink(const char* name){
    if(!this->isDirectory()){
	    return -1;
    }
    else{
        StrongPtr<Node> tgt = this->findNode(name);
	    if(tgt.isNull()){
	        return 0;
        }
        else{
            tgt->setLinks(tgt->getLinks() - 1);
            if(tgt->getLinks() == 0){
				for(uint32_t i = 0; i < BobFS::BLOCK_SIZE; i+=8){
					uint32_t currNode;
					uint32_t currSize;
					char currName[100];
					readAll(i, &currNode, 4);  
					readAll(i + 4, &currSize, 4);   
					readAll(i + 8, &currName, currSize); 
					currName[currSize] = 0;   
					return 1 + unlink(currName);
					i += currSize;		
				}
			}
        }
    }
    return 0;
}

StrongPtr<Node> Node::newNode(const char* name, uint32_t type) {
    int32_t findEmpty = findBlock(fs->inodeBitmap);
    StrongPtr<Node> node = Node::get(fs, findEmpty);
    initializeNode(node, type, 0, 0);
    linkNode(name, node);
    return node;
}

uint32_t Node::getBlockNumber(uint32_t index){
    if(index < 0){
	    return 0;
    }
    else if(index == 0 && getDirect() != 0){
	    return getDirect();
    }
    else if(index == 0 && getDirect() == 0){
        uint32_t result = this->fs->allocateBlock();
	    this->setDirect(result);
	    return result;
    }
    else if(index != 0 && getIndirect() != 0){
	    uint32_t indirect = getIndirect();
        return checkIndirect(index-1, indirect);
    }
    else if(index != 0 && getIndirect() == 0){
        uint32_t indirect = this->fs->allocateBlock();
	    setIndirect(indirect);
        return checkIndirect(index-1, indirect);
    }
    else{
	    return -1;
    }
}

//Inumber at offset 0, name size at offset 4, name at offset 8
void Node::writeNodeData(const char* name, uint32_t currINumber, uint32_t size, uint32_t strSize){
    this->writeAll(size, &currINumber, 4);   
    this->writeAll(size + 4, &strSize, 4);
    this->writeAll(size + 8, name, strSize);
}

void Node::dump(const char* name) {
    uint32_t type = getType();
    switch (type) {
    case DIR_TYPE:
        Debug::printf("*** 0 directory:%s(%d)\n",name,getLinks());
        {
            uint32_t sz = getSize();
            uint32_t offset = 0;

            while (offset < sz) {
                uint32_t ichild;
                readAll(offset,&ichild,4);
                offset += 4;
                uint32_t len;
                readAll(offset,&len,4);
                offset += 4;
                char* ptr = (char*) malloc(len+1);
                readAll(offset,ptr,len);
                offset += len;
                ptr[len] = 0;              
                
                StrongPtr<Node> child = Node::get(fs,ichild);
                child->dump(ptr);
                free(ptr);
            }
        }
        break;
    case FILE_TYPE:
        Debug::printf("*** 0 file:%s(%d,%d)\n",name,getLinks(),getSize());
        break;
    default:
         Debug::panic("unknown i-node type %d\n",type);
    }
}


///////////
// BobFS //
///////////

BobFS::BobFS(StrongPtr<Ide> device){ //initialize bitmaps and device
    this->device = device;
    this->dataBitmap->fs = this;
    this->inodeBitmap->fs = this;
    this->dataBitmap->offset = BLOCK_SIZE;  //2nd block
    this->inodeBitmap->offset = BLOCK_SIZE * 2; //3rd block
}

BobFS::~BobFS() {
}

StrongPtr<Node> BobFS::root(StrongPtr<BobFS> fs) {
    uint32_t temp;
    fs->device->readAll(8, &temp, 4); //get inumber pointed to by superblock
    return Node::get(fs, temp);
}

StrongPtr<BobFS> BobFS::mount(StrongPtr<Ide> device) {
    StrongPtr<BobFS> fs { new BobFS(device) };
    return fs;
}

StrongPtr<BobFS> BobFS::mkfs(StrongPtr<Ide> device) { 
    StrongPtr<BobFS> fs { new BobFS(device) };
    device->writeAll(0,"BOBFS439", 8);  //set magic number in superblock
    uint32_t rootIndex;
    device->writeAll(8, &rootIndex, 4);  //store inumber for root directory 
    StrongPtr<Node> root = Node::get(fs, rootIndex);  //get root based on inumber above
    initializeNode(root, 1, 0, 1);  //initialize root values
    return fs; 
}

uint32_t BobFS::allocateBlock(void) {
    uint32_t index = ((3 * BLOCK_SIZE) + (16 * BLOCK_SIZE))/(BLOCK_SIZE) + (findBlock(inodeBitmap));
    return index;
}

//////////Helper functions

int32_t findBlock(Bitmap* b){
    bool found = false;
    uint32_t index = 0;
    while(!found && index < BobFS::BLOCK_SIZE){
	uint32_t temp;
        b->fs->device->readAll(b->offset + index, &temp, 4);
	if(temp == 0){    //found a block
	    temp = 1;      //set as taken
	    found = true;
            uint32_t currOffset = b->offset + index;
            b->fs->device->writeAll(currOffset, &temp, 4);
        }
        else{
	    index++;
        }
    }
    if(found){
	    return index;
    }
    else{
	    return -1;
    }   
} 