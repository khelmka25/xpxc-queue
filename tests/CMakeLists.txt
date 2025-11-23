add_executable(QueueTest QueueTest.cc)
target_link_libraries(QueueTest 
    PRIVATE
    Catch2WithMain
    MpmcQueue    
)
target_compile_options(QueueTest
    PUBLIC
    -O2
)
add_test(NAME QueueTest COMMAND QueueTest)