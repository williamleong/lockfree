// lockfree::AtomicWrapper - C++11 lock-free wrapper for multi byte data structures
// http://github.com/williamleong/lockfree

// MIT License

// Copyright (c) 2020 William Leong

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef LOCKFREE_ATOMICWRAPPER_H_
#define LOCKFREE_ATOMICWRAPPER_H_

#include <memory>
#include <functional>

namespace lockfree
{

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable:4522)
#endif

    template <class T>
    class AtomicWrapper
    {
        static_assert(std::is_default_constructible<T>::value, "AtomicWrapper requires a default constructable type");

        std::shared_ptr<T> objectPtr = nullptr;

    public:
        void store(const T& updatedObject)
        {
            std::atomic_store_explicit(&objectPtr, std::make_shared<T>(updatedObject), std::memory_order::memory_order_relaxed);
        }

        T load() const
        {
            const auto currentObjectPtr = std::atomic_load_explicit(&objectPtr, std::memory_order::memory_order_relaxed);
            if (currentObjectPtr == nullptr)
                return T();
            else
                return *currentObjectPtr;
        }

        bool load(T& outputObject) const
        {
            const auto currentObjectPtr = std::atomic_load_explicit(&objectPtr, std::memory_order::memory_order_relaxed);

            if (currentObjectPtr == nullptr)
                return false;
            else
            {
                outputObject = *currentObjectPtr;
                return true;
            }
        }

        template<typename U>
        auto read(U readFunction) const -> decltype(readFunction((const T&) T()))
        {
            const auto currentObjectPtr = std::atomic_load_explicit(&objectPtr, std::memory_order::memory_order_relaxed);
            return readFunction(currentObjectPtr == nullptr ? T() : *currentObjectPtr);
        }

        template <typename U>
        U update(std::function<U (T&)> updateFunction)
        {
            std::shared_ptr<T> currentObjectPtr = std::atomic_load(&objectPtr);
            std::shared_ptr<T> updateObjectPtr = nullptr;

            U result;

            do
            {
                T updateObject = currentObjectPtr == nullptr ? T() : *currentObjectPtr;
                result = updateFunction(updateObject);
                updateObjectPtr = std::make_shared<T>(updateObject);
            }
            while (!std::atomic_compare_exchange_weak(&objectPtr, &currentObjectPtr, updateObjectPtr));

            return result;
        }

        void update(std::function<void (T&)> updateFunction)
        {
            std::shared_ptr<T> currentObjectPtr = std::atomic_load(&objectPtr);
            std::shared_ptr<T> updateObjectPtr = nullptr;

            do
            {
                T updateObject = currentObjectPtr == nullptr ? T() : *currentObjectPtr;
                updateFunction(updateObject);
                updateObjectPtr = std::make_shared<T>(updateObject);
            }
            while (!std::atomic_compare_exchange_weak(&objectPtr, &currentObjectPtr, updateObjectPtr));
        }

        void init()
        {
            store(T());
        }

        void reset()
        {
            std::atomic_store_explicit(&objectPtr, std::shared_ptr<T>(nullptr), std::memory_order::memory_order_relaxed);
        }

        //Constructor similar to shared_ptr
        template <typename... Args>
        AtomicWrapper(Args&&... args)
        {
            objectPtr = std::make_shared<T>(std::forward<Args>(args)...);
        }

        AtomicWrapper(T initializeObject)
        {
            objectPtr = std::make_shared<T>(initializeObject);
        }

        //Assignment from T
        AtomicWrapper& operator=(const T& t)
        {
            store(t);
            return *this;
        }

        //Assignment to T
        operator T() const
        {
            return load();
        }

        //Assignment to nullptr
        AtomicWrapper& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }

        //Compare to nullptr
        bool operator== (std::nullptr_t) const
        {
            return std::atomic_load_explicit(&objectPtr, std::memory_order::memory_order_relaxed) == nullptr;
        }

        //Compare to nullptr
        bool operator!= (std::nullptr_t t) const
        {
            return !(*this == t);
        }

        T const* operator->() const = delete;
        T const& operator*() const = delete;

        AtomicWrapper() = default;
        ~AtomicWrapper() = default;
        AtomicWrapper(const AtomicWrapper&) = delete;
        AtomicWrapper& operator=(const AtomicWrapper&) = delete;
        AtomicWrapper& operator=(const AtomicWrapper&) volatile = delete;
    };

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

}

#endif