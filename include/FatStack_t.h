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
    FatStack_t(const FatStack_t& fatStack) = default;
    FatStack_t&operator=(const FatStack_t& fatStack) = default;
    ~FatStack_t();

    T top() const;
    bool empty() const;
    uint64_t size() const;

    void push(T const item);
    void pop();

private:

    uint32_t sizeCurrent_;
    uint32_t sizeAllocated_;
    T* items_;

    void reallocate();
    inline T* currentHead() const;
};

template <class T> using Stack_t = FatStack_t<T>;


#include "../src/FatStack.cxx"

#endif //FATSTACK_T_H