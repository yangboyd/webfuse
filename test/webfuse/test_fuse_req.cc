#include <gtest/gtest.h>
#include "webfuse/impl/fuse_wrapper.h"

TEST(libfuse, fuse_req_t_size)
{
    ASSERT_EQ(sizeof(void*), sizeof(fuse_req_t));
}