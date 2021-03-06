#include "webfuse/impl/operation/open.h"
#include "webfuse/impl/jsonrpc/error.h"

#include "webfuse/status.h"

#include "webfuse/test_util/json_doc.hpp"
#include "webfuse/mocks/mock_fuse.hpp"
#include "webfuse/mocks/mock_operation_context.hpp"
#include "webfuse/mocks/mock_jsonrpc_proxy.hpp"

#include <gtest/gtest.h>

using webfuse_test::JsonDoc;
using webfuse_test::MockJsonRpcProxy;
using webfuse_test::MockOperationContext;
using webfuse_test::FuseMock;
using testing::_;
using testing::Return;
using testing::StrEq;

TEST(wf_impl_operation_open, invoke_proxy)
{
    MockJsonRpcProxy proxy;
    EXPECT_CALL(proxy, wf_impl_jsonrpc_proxy_vinvoke(_,_,_,StrEq("open"),StrEq("sii"))).Times(1);

    MockOperationContext context;
    EXPECT_CALL(context, wf_impl_operation_context_get_proxy(_)).Times(1)
        .WillOnce(Return(reinterpret_cast<wf_jsonrpc_proxy*>(&proxy)));

    wf_impl_operation_context op_context;
    op_context.name = nullptr;
    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_req_userdata(_)).Times(1).WillOnce(Return(&op_context));


    fuse_req_t request = nullptr;
    fuse_ino_t inode = 1;
    fuse_file_info file_info;
    file_info.flags = 0;
    wf_impl_operation_open(request, inode, &file_info);
}

TEST(wf_impl_operation_open, fail_rpc_null)
{
    MockOperationContext context;
    EXPECT_CALL(context, wf_impl_operation_context_get_proxy(_)).Times(1)
        .WillOnce(Return(nullptr));

    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_req_userdata(_)).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(fuse, fuse_reply_err(_, ENOENT)).Times(1).WillOnce(Return(0));

    fuse_req_t request = nullptr;
    fuse_ino_t inode = 1;
    fuse_file_info * file_info = nullptr;
    wf_impl_operation_open(request, inode, file_info);
}

TEST(wf_impl_operation_open, finished)
{
    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_reply_open(_,_)).Times(1).WillOnce(Return(0));

    JsonDoc result("{\"handle\": 42}");
    wf_impl_operation_open_finished(nullptr, result.root(), nullptr);
}

TEST(wf_impl_operation_open, finished_fail_error)
{
    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_reply_open(_,_)).Times(0);
    EXPECT_CALL(fuse, fuse_reply_err(_, ENOENT)).Times(1).WillOnce(Return(0));

    struct wf_jsonrpc_error * error = wf_impl_jsonrpc_error(WF_BAD, "");
    wf_impl_operation_open_finished(nullptr, nullptr, error);
    wf_impl_jsonrpc_error_dispose(error);
}

TEST(wf_impl_operation_open, finished_fail_no_handle)
{
    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_reply_open(_,_)).Times(0);
    EXPECT_CALL(fuse, fuse_reply_err(_, ENOENT)).Times(1).WillOnce(Return(0));

    JsonDoc result("{}");
    wf_impl_operation_open_finished(nullptr, result.root(), nullptr);
}

TEST(wf_impl_operation_open, finished_fail_invalid_handle_type)
{
    FuseMock fuse;
    EXPECT_CALL(fuse, fuse_reply_open(_,_)).Times(0);
    EXPECT_CALL(fuse, fuse_reply_err(_, ENOENT)).Times(1).WillOnce(Return(0));

    JsonDoc result("{\"handle\": \"42\"}");
    wf_impl_operation_open_finished(nullptr, result.root(), nullptr);
}
