# Libraries..
add_subdirectory(carl)

# For unit tests.
# We decided to always compile them, if gtest is present,
# since non-compiling tests are a major blocker of development.
add_subdirectory(tests)

# For examples.
add_subdirectory(examples EXCLUDE_FROM_ALL)
