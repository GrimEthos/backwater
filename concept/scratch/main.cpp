#include <exception>
#include <iostream>

import cpp.program;

int main( )
{
    cpp::Program program;
    cpp::Log::addConsoleHandler( );
    try
    {
        cpp::Log::info( "hello world!" );
        throw std::exception( "oops!" );
        return 0;
    }
    catch(std::exception & e) 
    {
        cpp::Log::error(e.what( ));
        return -1;
    }
}