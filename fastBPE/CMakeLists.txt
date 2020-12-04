set(PUBLIC_HEADERS API.hpp utils.hpp)

add_executable(fastbpe-bin main.cc utils.cpp)
target_link_libraries(fastbpe-bin
  PRIVATE
  pthread
  Boost::boost)
install (TARGETS fastbpe-bin
  RUNTIME DESTINATION bin)

add_library(fastbpe SHARED API.cpp utils.cpp)
target_link_libraries(fastbpe
  PRIVATE pthread
  PUBLIC Boost::boost
  )

add_executable(fastbpecli API_cli.cc API.cpp utils.cpp)
target_link_libraries(fastbpecli
  PRIVATE
  pthread
  Boost::boost)


install (FILES ${PUBLIC_HEADERS} DESTINATION include/fastbpe)
install (TARGETS fastbpe
  EXPORT FastBPETargets
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include
  )

install(EXPORT FastBPETargets NAMESPACE FastBPE:: DESTINATION lib/cmake/FastBPE)
