# lockfree::AtomicWrapper&lt;T&gt;
C++11 lock-free atomic wrapper

## Introduction
- Thread-safe, templated, lock-free wrapper for the data structures you want to access from multiple threads concurrently
- Like std::atomic&lt;T&gt; but not limited to primitive data types

## Features
- Turn any struct into a lock-free struct
- Elegant single header implementation (&lt; 200 lines of code)
- Portable C++11 code (tested on x64, ARM, Windows, Linux)
- Easy to use

## Requirements
- Modern C++11 compiler (tested on GCC 5.5, Clang 6.0, MSVC 2015)

## Examples

    TEST_CASE("Atomic Wrapper test", "[lockfree]")
    {
        struct Rectangle
        {
            uint16_t length;
            uint16_t width;

            bool operator== (Rectangle other) const
            {
                return length == other.length && width == other.width;
            }

            Rectangle() = default;
            Rectangle(uint16_t l, uint16_t w) : length(l), width(w) {};
        };

        using AtomicRectangle = AtomicWrapper<Rectangle>;

        SECTION("Update with return variable test")
        {
            //Create an atomic rectangle with length 5 and width 10
            AtomicRectangle atomicRectangleInstance(5, 10);

            //Load and test the values
            auto rectangleInstance = atomicRectangleInstance.load();

            REQUIRE(rectangleInstance.length == 5);
            REQUIRE(rectangleInstance.width == 10);

            //Assigning from a non-atomic variable
            Rectangle updateRectangle(1, 2);
            atomicRectangleInstance = updateRectangle;

            //Assigning to a non-atomic variable
            Rectangle typedRectangleInstance = atomicRectangleInstance;

            REQUIRE(typedRectangleInstance.length == 1);
            REQUIRE(typedRectangleInstance.width == 2);

            //Atomically update part of the struct
            atomicRectangleInstance.update([](Rectangle& updateRectangle)
            {
                updateRectangle.length = 3;
            });

            rectangleInstance = atomicRectangleInstance.load();

            REQUIRE(rectangleInstance.length == 3);
            REQUIRE(rectangleInstance.width == 2);

            //Update atomic rectangle and get area in 1 atomic operation
            const auto updateLength = 10;
            const auto updateWidth = 20;

            const auto area = atomicRectangleInstance.update<uint16_t>([=](Rectangle& updateRectangle)
            {
                updateRectangle.length = updateLength;
                updateRectangle.width = updateWidth;

                return updateRectangle.length * updateRectangle.width;
            });

            rectangleInstance = atomicRectangleInstance.load();

            REQUIRE(area == 200);
            REQUIRE(rectangleInstance.length == 10);
            REQUIRE(rectangleInstance.width == 20);
        }

        SECTION("Reset and null ptr check")
        {
            //Initially equivalent to nullptr
            AtomicRectangle atomicRectangleInstance;
            REQUIRE(atomicRectangleInstance == nullptr);

            //Not equivalent to nullptr after assignment
            atomicRectangleInstance = Rectangle(5, 10);
            REQUIRE(atomicRectangleInstance != nullptr);

            //Check value
            const auto rectangleInstance = atomicRectangleInstance.load();
            REQUIRE(rectangleInstance == Rectangle(5, 10));

            //Equivalent to nullptr after reset
            atomicRectangleInstance.reset();
            REQUIRE(atomicRectangleInstance == nullptr);
        }
    } //TEST_CASE

## About
I build large and complex systems such as simulators. To achieve the required performance, I lean heavily on multi-threading. The
general idea is to spawn a large number (thousands) of sleeping threads and use a publish-subscribe message bus and event-driven
programming to allow the system to scale. That meant that I had to find a way to share data across multiple threads. Initially,
I used mutexes, but then reached a point where std::mutex was bottlenecking the system.

There was no simple, complete, and open source implementation that suited my needs, so I set out to create a cross platform templated wrapper for *any* struct.

I'd already exploited lock-free programming to implement my message bus, so I figured that if my data structures were lock-free, I could do away with mutexes completely. If you try to Google [lock-free programming](https://www.google.com/search?q=+lock+free+programming), you will realize the general consensus is [not to](https://www.modernescpp.com/index.php/c-core-guidelines-concurrency-and-lock-free-programming#h1-lock-free-programming) [do it](https://medium.com/@tylerneely/fear-and-loathing-in-lock-free-programming-7158b1cdd50c).

That said, lockfree::AtomicWrapper is not a silver bullet for all your concurrent data access problems. When in doubt, benchmark to determine the bottleneck before applying this as a solution. This is because lock-free programming can result in bugs that are incredibly difficult to resolve as at any *single* point in time, different threads can see the object in *different* states. Hence, this solution is best for data that is read and updated by multiple threads regularly, so if a thread sees an outdated copy of the data, it would not cause the end of the universe. You should also consider if a
[lock-free queue](https://github.com/cameron314/concurrentqueue) would suit your needs better.

I did say that it is *easy to use*, but it is difficult to use *correctly*.

## References
- [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) - Cameron Desrochers' most excellent concurrentqueue, an inspiration indeed
- [atomic_data](http://alexpolt.github.io/atomic-data.html) - Alexandr Poltavsky's atomic_data lock-free wrapper is similar. It uses a "backing multi-producer/multi-consumer queue" to solve the ABA problem (I use smart pointers) and guarantee wait-free reads (I don't have this). It *should* be faster, but the implementation is more complex. I shamelessly stole some of his API.
- [An Introduction to Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming) - Jeff Preshing's Preshing on Programming
was where I learnt a lot about how lock-free programming works.
