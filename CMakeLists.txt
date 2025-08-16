cmake_minimum_required(VERSION 3.16)

# Include test components in all builds
set(EXTRA_COMPONENT_DIRS "test_ingest" "test_aggregator" "test_receiver")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wireless_audio_test)
