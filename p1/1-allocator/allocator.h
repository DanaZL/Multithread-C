#include <stdexcept>
#include <string>
#include <list>
#include <iostream>

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class Allocator;
class MemoryUnit;

class Pointer {
public:
    static Allocator * allocator; 

    std::list<MemoryUnit>::iterator unit;

    Pointer();

    Pointer(std::list<MemoryUnit>::iterator _unit):
            unit(_unit)
    {}

    void *get() const; 

};

class MemoryUnit{
public:
    size_t offset;
    size_t size;

    MemoryUnit() {}
    
    MemoryUnit(size_t _offset, size_t _size):
                    offset(_offset),
                    size(_size)
    {}

    void Expansion (size_t add_size);

    size_t Decrease (size_t substr_size);

};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

class Allocator;

class Allocator {
public:
    Allocator(void *base, size_t size);
    
    Pointer alloc(size_t N);

    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);

    void defrag();
    void dump();

    std::list <MemoryUnit> used_list;
    void * base;
private:
    std::list <MemoryUnit> free_list;
    
    size_t size;
    size_t used_memory;
};

