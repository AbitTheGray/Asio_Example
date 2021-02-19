set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

#--------------------------------
# Asio
# C++ Async Network Library
#--------------------------------

if(NOT TARGET asio)
    add_library(asio INTERFACE)

    target_include_directories(asio INTERFACE asio/asio/include)

    # https://think-async.com/Asio/asio-1.18.1/doc/asio/using.html
    target_compile_definitions(asio INTERFACE
            ASIO_HAS_STD_SHARED_PTR=1
            ASIO_HAS_CO_AWAIT=1
            ASIO_HAS_STD_COROUTINE=1
            ASIO_HAS_CONSTEXPR=1
            ASIO_HAS_CSTDINT=1
            ASIO_HAS_CXX11_ALLOCATORS=1
            ASIO_HAS_NOEXCEPT=1
            ASIO_HAX_NULLPTR=1
            ASIO_HAS_STD_FUNCTION=1
            ASIO_HAS_STD_STRING_VIEW=1
            ASIO_HAS_STD_MUTEX_AND_CONDVAR=1
            ASIO_NO_DEPRECATED=1
            )
endif()
