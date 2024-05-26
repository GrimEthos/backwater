module;
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
export module grim.proxy;

import cpp.memory;

export namespace grim
{
    namespace proxy
    {
        struct A
        {
            virtual void foo( ) = 0;
        };
        struct B
        {
            virtual void foo( ) = 0;
        };
        struct C
        {

            virtual void foo( ) = 0;
        };

        struct ABC
        {
            A & a;
            B & b;
            C & c;
        };

        class D : public ABC
        {
        public:
            D( )
                : ABC{ a_, b_, c_ }
            {
            }

        private:
            struct MyA : A { void foo( ) override; };
            struct MyB : B { void foo( ) override; };
            struct MyC : C { void foo( ) override; };
        private:
            MyA a_;
            MyB b_;
            MyC c_;
        };

        struct IProxy
        {

        };
    }
}