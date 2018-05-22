#ifndef FATSTACK_T_H
#define FATSTACK_T_H

#include <exception>
#include <cstdint>

template <class T, uint32_t kNumMax=16384>
class FatStack_t{

public:

    class EmptyStackException: public std::exception {
        virtual const char *what() const throw() {
            return "Stack is empty";
        }
    };


    FatStack_t(uint32_t aSizeMin);

    ~FatStack_t();

    T top();
    bool empty();
    uint64_t size();

    void push(T item);
    void pop();

private:

    uint32_t sizeCurrent_;
    uint32_t sizeAllocated_;
    T* items_;

    void reallocate();
    inline T* currentHead();
};

template <class T> using Stack_t = FatStack_t<T>;


#include "../src/FatStack.cxx"

#endif //FATSTACK_T_H