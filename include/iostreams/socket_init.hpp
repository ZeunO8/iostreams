#pragma once
namespace iostreams
{
    struct socket_init
    {
        static bool initialized;
        static void initialize();
    };
}