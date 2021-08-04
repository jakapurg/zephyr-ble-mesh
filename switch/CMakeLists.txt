set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(switch)

target_sources(app PRIVATE src/main.c)

cmake_minimum_required(VERSION 3.13.1)
