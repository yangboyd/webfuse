#include <string>
#include <gtest/gtest.h>

#include "webfuse/adapter/impl/jsonrpc/response.h"


static void response_parse_str(
	std::string const & buffer,
	struct wf_impl_jsonrpc_response * response)
{
	json_t * message = json_loadb(buffer.c_str(), buffer.size(), 0, nullptr);
	if (nullptr != message)
	{
		wf_impl_jsonrpc_response_init(response, message);
		json_decref(message);
	}
}

TEST(response_parser, test)
{
	struct wf_impl_jsonrpc_response response;

	// no object
	response_parse_str("[]", &response);
	ASSERT_NE(WF_GOOD, response.status);
	ASSERT_EQ(-1, response.id);
	ASSERT_EQ(nullptr, response.result);

	// empty
	response_parse_str("{}", &response);
	ASSERT_NE(WF_GOOD, response.status);
	ASSERT_EQ(-1, response.id);
	ASSERT_EQ(nullptr, response.result);

	// no data
	response_parse_str("{\"id\":42}", &response);
	ASSERT_NE(WF_GOOD, response.status);
	ASSERT_EQ(42, response.id);
	ASSERT_EQ(nullptr, response.result);

	// custom error code
	response_parse_str("{\"error\":{\"code\": 42}, \"id\": 42}", &response);
	ASSERT_NE(WF_GOOD, response.status);
	ASSERT_EQ(42, response.status);
	ASSERT_EQ(42, response.id);
	ASSERT_EQ(nullptr, response.result);

	// valid response
	response_parse_str("{\"result\": true, \"id\": 42}", &response);
	ASSERT_EQ(WF_GOOD, response.status);
	ASSERT_EQ(42, response.id);
	ASSERT_NE(nullptr, response.result);
	json_decref(response.result);
}