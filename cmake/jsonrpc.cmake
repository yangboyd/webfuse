# jsonrpc

add_library(jsonrpc STATIC
	lib/jsonrpc/src/jsonrpc/api.c
	lib/jsonrpc/src/jsonrpc/impl/proxy.c
	lib/jsonrpc/src/jsonrpc/impl/server.c
	lib/jsonrpc/src/jsonrpc/impl/method.c
	lib/jsonrpc/src/jsonrpc/impl/request.c
	lib/jsonrpc/src/jsonrpc/impl/response.c
	lib/jsonrpc/src/jsonrpc/impl/error.c
)

target_link_libraries(jsonrpc PUBLIC wf_timer)

target_include_directories(jsonrpc PRIVATE 
	lib/wf/timer/include
	lib/jsonrpc/src
	lib
)

target_include_directories(jsonrpc PUBLIC 
	lib/jsonrpc/include
)

set_target_properties(jsonrpc PROPERTIES C_VISIBILITY_PRESET hidden)
